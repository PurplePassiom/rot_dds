#ifndef SELFCOM_TRANSPORT_H_
#define SELFCOM_TRANSPORT_H_
#ifdef __cplusplus
extern "C"
{
#endif // ifdef __cplusplus

#include <stdbool.h>

bool self_com_transport_init(void);

bool self_com_transport_close(
        void* transport);

void* self_com_get_instance_info(void);
#ifdef __cplusplus
}
#endif // ifdef __cplusplus
#endif