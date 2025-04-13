#include "session_manager_config.h"
#include "session_manager.h"
#include "cmsis_os.h"

typedef struct
{
    bool isInited;
    uxrSession session;
    uxrStreamId output_stream;
    uxrStreamId input_stream;
    uxrCommunication *com;
}sessioin_fd_type;

static sessioin_fd_type fds[SESSION_NUMBER];
static osMessageQId dds_rx_event = NULL;
static osMessageQId dds_tx_event = NULL;
static osThreadId session_tx_thread = NULL;
static osThreadId session_rx_thread = NULL;

void sessioin_rx_function(void const* parm);
void sessioin_tx_function(void const* parm);

static uxrCommunication* sessionM_get_com(uint8_t protocol)
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

void sessionM_init(void)
{
    /* Tempory to init transport layer */
    // uart_transport_init();
    self_com_transport_init();
    can_transport_init();

    for (int i = 0; i < SESSION_NUMBER; i++)
    {
        session_data_type* session_data = &sessions[i];
        sessioin_fd_type *sessioin_fd = &fds[i];
        sessioin_fd->com = sessionM_get_com(session_data->protocol);
        if ((sessioin_fd->com == NULL))
        {
            // printf("session_data->id:%d fd->com:%d session_id:%d\n",session_data->id,(int)&sessioin_fd->com, session_id);
        }
        else
        {
            /* init sessions */
            uxr_init_session(&sessioin_fd->session, sessioin_fd->com, 0);
            /* init output stream and input stream */
            // for (size_t serid = 0; serid < MAX_SERVICE_ID; serid++)
            // {
            //     stream_buffer[serid].output_stream = uxr_create_output_best_effort_stream(&sessioin_fd->session, 
            //                                                                                stream_buffer[serid].stream_buffer, SESSION_BUFFER_SIZE);
            //     stream_buffer[serid].input_stream = uxr_create_input_best_effort_stream(&sessioin_fd->session);
            // }
            sessioin_fd->output_stream = uxr_create_output_best_effort_stream(&sessioin_fd->session,
                                                                               session_data->session_buffer,
                                                                               session_data->buffer_size);
            sessioin_fd->input_stream = uxr_create_input_best_effort_stream(&sessioin_fd->session);
            urx_update_session_id(&sessioin_fd->session, (uint8_t)(session_data->id|0x80));
            /* set callback */
            uxr_set_topic_callback(&sessioin_fd->session, session_data->cb, NULL);
            sessioin_fd->isInited = true;
        }
    }
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
}

uint8_t sessionM_get_protocolfd(uint8_t protocol)
{
    uint8_t protocol_fd = SESSION_PROTOCOL_INVALID;
    for (int i = 0; i < SESSION_NUMBER; i++)
    {
        if (sessions[i].protocol == protocol)
        {
            protocol_fd = i;
        }
    }
    return protocol_fd;
}

void session_set_on_topic(uint8_t expect_id, session_callbakc_type func)
{
    if (expect_id > MAX_CALLBACK_NUM || SESSIONM_CB_TABLE[expect_id].cb != NULL || func == NULL)
    {
        //ERROR
    }
    else
    {
        SESSIONM_CB_TABLE[expect_id].cb = func;
    }
}

bool session_manager_send(uint8_t fd, uint8_t des_id, uint8_t *data, uint16_t size)
{
    // bool res = false;
    if (fd >= SESSION_NUMBER || data == NULL || !size || des_id > 0x0f)
    {
        // printf("session_manager_send -1\n");
        return false;
    }
    ucdrBuffer ub;
    sessioin_fd_type* session_fd = &fds[fd];
    uxrObjectId object = {
        .id =des_id,
        .type = UXR_DATAREADER_ID
    };
    
    uxr_prepare_output_stream(&session_fd->session, session_fd->output_stream, object, &ub, size+4);//+4 for length
    // uxr_prepare_output_stream(&session_fd->session, stream_buffer[src_id].output_stream, object, &ub, size+8);
    // printf("uxr_prepare_output_stream:%d\n",request_id);
    ucdr_serialize_sequence_char(&ub, (const char*)data, size);

    osMessagePut(dds_tx_event, fd, 0);
    return true;
}

void sessioin_tx_function(void const * parm)
{
    for (;;)
    {
        uint8_t txFd;
        osEvent event = osMessageGet(dds_tx_event, osWaitForever);
        txFd = event.value.v;
        if (txFd < SESSION_NUMBER)
        {
            uxr_flash_output_streams(&fds[txFd].session);
        }
    }
}

extern bool listen_message_reliably(uxrSession* session,int poll_ms);
void sessioin_rx_function(void const * parm)
{
    for (;;)
    {
        uint8_t rxFd;
        osEvent event = osMessageGet(dds_rx_event, osWaitForever);
        rxFd = event.value.v;
        if (rxFd < SESSION_NUMBER)
        {
            listen_message_reliably(&fds[rxFd].session, 0);
        }
    }
}

void session_set_event(uint8_t id)
{
    osMessagePut(dds_rx_event, id, 0);
}
