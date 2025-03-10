#ifndef _SESSION_MANAGER_CONFIG_H
#define _SESSION_MANAGER_CONFIG_H

#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include "selfcom_transport.h"

/* session protocols definition */
#define SESSION_PROTOCOL_SELF_COM   1u

#define SESSION_NUMBER      1u

typedef struct
{
    uint8_t id;
    uint8_t protocol;
}session_data_type;

extern session_data_type sessions[SESSION_NUMBER];

#endif