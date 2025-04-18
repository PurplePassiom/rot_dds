#include <uxr/client/core/communication/communication.h>

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "can.h"
#include "pqueue.h"
#include "cancom_tansport.h"
#include "session_manager.h"


#define SELF_COM_TRANSPORT_BUFFER_SIZE      64
#define SELF_COM_TRANSPORT_RX_SIZE          64
#define CAN_TRANSPORT_MTU                   7
#define UXR_RX_BUFF                         32
#define CAN_SF_SIZE                         8

#define DDS_RX_EVENT                        0x01

#define TX_REQUEST_EVENT                    0x01
#define TX_CONFIRM_EVENT                    0x02
#define RX_COMPLETED_EVENT                  0x04
#define RX_NOTIFICATION_FIFO0               0x08
#define RX_NOTIFICATION_FIFO1               0x10

#define SESSIONID_OFFSET                    0u
#define MSGLEN_OFFSET                       2u
#define DESTIN_OFFSET                       1u
#define DATA_OFFSET                         6u
#define GET_MSGLEN(buf)                     *(uint32_t *)(buf+MSGLEN_OFFSET)
#define GET_SRCID(buf)                      ((buf[0])&0x7f)
#define GET_DESID(buf)                      (uint8_t)(((buf[DESTIN_OFFSET])&0xf0) >> 4)

#define CAN_HEAD                            0x15A00000
#define IS_HEAD_VALIDTE(canId)              ((canId & CAN_HEAD) == CAN_HEAD)
typedef struct
{
    bool isInited;
    uint8_t txBuff[SELF_COM_TRANSPORT_BUFFER_SIZE];
    uint8_t rxBuff[SELF_COM_TRANSPORT_RX_SIZE];
    VLQueue_ccb_t canTx_queue;
    VLQueue_ccb_t canRx_queue;
} canTp_InfoType;

static canTp_InfoType can;
static uxrCommunication can_urxCom;
TaskHandle_t canTp_handle;
StaticEventGroup_t canTp_eventGroup;
StaticEventGroup_t dds_rxEventGroup;
EventGroupHandle_t canTp_eventGroupHandle;
EventGroupHandle_t dds_rxEventHandle;
/********************************sessionid, streamid ************************/
uint8_t udr_rxBuff[UXR_RX_BUFF]={0x0};
static CAN_HandleTypeDef* handle_ = &hcan;
bool send_message(uint32_t canId, uint8_t *data, uint8_t len);

