#ifndef _SESSION_MANAGER_H_
#define _SESSION_MANAGER_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
// #include "session_manager_config.h"
// #include <uxr/client/core/session/session.h>
typedef void (*session_callbakc_type)(uint8_t to_id, uint8_t* data, uint16_t len);

void sessionM_init(void);

uint8_t sessionM_get_protocolfd(uint8_t protocol);

void session_set_on_topic(uint8_t expect_id, session_callbakc_type func);

bool session_manager_send(uint8_t fd, uint8_t src_id, uint8_t des_id, uint8_t *data, uint16_t size);

void session_set_event(uint8_t id);

#ifdef __cplusplus
}
#endif
#endif