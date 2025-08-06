#include <communication.h>

#include "ch.h"
#include "lqueue.h"
#include "cancom_tansport.h"
#include "session_manager.h"

/* frame control information */
/**
 * Buffer structure:
 * byte0: source, byte1: destination, byte2~5: length, byte6~:data
 * 
*/
#define HEAD                                2
#define LENGTH                              4
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
#define CAN_HEAD                            0x15A00000
#define GET_MSGLEN(buf)                     *(uint32_t *)(buf+HEAD)
#define PREPARE_CAN_ID(srcId, desId)        (CAN_HEAD|(srcId<<8)|desId)
#define IS_HEAD_VALIDTE(canId)              ((canId & CAN_HEAD) == CAN_HEAD)

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

static mutex_t can_tp_mtx;
thread_t *comm_tp;
static THD_FUNCTION(can_tpTask, arg);
static THD_WORKING_AREA(cantp_thread_wa, 256);

static uint8_t udr_rxBuff[UXR_RX_BUFF]={0x0};
static CAN_HandleTypeDef* handle_ = &hcan;
bool send_message(uint32_t canId, uint8_t *data, uint8_t len);

static bool send_can_msg(
        void* instance,
        const uint8_t* buf,
        size_t len)
{
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
        portENTER_CRITICAL();
        if (!queue_push(&can.canTx_queue, buf, len))
        {
            rv = true;
        }
        else
        {
            rv = false;
        }
        portEXIT_CRITICAL();
        if (rv == true)
        {
            chEvtSignal(comm_tp, (eventmask_t) TX_REQUEST_EVENT);
        }
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
    bool rv = false;
    if (can.isInited == false)
    {
        rv = false;
    }
    else
    {
        uint8_t head[3];
        int32_t msgLen;
        // EventBits_t events;

        // events = xEventGroupWaitBits(dds_rxEventHandle, 0, pdTRUE, pdTRUE, portMAX_DELAY);
        /* read data from lower layer */
        portENTER_CRITICAL();

        /* pop send data */
        msgLen = queue_pop(&can.canRx_queue, udr_rxBuff, CAN_TRANSPORT_MTU);
        portEXIT_CRITICAL();
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
    uint8_t buffer[BUFFER_SIZE];

    chMtxLock(&can_tp_mtx);
    buffer[0] = (uint8_t)((canid >> 8) & 0xFF);//srcId
    buffer[1] = (uint8_t)(canid & 0xFF);//desId
    buffer[2] = len; //update length
    memcpy(&buffer[HEAD+LENGTH], data, len);
    if (!queue_push(&can.canRx_queue, (const uint8_t *)buffer, HEAD+LENGTH+header.DLC))
    {
        rv = true;
    }
    else
    {
        rv = false;
    }
    chMtxUnlock(&can_tp_mtx);
    if (rv == true)
    {
        chEvtSignal(comm_tp, (eventmask_t) RX_COMPLETED_EVENT);
    }
}

// Send a CAN message on the bus
bool send_message(uint32_t canId, uint8_t *data, uint8_t len) {
    if (HAL_CAN_GetError(handle_) != HAL_CAN_ERROR_NONE) {
    	HAL_CAN_ResetError(handle_);
		if (HAL_CAN_Start(handle_) == HAL_OK)
			HAL_CAN_ActivateNotification(handle_, CAN_IT_TX_MAILBOX_EMPTY |
				CAN_IT_RX_FIFO0_MSG_PENDING|CAN_IT_RX_FIFO0_OVERRUN |
				CAN_IT_RX_FIFO1_MSG_PENDING|CAN_IT_RX_FIFO1_OVERRUN);
        return false;
    }
    else
    {

    }

    CAN_TxHeaderTypeDef header;
    header.StdId = 0;
    header.ExtId = canId;
    header.IDE = CAN_ID_EXT;
    header.RTR = CAN_RTR_DATA;
    header.DLC = len;
    header.TransmitGlobalTime = DISABLE;

    uint32_t retTxMailbox = 0;
    if (!HAL_CAN_GetTxMailboxesFreeLevel(handle_)) {
        return false;
    }

    return HAL_CAN_AddTxMessage(handle_, &header, data, &retTxMailbox) == HAL_OK;
}

extern void comm_can_transmit_sid(uint32_t id, const uint8_t *data, uint8_t len);
void request2send(void)
{
    int32_t msgLen;
    uint16_t id;
    uint8_t buffer[BUFFER_SIZE];

    /* TODO: check lower layer(CAN) status */

    chMtxLock(&can_tp_mtx);
    /* pop source id and destination id */
    msgLen = queue_pop(&can.canTx_queue, buffer, BUFFER_SIZE);

    chMtxUnlock(&can_tp_mtx);
    if ((0 < msgLen) && (msgLen <= PAYLOAD_SIZE))
    {
        comm_can_transmit_sid(PREPARE_CAN_ID(buffer[0], buffer[1]), buffer+HEAD+LENGTH, msgLen-HEAD-LENGTH)
    }
}

static void can_tpTask(void* arg)
{
    for (;;)
    {
        uint32_t events;
        // events = xEventGroupWaitBits(canTp_eventGroupHandle, 0, pdTRUE, pdTRUE, portMAX_DELAY);
        xTaskNotifyWait(0, //not clear when enter this function
                        0xFFFFFFFF, //clear all bits when leaving this function
                        &events,    //read events bits
                        0xFFFFFFFF);//wait for max
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
            process_rx_fifo(0);
            // HAL_CAN_ActivateNotification(handle_, CAN_IT_RX_FIFO0_MSG_PENDING);
        }
        if (events & RX_NOTIFICATION_FIFO1)
        {
            process_rx_fifo(1);
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
    else if (queue_init(&can.canTx_queue, can.txBuff, PAYLOAD_SIZE))
    {
        rv = false;
    }
    else if (queue_init(&can.canRx_queue, can.rxBuff, PAYLOAD_SIZE))
    {
        rv = false;
    }
    else
    {
        chMtxObjectInit(&can_tp_mtx);
        comm_tp = chThdCreateStatic(cantp_thread_wa, sizeof(cantp_thread_wa), NORMALPRIO + 1,
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

