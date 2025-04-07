#include <uxr/client/core/communication/communication.h>

#include "pqueue.h"
#include "selfcom_transport.h"
#include "session_manager.h"

#define SELF_COM_TRANSPORT_BUFFER_SIZE      512
#define SELF_COM_TRANSPORT_RX_SIZE          256

typedef struct
{
    bool is_inited;
    uint8_t ipc_buff[SELF_COM_TRANSPORT_BUFFER_SIZE];
    uint8_t rx_buff[SELF_COM_TRANSPORT_RX_SIZE];
    VLQueue_ccb_t self_com_queue;
} self_com_tp_type;

static self_com_tp_type self_com;
static uxrCommunication self_com_urxCom;

static bool send_self_com_msg(
        void* instance,
        const uint8_t* buf,
        size_t len)
{
    
    bool rv = false;
    if (self_com.is_inited == false)
    {
        rv = false;
    }
    else
    {
        if (!queue_push(&self_com.self_com_queue, buf, len))
        {
            rv = true;
            session_set_event(0);
        }
        // printf("\nrv:%d len:%d data: ",rv, len);
        // for (size_t i = 0; i < len; i++)
        // {
        //     printf("%c",buf[i]);
        // }
        // printf("--------------send end\n");
    }
    return rv;
}

static bool recv_self_com_msg(
        void* instance,
        uint8_t** buf,
        size_t* len,
        int timeout)
{
    //TODO: timeout
    bool rv = false;
    if (self_com.is_inited == false)
    {
        rv = false;
    }
    else if (0 >= queue_size(&self_com.self_com_queue))
    {
        rv = false;
    }
    else
    {
        size_t size = queue_pop(&self_com.self_com_queue, self_com.rx_buff, SELF_COM_TRANSPORT_RX_SIZE);
        // printf("\npop size:%d data:",size);
        // for (size_t i = 0; i < size; i++)
        // {
        //     printf("%c",self_com.rx_buff[i]);
        // }
        // printf("--------pop end\n");
        if (size)
        {
            *buf = self_com.rx_buff;
            *len = size;
            rv = true;
        }
    }
    return rv;
}


static uint8_t get_self_com_error(
        void)
{
    return 0;
}

bool self_com_transport_init(void)
{
    bool rv = false;

    if (self_com.is_inited == true)
    {
        rv = true;
    }
    //init self com queue
    else if (!queue_init(&self_com.self_com_queue, self_com.ipc_buff, SELF_COM_TRANSPORT_BUFFER_SIZE))
    {
        self_com_urxCom.instance = &self_com_urxCom;
        self_com_urxCom.comm_error = get_self_com_error;
        self_com_urxCom.send_msg = send_self_com_msg;
        self_com_urxCom.recv_msg = recv_self_com_msg;
        self_com.is_inited = true;
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
