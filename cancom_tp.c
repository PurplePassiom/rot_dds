#include <communication.h>

#include "ch.h"
#include "hal.h"
#include "lqueue.h"
#include "cancom_tansport.h"
#include "session_manager.h"
#include <string.h>

/* frame control information */
/**
 * Buffer structure:
 * byte0: source, byte1: destination, byte2~5: length, byte6~:data
 * 
*/
#define HEAD                                2
#define LENGTH                              2
#define PAYLOAD_SIZE                        8
#define BUFFER_SIZE                         (HEAD + LENGTH + PAYLOAD_SIZE)
#define UXR_RX_BUFF                         32

/* CAN transport events */
#define DDS_RX_EVENT                        0x01
#define TX_REQUEST_EVENT                    0x01
#define TX_CONFIRM_EVENT                    0x02
#define RX_COMPLETED_EVENT                  0x04
#define RX_NOTIFICATION_FIFO0               0x08
#define RX_NOTIFICATION_FIFO1               0x10

/* utils */
#define CAN_HEAD                                    0x15000000
#define GET_MSGLEN(buf)                             *(uint32_t *)(buf+HEAD)
#define PREPARE_CAN_ID(srcId, desId,subId)          (CAN_HEAD|(srcId<<16)|(desId<<8)|subId)
#define IS_HEAD_VALIDTE(canId)                      ((canId & CAN_HEAD) == CAN_HEAD)

typedef struct
{
    bool isInited;
    uint8_t txBuff[64];
    uint8_t rxBuff[64];
    VLQueue_ccb_t canTx_queue;
    VLQueue_ccb_t canRx_queue;
} canTp_InfoType;

typedef struct
{
    tx_indication tx_indication;
    rx_confirmation rx_confirmation;
    uint8_t protocol_id;
}can_Cb_Type;

static can_Cb_Type can_callback;
static canTp_InfoType can;
static uxrCommunication can_urxCom;

static mutex_t can_tx_mtx;
static mutex_t can_rx_mtx;
thread_t *comm_tp;
static THD_FUNCTION(can_tpTask, arg);
static THD_WORKING_AREA(cantp_thread_wa, 512);

static uint8_t udr_rxBuff[UXR_RX_BUFF]={0x0};

static bool send_can_msg(
        void* instance,
        const uint8_t* buf,
        size_t len)
{
    (void)instance;
    bool rv = false;
    #if 1
    if (can.isInited == false)
    {
        rv = false;
    }
    else if (len > BUFFER_SIZE)
    {
        rv = false;
    }
    else
    {
        #if 0
        CANTxFrame txmsg;
        chMtxLock(&can_tx_mtx);
        txmsg.EID = PREPARE_CAN_ID(buf[0], buf[1], buf[HEAD + LENGTH]);
        txmsg.DLC = buf[HEAD];//length
        txmsg.IDE = CAN_IDE_EXT;
	    txmsg.RTR = CAN_RTR_DATA;
        memcpy(txmsg.data8, buf + HEAD + LENGTH, txmsg.DLC);
        if (MSG_OK == canTransmit(&HW_CAN_DEV, CAN_ANY_MAILBOX, &txmsg, 10))
        {
            rv = true;
        }
        else
        {
            rv = false;
        }
        chMtxUnlock(&can_tx_mtx);
        #else
        if (!lqueue_push(&can.canTx_queue, buf, len))
        {
            rv = true;
        }
        else
        {
            rv = false;
        }
        chMtxUnlock(&can_tx_mtx);
        if (rv == true)
        {
            chEvtSignalI(comm_tp, (eventmask_t) TX_REQUEST_EVENT);
        }
        #endif
    }
    #endif
    return rv;
}

static bool recv_can_msg(
        void* instance,
        uint8_t** buf,
        size_t* len,
        int timeout)
{
    (void)timeout;
    (void)instance;
    bool rv = false;
    if (can.isInited == false)
    {
        rv = false;
    }
    else
    {
        int32_t msgLen;
        // EventBits_t events;

        // events = xEventGroupWaitBits(dds_rxEventHandle, 0, pdTRUE, pdTRUE, portMAX_DELAY);
        /* read data from lower layer */
        chMtxLock(&can_rx_mtx);

        /* pop send data */
        msgLen = lqueue_pop(&can.canRx_queue, udr_rxBuff, UXR_RX_BUFF);
        chMtxUnlock(&can_rx_mtx);
        if (msgLen)
        {
            *buf = udr_rxBuff;
            *len = msgLen;
            rv = true;
        }
    }
    return rv;
}


static uint8_t get_can_error(
        void)
{
    return 0;
}


