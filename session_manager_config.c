#include "session_manager_config.h"
#include "selfcom_transport.h"

session_data_type sessions[SESSION_NUMBER] = {
    {.id = 0x81, .protocol = SESSION_PROTOCOL_SELF_COM}
};