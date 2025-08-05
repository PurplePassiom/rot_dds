#include <communication.h>

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "can.h"
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
static TaskHandle_t canTp_handle;
// StaticEventGroup_t canTp_eventGroup;
// StaticEventGroup_t dds_rxEventGroup;
// EventGroupHandle_t canTp_eventGroupHandle;
// EventGroupHandle_t dds_rxEventHandle;

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
            xTaskNotify(canTp_handle,TX_REQUEST_EVENT, eSetBits);
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

void process_rx_fifo(uint32_t fifo) {
    while (HAL_CAN_GetRxFifoFillLevel(handle_, fifo)) {
        CAN_RxHeaderTypeDef header;
        uint8_t buff[BUFFER_SIZE];
        HAL_CAN_GetRxMessage(handle_, fifo, &header, &buff[HEAD+LENGTH]);
        if ((header.IDE == CAN_ID_EXT) && 
            (IS_HEAD_VALIDTE(header.ExtId)) &&
            (header.DLC <= PAYLOAD_SIZE))
        {
            bool rv;
            buff[0] = (uint8_t)((header.ExtId >> 8) & 0xFF);//srcId
            buff[1] = (uint8_t)(header.ExtId & 0xFF);//desId
            buff[2] = header.DLC; //update length
            portENTER_CRITICAL();
            if (!queue_push(&can.canRx_queue, (const uint8_t *)buff, HEAD+LENGTH+header.DLC))
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
                if (can_callback.rx_confirmation != NULL)
                {
                    can_callback.rx_confirmation(can_callback.protocol_id);
                }
                // session_set_event(2);
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
    uint8_t buffer[BUFFER_SIZE];

    /* TODO: check lower layer(CAN) status */

    portENTER_CRITICAL();
    /* pop source id and destination id */
    msgLen = queue_pop(&can.canTx_queue, buffer, BUFFER_SIZE);

    portEXIT_CRITICAL();
    if ((0 < msgLen) && (msgLen <= PAYLOAD_SIZE))
    {
        if (false == send_message(PREPARE_CAN_ID(buffer[0], buffer[1]), buffer+HEAD+LENGTH, msgLen-HEAD-LENGTH))
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
        xTaskCreate( can_tpTask, "can_transport", 128, NULL, 11, &canTp_handle );
        /* this event group is used to trigger tx and rx */
        // canTp_eventGroupHandle = xEventGroupCreateStatic(&canTp_eventGroup);
        /* this event group is used to trigger dds rx thread */
        dds_rxEventHandle = xEventGroupCreateStatic(&dds_rxEventGroup);

        can_urxCom.instance = &can_urxCom;
        can_urxCom.comm_error = get_can_error;
        can_urxCom.send_msg = send_can_msg;
        can_urxCom.recv_msg = recv_can_msg;
        can_urxCom.comm_registe_txrx = can_registe_cb;
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
