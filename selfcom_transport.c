#include "selfcom_transport.h"
#include "session_manager.h"
#include "communication.h"
#include "string.h"
#include "lqueue.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
// #include "ch.h"

#define SELF_COM_TRANSPORT_BUFFER_SIZE      64
#define SELF_COM_TRANSPORT_RX_SIZE          64

typedef struct
{
    bool is_inited;
    uint8_t ipc_buff[SELF_COM_TRANSPORT_BUFFER_SIZE];
    uint8_t rx_buff[SELF_COM_TRANSPORT_RX_SIZE];
    VLQueue_ccb_t self_com_queue;
} self_com_tp_type;

tx_indication self_com_tx_indication = NULL;
rx_confirmation self_com_rx_confirmation = NULL;
uint8_t self_com_allocted_id;
static self_com_tp_type self_com;
static uxrCommunication self_com_urxCom;

// static THD_WORKING_AREA(selfcom_thread_wa, 512);
// static thread_t * selfcom_thread;
static TaskHandle_t selfcom_thread = NULL;

// uint16_t glen;

static bool send_self_com_msg(
        void* instance,
        const uint8_t* buf,
        size_t len)
{
    (void) instance;
    (void) buf;
    (void) len;
    bool rv = false;
    if (self_com.is_inited == false)
    {
        rv = false;
    }
    else
    {
        // memcpy(self_com.rx_buff, buf, len);
        if(self_com_tx_indication != NULL)
        {
            self_com_tx_indication(self_com_allocted_id);
        }
        if (!lqueue_push(&self_com.self_com_queue, buf, len))
        {
            rv = true;
            // chEvtSignalI(selfcom_thread, (eventmask_t) 0x01);
        }
    }
    return rv;
}

static bool recv_self_com_msg(
        void* instance,
        uint8_t** buf,
        size_t* len,
        int timeout)
{
    (void) instance;
    (void) timeout;
    //TODO: timeout
    bool rv = false;
    if (self_com.is_inited == false)
    {
        rv = false;
    }
    else if (0 >= lqueue_size(&self_com.self_com_queue))
    {
        rv = false;
    }
    else
    {
        size_t size = lqueue_pop(&self_com.self_com_queue, self_com.rx_buff, SELF_COM_TRANSPORT_RX_SIZE);
        // printf("\npop size:%d data:",size);
        // for (size_t i = 0; i < size; i++)
        // {
        //     printf("%c",self_com.rx_buff[i]);
        // }
        // printf("--------pop end\n");
        *buf = self_com.rx_buff;
        *len = size;
        rv = true;
    }
    return rv;
}


static uint8_t get_self_com_error(
        void)
{
    return 0;
}

static uint8_t registe_txrx_func(tx_indication tx_ind, rx_confirmation rx_conf, uint8_t protocol_id)
{
    self_com_tx_indication = tx_ind;
    self_com_rx_confirmation = rx_conf;
    self_com_allocted_id = protocol_id;
    return 0;
}

void selfcom_func(void *p)
{
    (void)p;
    while (1)
    {
        // eventmask_t events = chEvtWaitAny(ALL_EVENTS);
        if (0x01)
        {
            if(self_com_rx_confirmation != NULL)
            {
                self_com_rx_confirmation(self_com_allocted_id);
            }
        }
    }
}

bool self_com_transport_init(void)
{
    bool rv = false;

    if (self_com.is_inited == true)
    {
        rv = true;
    }
    //init self com queue
    else if (!lqueue_init(&self_com.self_com_queue, self_com.ipc_buff, SELF_COM_TRANSPORT_BUFFER_SIZE))
    {
        self_com_urxCom.instance = &self_com_urxCom;
        self_com_urxCom.comm_error = get_self_com_error;
        self_com_urxCom.send_msg = send_self_com_msg;
        self_com_urxCom.recv_msg = recv_self_com_msg;
        self_com_urxCom.comm_registe_txrx = registe_txrx_func;
        self_com.is_inited = true;
        xTaskCreate( selfcom_func, "self_com", 64, NULL, 11, &selfcom_thread );
        // selfcom_thread = chThdCreateStatic(selfcom_thread_wa, sizeof(selfcom_thread_wa), NORMALPRIO, selfcom_func, NULL);
        rv = true;
    }
    return rv;
}

bool self_com_transport_close(
        void* transport)
{
    bool rv = false;
    if (transport == &self_com_urxCom)
    {
        self_com.is_inited = false;
        rv = true;
    }
    else
    {
        rv = false;
    }
    return rv;
}

void* self_com_get_instance_info(void)
{
    return &self_com_urxCom;
}

