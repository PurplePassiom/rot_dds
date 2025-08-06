#ifndef _CANCOM_TRANSPORT_H_
#define _CANCOM_TRANSPORT_H_
#ifdef __cplusplus
extern "C"
{
#endif // ifdef __cplusplus

#include <stdbool.h>
#include <stdint.h>

bool can_transport_init(void);

bool can_transport_close(
    void* transport);

void* can_get_instance_info(void);

void receive_can_frame(uint32_t canid, uint8_t *data, uint8_t len)

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif // _CANCOM_TRANSPORT_H_