#include <uxr/client/core/communication/communication.h>

#include "queue.h"
#include "FreeRTOS.h"
// #include "selfcom_transport.h"

#define SELF_COM_TRANSPORT_BUFFER_SIZE      512
#define SELF_COM_TRANSPORT_RX_SIZE          256
#define CAN_TRANSPORT_MTU                   31
#define CAN_SF_SIZE                         8

#define DDS_RX_EVENT                        0x01

#define TX_REQUEST_EVENT                    0x01
#define TX_CONFIRM_EVENT                    0x02
#define RX_COMPLETED_EVENT                  0x04

#define MSGLEN_OFFSET                       5u
#define DESTIN_OFFSET                       9u
#define DATA_OFFSET                         11u
#define GET_MSGLEN(buf)                     *((uint16_t*)(buf+MSGLEN_OFFSET))
#define GET_SRCID(buf)                      ((buf[0])&0x7f)
#define GET_DESID(buf)                      ((buf[DESTIN_OFFSET])&0xf)
#define CALC_HEAD(sn, len)                  (uint8_t)((sn<<4)|(len&0x0f))

typedef struct
{
    bool isInited;
    uint8_t ipcBuff[SELF_COM_TRANSPORT_BUFFER_SIZE];
    uint8_t rxBuff[SELF_COM_TRANSPORT_RX_SIZE];
    VLQueue_ccb_t canTx_queue;
    VLQueue_ccb_t canRx_queue;
} canTp_InfoType;

typedef struct
{
    uint16_t srcNdest_Id;
    uint8_t data[CAN_TRANSPORT_MTU];
    uint16_t len;
    uint16_t index;
    uint8_t sn;
#define CAN_TX_IDLE     0
#define CAN_TX_SENDING  1
    uint8_t state;
}canTp_TxBuffType;


static canTp_InfoType can;
static canTp_TxBuffType canTxBuff;
static uxrCommunication can_urxCom;
TaskHandle_t canTp_handle;
StaticEventGroup_t canTp_eventGroup;
StaticEventGroup_t dds_rxEventGroup;
EventGroupHandle_t canTp_eventGroupHandle;
EventGroupHandle_t dds_rxEventHandle;

static bool send_can_msg(
        void* instance,
        const uint8_t* buf,
        size_t len)
{
    bool rv = false;
    uint16_t msgLen = GET_MSGLEN(buf);
    if (can.isInited == false)
    {
        rv = false;
    }
    else if (len > CAN_TRANSPORT_MTU)
    {
        rv = false;
    }
    else
    {
        uint8_t srcId = GET_SRCID(buf);
        uint8_t desId = GET_DESID(buf);
        uint16_t id = (srcId << 8) | desId;
        //check srcId and desId is valid
        if (srcId > 0x7f || desId > 0x7f)
        {
            rv = false;
        }
        else
        {
            portENTER_CRITICAL();
            if (!queue_push(&can.canTx_queue, (const uint8_t *)&id, 2))
            {
                if (!queue_push(&can.canTx_queue, buf+DATA_OFFSET, msgLen))
                {
                    rv = true;
                }
                else
                {
                    rv = false;
                    //dequeue previous id
                    queue_pop(&can.canTx_queue, NULL, 2);
                }
            }
            else
            {
                rv = false;
                //ERROR
            }
            portEXIT_CRITICAL();
            if (rv == true)
            {
                xEventGroupSetBits(canTp_eventGroupHandle, TX_REQUEST_EVENT);
            }
        }
    }
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
        EventBits_t events;
        events = xEventGroupWaitBits(dds_rxEventHandle, 0, pdTRUE, pdTRUE, portMAX_DELAY);
        /* read data from lower layer */
        rv = (events > 0? true: false);
    }
    return rv;
}


static uint8_t get_can_error(
        void)
{
    return 0;
}

static void request2send(void)
{
    uint16_t id;
    uint8_t data[CAN_SF_SIZE];
    int32_t msgLen;
    /* check lower layer states */

    /* if any request is ongoing */
    if (canTxBuff.state == CAN_TX_SENDING)
    {
        uint16_t remainLen = canTxBuff.len - canTxBuff.index;
        if (remainLen > CAN_SF_SIZE)
        {
            canTxBuff.sn++;
            /* send next frame */
            data[0] = CALC_HEAD(canTxBuff.sn, remainLen);
            memcpy(data+1, canTxBuff.data+canTxBuff.index, CAN_SF_SIZE-1);
            // send next frame
            canTxBuff.index += CAN_SF_SIZE - 1;
        }
        else
        {
            canTxBuff.sn++;
            /* send last frame */
            data[0] = CALC_HEAD(canTxBuff.sn, remainLen);
            memcpy(data+1, canTxBuff.data+canTxBuff.index, remainLen);
            // send last frame
            canTxBuff.state = CAN_TX_IDLE;
        }
        canTxBuff.index = CAN_SF_SIZE - 1;
        return;
    }

    portENTER_CRITICAL();
    /* pop source id and destination id */
    msgLen = queue_pop(&can.canTx_queue, &id, 2);
    if (msgLen == 2)
    {
        /* pop send data */
        msgLen = queue_pop(&can.canTx_queue, canTxBuff.data, CAN_TRANSPORT_MTU);
    }
    else
    {
        /* error */
    }
    portEXIT_CRITICAL();

    /* length is valided request to send */
    /* single frame */
    if (msgLen && (CAN_SF_SIZE >= msgLen))
    {
        /* call send function */
    }
    /* multi frame */
    else if (msgLen && (CAN_SF_SIZE < msgLen))
    {
        canTxBuff.len = msgLen;
        canTxBuff.sn = 0;
        canTxBuff.state = CAN_TX_SENDING;
        // send first frame
        data[0] = CALC_HEAD(canTxBuff.sn, canTxBuff.len);
        memcpy(data+1, canTxBuff.data, CAN_SF_SIZE-1);
        // send first frame
        canTxBuff.index = CAN_SF_SIZE - 1;
        /* code */
    }
    else
    {
        /* code */
    }
}
static void can_tpTask(void* pvParameters)
{
    for (;;)
    {
        EventBits_t events;
        events = xEventGroupWaitBits(canTp_eventGroupHandle, 0, pdTRUE, pdTRUE, portMAX_DELAY);
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
        if (events & RX_COMPLETED_EVENT)
        {
            xEventGroupSetBits(dds_rxEventHandle, DDS_RX_EVENT);
        }
    }
}

bool can_transport_init(void)
{
    bool rv = false;

    if (can.isInited == false)
    {
        rv = false;
    }
    else if (queue_init(&can.canTx_queue, can.ipcBuff, SELF_COM_TRANSPORT_BUFFER_SIZE))
    {
        rv = false;
    }
    else if (queue_init(&can.canRx_queue, can.rxBuff, SELF_COM_TRANSPORT_BUFFER_SIZE))
    {
        rv = false;
    }
    else
    {
        xTaskCreate( can_tpTask, "can_transport", 512, NULL, 10, &canTp_handle );
        /* this event group is used to trigger tx and rx */
        canTp_eventGroupHandle = xEventGroupCreateStatic(&canTp_eventGroup);
        /* this event group is used to trigger dds rx thread */
        dds_rxEventHandle = xEventGroupCreateStatic(&dds_rxEventGroup);

        can_urxCom.instance = &can_urxCom;
        can_urxCom.comm_error = get_can_error;
        can_urxCom.send_msg = send_can_msg;
        can_urxCom.recv_msg = recv_can_msg;
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