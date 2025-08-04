#ifndef _SESSION_MANAGER_CONFIG_H
#define _SESSION_MANAGER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif
#include <client.h>
#include <microcdr.h>

#include "selfcom_transport.h"
// #include "communication.h"
// #include "uart_tansport.h"
// #include "cancom_tansport.h"

#define MAX_SERVICE_ID 0x03u
/* session protocols definition */
#define SESSION_PROTOCOL_SELF_COM   1u
#define SESSION_PROTOCOL_UART_COM   2u
#define SESSION_PROTOCOL_CAN_COM    3u
#define SESSION_BUFFER_SIZE         256u
#define SESSION_PROTOCOL_INVALID    0xFFu

#define SESSION_NUMBER              3u

#define MAX_CALLBACK_NUM            0xFu

typedef void (*session_callbakc_func)(uint8_t src_id, uint8_t to_id, uint8_t* data, uint16_t len);
typedef bool (*session_init_func)(void);

typedef struct
{
    uint8_t id;
    uint8_t protocol;
    session_init_func init_func;
    uint8_t* session_buffer;
    uint16_t buffer_size;
    uxrOnTopicFunc cb;
    // tx_indication tp_tx_indication;
    // rx_confirmation tp_rx_confirmation;
}session_data_type;

typedef struct
{
    session_callbakc_func cb;
}session_manager_callback_type;

typedef struct
{
    uxrStreamId output_stream;
    uxrStreamId input_stream;
    uint8_t stream_buffer[SESSION_BUFFER_SIZE];
}session_stream_buffer_type;


extern session_data_type sessions[SESSION_NUMBER];

extern session_manager_callback_type cb_table[MAX_CALLBACK_NUM];

// extern session_stream_buffer_type stream_buffer[MAX_SERVICE_ID];
#define SESSIONM_CB_TABLE           cb_table
#ifdef __cplusplus
}
#endif
#endif