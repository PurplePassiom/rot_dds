#include "session_manager_config.h"
#include "selfcom_transport.h"


uint8_t session_uart_buffer[SESSION_BUFFER_SIZE];
uint8_t session_can_buffer[SESSION_BUFFER_SIZE];
uint8_t internal_buffer[SESSION_BUFFER_SIZE];

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

// session_stream_buffer_type stream_buffer[MAX_SERVICE_ID];

session_data_type sessions[SESSION_NUMBER] = {
    {.id = 1, .protocol = SESSION_PROTOCOL_SELF_COM, 
    .session_buffer = internal_buffer, 
    .buffer_size = SESSION_BUFFER_SIZE,
    .cb = on_internal_cb},
    {.id = 2, .protocol = SESSION_PROTOCOL_UART_COM, 
    .session_buffer = session_uart_buffer, 
    .buffer_size = SESSION_BUFFER_SIZE,
    .cb = on_uart_cb},
    {.id = 3, .protocol = SESSION_PROTOCOL_CAN_COM, 
    .session_buffer = session_can_buffer, 
    .buffer_size = SESSION_BUFFER_SIZE,
    .cb = on_can_cb},
};

sessionM_callback_type cb_table[MAX_CALLBACK_NUM] = {0};

static void on_internal_cb(
        uxrSession* session,
        uxrObjectId object_id,
        uint16_t request_id,
        uxrStreamId stream_id,
        struct ucdrBuffer* ub,
        uint16_t length,
        void* args)
{
    (void) request_id; (void) stream_id; (void) length;
    if (object_id.id > MAX_CALLBACK_NUM)
    {
        
    }
    else
    {
        uint32_t len = 0;
        uint8_t des_id = object_id.id;
        if (cb_table[des_id].cb != NULL)
        {
            ucdr_deserialize_uint32_t(ub, &len);
            cb_table[des_id].cb(des_id, ub->iterator, (uint16_t)len);
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
    (void) request_id; (void) stream_id; (void) length;
    if (object_id.id > MAX_CALLBACK_NUM)
    {
        
    }
    else
    {
        uint32_t len = 0;
        uint8_t des_id = object_id.id;
        if (cb_table[des_id].cb != NULL)
        {
            ucdr_deserialize_uint32_t(ub, &len);
            cb_table[des_id].cb(des_id, ub->iterator, (uint16_t)len);
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
(void) request_id; (void) stream_id; (void) length;
if (object_id.id > MAX_CALLBACK_NUM)
{
    
}
else
{
    uint32_t len = 0;
    uint8_t des_id = object_id.id;
    if (cb_table[des_id].cb != NULL)
    {
        ucdr_deserialize_uint32_t(ub, &len);
        cb_table[des_id].cb(des_id, ub->iterator, (uint16_t)len);
    }
}
}
