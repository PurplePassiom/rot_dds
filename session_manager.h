#ifndef _SESSION_MANAGER_H_
#define _SESSION_MANAGER_H_
#include <stdint.h>

void sessioin_excution_function(void);

void session_set_on_topic(int fd, uxrOnTopicFunc func);

int session_manager_init(uint8_t id, uint8_t* output_buff, uint16_t buffer_size);

bool session_manager_send(int fd, uint8_t des_id, uint8_t *data, uint16_t size);
#endif