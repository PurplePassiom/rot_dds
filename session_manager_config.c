/********************************************************************
 * @file this file is the bridge of session layer and transport layer.
 * 
 * 
 ********************************************************************/
#include "session_manager_config.h"
#include "selfcom_transport.h"
#include "cancom_tansport.h"
#include "microcdr.h"

uint8_t session_uart_buffer[SESSION_BUFFER_SIZE];
uint8_t session_can_buffer[SESSION_BUFFER_SIZE];
uint8_t internal_buffer[SESSION_BUFFER_SIZE];
session_manager_callback_type cb_table[MAX_CALLBACK_NUM] = {0};

#ifdef CUSTOME_PROTOCOL
static void interal_stream_cb(uint8_t stream_index, uint8_t src_id, uint8_t des_id, uint8_t* data, uint16_t len){
    (void)stream_index;
    if (cb_table[des_id].cb != NULL)
    {
        cb_table[des_id].cb(src_id, des_id, data, len);
    }
}

static void uart_stream_cb(uint8_t stream_index, uint8_t src_id, uint8_t des_id, uint8_t* data, uint16_t len){
    (void)stream_index;
    if (cb_table[des_id].cb != NULL)
    {
        cb_table[des_id].cb(src_id, des_id, data, len);
    }
}

static void can_stream_cb(uint8_t stream_index, uint8_t src_id, uint8_t des_id, uint8_t* data, uint16_t len){
    (void)stream_index;
    if (cb_table[des_id].cb != NULL)
    {
        cb_table[des_id].cb(src_id, des_id, data, len);
    }
}

#else
static void on_uart_cb(
        uxrSession* session,
        uxrObjectId object_id,
        uint16_t request_id,
        uxrStreamId stream_id,
        struct ucdrBuffer* ub,
        uint16_t length,
        void* args);

static void on_can_cb(
    uxrSession* session,
    uxrObjectId object_id,
    uint16_t request_id,
    uxrStreamId stream_id,
    struct ucdrBuffer* ub,
    uint16_t length,
    void* args);

static void on_internal_cb(
        uxrSession* session,
        uxrObjectId object_id,
        uint16_t request_id,
        uxrStreamId stream_id,
        struct ucdrBuffer* ub,
        uint16_t length,
        void* args);

static void on_internal_cb(
        uxrSession* session,
        uxrObjectId object_id,
        uint16_t request_id,
        uxrStreamId stream_id,
        struct ucdrBuffer* ub,
        uint16_t length,
        void* args)
{
    uint8_t des_id = object_id.id>>4;
    (void) request_id; (void) stream_id; (void) args; (void) session;
    if (des_id > MAX_CALLBACK_NUM)
    {
        
    }
    else
    {
        uint8_t src_id = session->info.id;
        if (cb_table[des_id].cb != NULL)
        {
            cb_table[des_id].cb(src_id, des_id, ub->iterator, (uint16_t)length);
        }
    }
}

static void on_uart_cb(
        uxrSession* session,
        uxrObjectId object_id,
        uint16_t request_id,
        uxrStreamId stream_id,
        struct ucdrBuffer* ub,
        uint16_t length,
        void* args)
{
    uint8_t des_id = object_id.id>>4;
    (void) request_id; (void) stream_id; (void) args; (void) session;
    if (des_id > MAX_CALLBACK_NUM)
    {
        
    }
    else
    {
        uint8_t src_id = session->info.id;
        if (cb_table[des_id].cb != NULL)
        {
            cb_table[des_id].cb(src_id, des_id, ub->iterator, (uint16_t)length);
        }
    }
}

static void on_can_cb(
    uxrSession* session,
    uxrObjectId object_id,
    uint16_t request_id,
    uxrStreamId stream_id,
    struct ucdrBuffer* ub,
    uint16_t length,
    void* args)
{
    uint8_t des_id = object_id.id;
    (void) request_id; (void) stream_id; (void) length; (void) args; (void) session;
    if (des_id > MAX_CALLBACK_NUM)
    {
        
    }
    else
    {
        uint8_t src_id = session->info.id;
        if (cb_table[des_id].cb != NULL)
        {
            cb_table[des_id].cb(src_id, des_id, ub->iterator, (uint16_t)length);
        }
    }
}

#endif
// session_stream_buffer_type stream_buffer[MAX_SERVICE_ID];

session_data_type sessions[SESSION_NUMBER] = {
    {.id = 1, .protocol = SESSION_PROTOCOL_SELF_COM, 
    .session_buffer = internal_buffer, 
    .buffer_size = SESSION_BUFFER_SIZE,
    #ifdef CUSTOME_PROTOCOL
    .cb = interal_stream_cb,
    #else
    .cb = on_internal_cb,
    #endif
    // .tp_tx_indication = selfcom_tx_indication,
    // .tp_rx_confirmation = selfcom_rx_confirmation,
    .init_func = self_com_transport_init},

    {.id = 2, .protocol = SESSION_PROTOCOL_UART_COM, 
    .session_buffer = session_uart_buffer, 
    .buffer_size = SESSION_BUFFER_SIZE,
    #ifdef CUSTOME_PROTOCOL
    .cb = uart_stream_cb,
    #else
    .cb = on_uart_cb, 
    #endif
    .init_func = NULL},

    {.id = 3, .protocol = SESSION_PROTOCOL_CAN_COM, 
    .session_buffer = session_can_buffer, 
    .buffer_size = SESSION_BUFFER_SIZE,
    #ifdef CUSTOME_PROTOCOL
    .cb = can_stream_cb,
    #else
    .cb = on_can_cb,
    #endif
    .init_func = can_transport_init},
};
