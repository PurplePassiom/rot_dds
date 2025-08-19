#include "session_manager_config.h"
#include "session_manager.h"
#include "stream_storage_internal.h"
#include "output_best_effort_stream_internal.h"
#include "submessage_internal.h"

typedef struct
{
    bool isInited;
    uxrSession session;
    uxrStreamId output_stream;
    uxrStreamId input_stream;
    uxrCommunication *com;
}sessioin_fd_type;

static uxrStreamStorage streams;
static sessioin_fd_type fds[SESSION_NUMBER];

extern bool listen_message_reliably(uxrSession* session,int poll_ms);
static void session_manager_tx_indication(uint8_t protocol_id);
static void session_manager_rx_confirmation(uint8_t protocol_id);
#if 0
static osMessageQId dds_rx_event = NULL;
static osMessageQId dds_tx_event = NULL;
static osThreadId session_tx_thread = NULL;
static osThreadId session_rx_thread = NULL;

void sessioin_rx_function(void const* parm);
void sessioin_tx_function(void const* parm);
#endif
static uxrCommunication* session_manager_get_com(uint8_t protocol)
{
    uxrCommunication* com = NULL;
    switch (protocol)
    {
    case SESSION_PROTOCOL_SELF_COM:
        com = self_com_get_instance_info();
        break;
    case SESSION_PROTOCOL_UART_COM:
        // com = uart_tp_instance_info();
        break;
    case SESSION_PROTOCOL_CAN_COM:
        com = can_get_instance_info();
        break;
    default:
        com = NULL;
        break;
    }
    return com;
}

void session_manager_init(void)
{
    for (uint8_t i = 0; i < SESSION_NUMBER; i++)
    {
        session_data_type* session_data = &sessions[i];
        sessioin_fd_type *sessioin_fd = &fds[i];

        // init session fd
        if (session_data->init_func != NULL)
        {
            session_data->init_func();
        }
        else
        {
            continue;
        }

        sessioin_fd->com = session_manager_get_com(session_data->protocol);
        if ((sessioin_fd->com == NULL))
        {
            // printf("session_data->id:%d fd->com:%d session_id:%d\n",session_data->id,(int)&sessioin_fd->com, session_id);
        }
        else
        {
            /* registe rx confirmation and tx indication from tp moudle */
            if (sessioin_fd->com->comm_registe_txrx)
            {
                sessioin_fd->com->comm_registe_txrx(session_manager_tx_indication, session_manager_rx_confirmation,
                                                    i);
            }
            
            /* init output stream and input stream */
            sessioin_fd->output_stream = uxr_add_output_best_effort_buffer(
                                              &streams, //each channel has one output stream
                                              session_data->session_buffer, //each channel has own output buffer
                                              session_data->buffer_size,
                                              0//header offset
                                            );
            sessioin_fd->input_stream = uxr_add_input_best_effort_buffer(&streams);
            /* set callback */
            // uxr_set_topic_callback(&sessioin_fd->session, session_data->cb, NULL);
            sessioin_fd->isInited = true;
        }
    }
    #if 0
    if ((dds_rx_event == NULL) || (dds_tx_event == NULL))
    {
        osMessageQDef(session_event, 8, uint8_t);
        dds_rx_event = osMessageCreate(osMessageQ(session_event), NULL);

        osMessageQDef(dds_tx_event, 8, uint8_t);
        dds_tx_event = osMessageCreate(osMessageQ(dds_tx_event), NULL);
    }

    if ((session_tx_thread == NULL) || (session_rx_thread == NULL))
    {
        osThreadDef(session_tx_thread, sessioin_tx_function, osPriorityAboveNormal, 10, 512);
        session_tx_thread = osThreadCreate(osThread(session_tx_thread), NULL);

        osThreadDef(session_rx_thread_def, sessioin_rx_function, osPriorityAboveNormal, 9, 512);
        session_rx_thread = osThreadCreate(osThread(session_rx_thread_def), NULL);
    }
    #endif
}

uint8_t session_manager_get_protocolfd(uint8_t protocol)
{
    uint8_t protocol_fd = SESSION_PROTOCOL_INVALID;
    for (uint8_t i = 0; i < SESSION_NUMBER; i++)
    {
        if (sessions[i].protocol == protocol)
        {
            protocol_fd = i;
        }
    }
    return protocol_fd;
}

void session_set_on_topic(uint8_t expect_id, void (*session_callbakc)(uint8_t from_id, uint8_t to_id, uint8_t* data, uint16_t len) )
{
    if (expect_id > MAX_CALLBACK_NUM || SESSIONM_CB_TABLE[expect_id].cb != NULL || session_callbakc == NULL)
    {
        //ERROR
    }
    else
    {
        SESSIONM_CB_TABLE[expect_id].cb = session_callbakc;
    }
}

bool session_manager_send(uint8_t fd, uint8_t src_id, uint8_t des_id, uint8_t *data, uint16_t size)
{
    if (fd >= SESSION_NUMBER || data == NULL || !size || des_id > 0x0f)
    {
        return false;
    }
    sessioin_fd_type* session_fd = &fds[fd];
    UXR_LOCK_STREAM_ID(&streams, session_fd->output_stream);
    ucdrBuffer ub;
    uint8_t tempDestId = des_id;

    uxrOutputBestEffortStream* stream = uxr_get_output_best_effort_stream(&streams, session_fd->output_stream.index);
    bool available = stream && uxr_prepare_best_effort_buffer_to_write(stream, size+4, &ub);
    if (available)
    {
        ucdr_serialize_uint8_t(&ub, src_id);
        ucdr_serialize_uint8_t(&ub, tempDestId);
        ucdr_serialize_uint16_t(&ub, size);
        ucdr_serialize_array_uint8_t(&ub, data, size);
        uint8_t* buffer; size_t length; uxrSeqNum seq_num;

        if (uxr_prepare_best_effort_buffer_to_send(stream, &buffer, &length, &seq_num) && fds[fd].com->send_msg)
        {
            fds[fd].com->send_msg(fds[fd].com->instance, buffer, length);
        }
        UXR_UNLOCK_STREAM_ID(&streams, session_fd->output_stream);
    }
    else
    {
        UXR_UNLOCK_STREAM_ID(&streams, session_fd->output_stream);
    }
    return true;
}

static void session_manager_tx_indication(uint8_t protocol_id)
{
    (void)protocol_id;
}

static void session_manager_rx_confirmation(uint8_t protocol_id)
{
    uint8_t* data; size_t length;
    UXR_LOCK_STREAM_ID(&streams, fds[protocol_id].input_stream);
    bool received = fds[protocol_id].com->recv_msg(fds[protocol_id].com->instance, &data, &length, 0);
    if (received)
    {
        ucdrBuffer ub; uint8_t src; uint8_t des; uint8_t payload_len;
        ucdr_init_buffer(&ub, data, (uint32_t)length);
        ucdr_deserialize_uint8_t(&ub, (uint8_t*)&src);
        ucdr_deserialize_uint8_t(&ub, (uint8_t*)&des);
        ucdr_deserialize_array_uint8_t(&ub, (uint8_t*)&payload_len, 2);
        UXR_UNLOCK_STREAM_ID(&streams, fds[protocol_id].input_stream);
        sessions[protocol_id].cb(protocol_id, src, des, ub.iterator, (uint16_t)payload_len);
    }
    else
    {
        UXR_UNLOCK_STREAM_ID(&streams, fds[protocol_id].input_stream);
    }
}