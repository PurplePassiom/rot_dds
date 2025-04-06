#ifndef _CANCOM_TRANSPORT_H_
#define _CANCOM_TRANSPORT_H_
#ifdef __cplusplus
extern "C"
{
#endif // ifdef __cplusplus

#include <stdbool.h>

bool can_transport_init(void);

bool can_transport_close(
    void* transport);

void* can_get_instance_info(void);

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif // _CANCOM_TRANSPORT_H_