#ifndef _UART_TRANSPORT_H_
#define _UART_TRANSPORT_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>


bool uart_transport_init(void);

void* uart_tp_instance_info(void);
#ifdef __cplusplus
}
#endif
#endif