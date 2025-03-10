#include "session_manager_config.h"
#include "session_manager.h"

typedef struct
{
    bool isInited;
    uxrSession session;
    uxrStreamId output_stream;
    uxrStreamId input_stream;
    uxrCommunication *com;
}sessioin_fd_type;

static sessioin_fd_type fds[SESSION_NUMBER];

static uxrCommunication* get_com(uint8_t protocol)
{
    uxrCommunication* com;
    switch (protocol)
    {
    case SESSION_PROTOCOL_SELF_COM:
        com = self_com_get_instance_info();
        break;
    
    default:
        com = NULL;
        break;
    }
    return com;
}

int session_manager_init(uint8_t id, uint8_t* output_buff, uint16_t buffer_size)
{
    int fd = -1;
    int i;
    if (output_buff == NULL || id >= 0x80 || !buffer_size)
    {
        // printf("output_buff:%x id:%d buffer_size:%d\n",(int)output_buff,id, buffer_size);
        return -1;
    }
    uint8_t session_id = (id | 0x80);
    for ( i = 0; i < SESSION_NUMBER; i++)
    {
        session_data_type* session_data = &sessions[i];
        sessioin_fd_type *sessioin_fd = &fds[i];
        sessioin_fd->com = get_com(SESSION_PROTOCOL_SELF_COM);
        if ((session_id != session_data->id) || (sessioin_fd->com == NULL))
        {
            // printf("session_data->id:%d fd->com:%d session_id:%d\n",session_data->id,(int)&sessioin_fd->com, session_id);
        }
        else
        {
            /* init sessions */
            uxr_init_session(&sessioin_fd->session, sessions[i].id, sessioin_fd->com, 0);
            /* init output stream and input stream */
            sessioin_fd->output_stream = uxr_create_output_best_effort_stream(&sessioin_fd->session, output_buff, buffer_size);
            sessioin_fd->input_stream = uxr_create_input_best_effort_stream(&sessioin_fd->session);
            fd = i;
            sessioin_fd->isInited = true;
            break;
        }
    }
    return fd;
}

void session_set_on_topic(int fd, uxrOnTopicFunc func)
{
    uint32_t icount = 0;
    if (fd < SESSION_NUMBER)
    {
        uxr_set_topic_callback(&fds[fd].session, func, &icount);
    }
}

bool session_manager_send(int fd, uint8_t des_id, uint8_t *data, uint16_t size)
{
    // bool res = false;
    if (fd >= SESSION_NUMBER || data == NULL || !size || des_id > 0x0f)
    {
        printf("session_manager_send -1\n");
        return false;
    }
    ucdrBuffer ub;
    sessioin_fd_type* session_fd = &fds[fd];
    uxrObjectId object = {
        .id =des_id,
        .type = UXR_DATAREADER_ID
    };
    uint16_t request_id = uxr_prepare_output_stream(&session_fd->session, session_fd->output_stream, object, &ub, size+4);
    // printf("uxr_prepare_output_stream:%d\n",request_id);
    ucdr_serialize_sequence_char(&ub, (const char*)data, size);
    // ucdr_serialize_string(&ub,(const char*) data);
    return true;
}

void sessioin_excution_function(void)
{
    for (int i = 0; i < SESSION_NUMBER; i++)
    {
        if (fds[i].isInited == true)
        {
            uxr_run_session_time(&fds[i].session, 1000);
        }
    }
}