void receive_can_frame(uint32_t canid, uint8_t *data, uint8_t len) {
    bool rv;
    uint8_t buffer[BUFFER_SIZE];
    chMtxLock(&can_rx_mtx);
    buffer[0] = (uint8_t)((canid >> 16) & 0xFF);//srcId
    buffer[1] = (uint8_t)((canid>>8) & 0xFF);//desId
    buffer[2] = len; //update length
    buffer[3] = 0;
    memcpy(&buffer[HEAD+LENGTH], data, len);
    if (!lqueue_push(&can.canRx_queue, (const uint8_t *)buffer, HEAD + LENGTH + len))
    {
        rv = true;
    }
    else
    {
        rv = false;
    }
    chMtxUnlock(&can_rx_mtx);
    if (rv == true)
    {
        chEvtSignal(comm_tp, (eventmask_t) RX_COMPLETED_EVENT);
    }
}


void request2send(void)
{
    int32_t msgLen = 0;
    uint8_t buffer[BUFFER_SIZE];

    /* TODO: check lower layer(CAN) status */
    
    do{
        chMtxLock(&can_tx_mtx);
        /* pop source id and destination id */
        msgLen = lqueue_pop(&can.canTx_queue, buffer, BUFFER_SIZE);
        if ((0 < msgLen))
        {
            CANTxFrame txmsg;
            txmsg.EID = PREPARE_CAN_ID(buffer[0], buffer[1], buffer[HEAD + LENGTH]);
            txmsg.DLC = buffer[HEAD];//length
            txmsg.IDE = CAN_IDE_EXT;
            txmsg.RTR = CAN_RTR_DATA;
            memcpy(txmsg.data8, buffer + HEAD + LENGTH, txmsg.DLC);
            canTransmit(&HW_CAN_DEV, CAN_ANY_MAILBOX, &txmsg, 10);
        }
        chMtxUnlock(&can_tx_mtx);
    }while (msgLen > 0);
}

static void can_tpTask(void* arg)
{
    (void)arg;
    for (;;)
    {
        uint32_t events;
        events = chEvtWaitAny(ALL_EVENTS);
        if (events & TX_REQUEST_EVENT)
        {
            /* request can layer to send message */
            request2send();
        }
        if (events & TX_CONFIRM_EVENT)
        {
            /* check is any data waiting to send */
            request2send();
        }
        /* received completed packet */
        if (events & RX_COMPLETED_EVENT)
        {
            if (can_callback.rx_confirmation != NULL)
            {
                can_callback.rx_confirmation(can_callback.protocol_id);
            }
        }
        /* received single can frame */
        if (events & RX_NOTIFICATION_FIFO0)
        {
            // HAL_CAN_ActivateNotification(handle_, CAN_IT_RX_FIFO0_MSG_PENDING);
        }
        if (events & RX_NOTIFICATION_FIFO1)
        {
            // HAL_CAN_ActivateNotification(handle_, CAN_IT_RX_FIFO1_MSG_PENDING);
        }
    }
}

uint8_t can_registe_cb(tx_indication tx_ind, rx_confirmation rx_conf, uint8_t protocol_id)
{
    if (tx_ind != NULL && rx_conf != NULL)
    {
        can_callback.tx_indication = tx_ind;
        can_callback.rx_confirmation = rx_conf;
        can_callback.protocol_id = protocol_id;
        return 0; // success
    }
    return 1; // error
}

bool can_transport_init(void)
{
    bool rv = false;

    if (can.isInited == true)
    {
        rv = false;
    }
    else if (lqueue_init(&can.canTx_queue, can.txBuff, 64))
    {
        rv = false;
    }
    else if (lqueue_init(&can.canRx_queue, can.rxBuff, 64))
    {
        rv = false;
    }
    else
    {
        chMtxObjectInit(&can_tx_mtx);
        chMtxObjectInit(&can_rx_mtx);
        comm_tp = chThdCreateStatic(cantp_thread_wa, sizeof(cantp_thread_wa), NORMALPRIO-2,
			can_tpTask, NULL);

        can_urxCom.instance = &can_urxCom;
        can_urxCom.comm_error = get_can_error;
        can_urxCom.send_msg = send_can_msg;
        can_urxCom.recv_msg = recv_can_msg;
        can_urxCom.comm_registe_txrx = can_registe_cb;
        can.isInited = true;
    }
    return rv;
}

bool can_transport_close(
        void* transport)
{
    bool rv = false;
    if (transport == &can_urxCom)
    {
        can.isInited = false;
        rv = true;
    }
    else
    {
        rv = false;
    }
    return rv;
}

void* can_get_instance_info(void)
{
    return &can_urxCom;
}