static bool send_can_msg(
        void* instance,
        const uint8_t* buf,
        size_t len)
{
    bool rv = false;
    #if 1
    uint16_t msgLen = GET_MSGLEN(buf);
    if (can.isInited == false)
    {
        rv = false;
    }
    else if (msgLen > CAN_TRANSPORT_MTU)
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
                xTaskNotify(canTp_handle,TX_REQUEST_EVENT, eSetBits);
            }
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
        /* pop source id and destination id */
        msgLen = queue_pop(&can.canRx_queue, (uint8_t*)head, 3);
        if (msgLen == 3)
        {
            /* pop send data */
            msgLen = queue_pop(&can.canRx_queue, udr_rxBuff+DATA_OFFSET, CAN_TRANSPORT_MTU);
        }
        else
        {
            /* error */
        }
        portEXIT_CRITICAL();
        if (msgLen)
        {
            //Destination id is the session id which is in first byte
            udr_rxBuff[SESSIONID_OFFSET] = head[0];
            //source id is the object id
            udr_rxBuff[DESTIN_OFFSET] = head[1];
            //message length 
            udr_rxBuff[MSGLEN_OFFSET] = head[2];
            *buf = udr_rxBuff;
            *len = msgLen + DATA_OFFSET;
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

void process_rx_fifo(uint32_t fifo) {
    while (HAL_CAN_GetRxFifoFillLevel(handle_, fifo)) {
        CAN_RxHeaderTypeDef header;
        uint8_t buff[64];
        HAL_CAN_GetRxMessage(handle_, fifo, &header, buff);
        if ((header.IDE == CAN_ID_EXT) && 
            (IS_HEAD_VALIDTE(header.ExtId)) &&
            (header.DLC <= CAN_SF_SIZE))
        {
            bool rv;
            uint8_t head[3];
            uint8_t msgLen = buff[0];
            head[0] = (uint8_t)((header.ExtId >> 8) & 0xFF);
            head[1] = (uint8_t)(header.ExtId & 0xFF);
            head[2] = msgLen;
            portENTER_CRITICAL();
            if (!queue_push(&can.canRx_queue, (const uint8_t *)head, 3))
            {
                if (!queue_push(&can.canRx_queue, buff+1, msgLen))
                {
                    rv = true;
                }
                else
                {
                    rv = false;
                    //dequeue previous id
                    queue_pop(&can.canRx_queue, NULL, 2);
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
                session_set_event(2);
                //xEventGroupSetBits(dds_rxEventHandle, DDS_RX_EVENT);
                // xEventGroupSetBits(dds_rxEventHandle, RX_COMPLETED_EVENT);
            }
        }
        else
        {
            /* error */
        }
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

void request2send(void)
{
    int32_t msgLen;
    uint16_t id;
    uint8_t payload[CAN_TRANSPORT_MTU];

    /* check lower layer(CAN) status */

    portENTER_CRITICAL();
    /* pop source id and destination id */
    msgLen = queue_pop(&can.canTx_queue, (uint8_t *)&id, 2);
    if (msgLen == 2)
    {
        /* pop send data */
        msgLen = queue_pop(&can.canTx_queue, payload + 1, CAN_TRANSPORT_MTU); //OFFSET one byte for length
    }
    else
    {
        /* error */
    }
    portEXIT_CRITICAL();
    if ((msgLen > 0) && (msgLen <= CAN_TRANSPORT_MTU))
    {
        payload[0] = (uint8_t)msgLen;
        if (false == send_message((uint32_t)(CAN_HEAD | id), payload, msgLen+1))
        {
            //error
        }
    }
}

static void can_tpTask(void* pvParameters)
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
            xEventGroupSetBits(dds_rxEventHandle, DDS_RX_EVENT);
        }
        /* received single can frame */
        if (events & RX_NOTIFICATION_FIFO0)
        {
            process_rx_fifo(0);
            HAL_CAN_ActivateNotification(handle_, CAN_IT_RX_FIFO0_MSG_PENDING);
        }
        if (events & RX_NOTIFICATION_FIFO1)
        {
            process_rx_fifo(1);
            HAL_CAN_ActivateNotification(handle_, CAN_IT_RX_FIFO1_MSG_PENDING);
        }
    }
}

bool can_transport_init(void)
{
    bool rv = false;

    if (can.isInited == true)
    {
        rv = false;
    }
    else if (queue_init(&can.canTx_queue, can.txBuff, SELF_COM_TRANSPORT_BUFFER_SIZE))
    {
        rv = false;
    }
    else if (queue_init(&can.canRx_queue, can.rxBuff, SELF_COM_TRANSPORT_RX_SIZE))
    {
        rv = false;
    }
    else
    {
        xTaskCreate( can_tpTask, "can_transport", 512, NULL, 11, &canTp_handle );
        /* this event group is used to trigger tx and rx */
        // canTp_eventGroupHandle = xEventGroupCreateStatic(&canTp_eventGroup);
        /* this event group is used to trigger dds rx thread */
        dds_rxEventHandle = xEventGroupCreateStatic(&dds_rxEventGroup);

        can_urxCom.instance = &can_urxCom;
        can_urxCom.comm_error = get_can_error;
        can_urxCom.send_msg = send_can_msg;
        can_urxCom.recv_msg = recv_can_msg;
        can.isInited = true;

        //start can interrupt
//        HAL_CAN_Init(handle_);
        HAL_CAN_ActivateNotification(handle_, CAN_IT_TX_MAILBOX_EMPTY |
        CAN_IT_RX_FIFO0_MSG_PENDING|CAN_IT_RX_FIFO0_OVERRUN |
        CAN_IT_RX_FIFO1_MSG_PENDING|CAN_IT_RX_FIFO1_OVERRUN);
        HAL_CAN_Start(handle_);
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

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
    // HAL_CAN_DeactivateNotification(hcan, CAN_IT_TX_MAILBOX_EMPTY);
    // xEventGroupSetBitsFromISR(canTp_eventGroupHandle, TX_CONFIRM_EVENT, &xHigherPriorityTaskWoken);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(canTp_handle,TX_CONFIRM_EVENT, eSetBits, &xHigherPriorityTaskWoken);
}
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(canTp_handle,TX_CONFIRM_EVENT, eSetBits, &xHigherPriorityTaskWoken);
}
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(canTp_handle,TX_CONFIRM_EVENT, eSetBits, &xHigherPriorityTaskWoken);
}
void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *hcan) {}
void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *hcan) {}
void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *hcan) {}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    HAL_CAN_DeactivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(canTp_handle,RX_NOTIFICATION_FIFO0, eSetBits, &xHigherPriorityTaskWoken);
}
void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan) {
    // HAL_CAN_DeactivateNotification(hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
    
}
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    HAL_CAN_DeactivateNotification(hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(canTp_handle,RX_NOTIFICATION_FIFO1, eSetBits, &xHigherPriorityTaskWoken);
}
void HAL_CAN_RxFifo1FullCallback(CAN_HandleTypeDef *hcan) {}
void HAL_CAN_SleepCallback(CAN_HandleTypeDef *hcan) {}
void HAL_CAN_WakeUpFromRxMsgCallback(CAN_HandleTypeDef *hcan) {}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
    //HAL_CAN_ResetError(hcan);
}
