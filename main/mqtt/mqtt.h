#ifndef MQTT_H_
#define MQTT_H_

#include "mqtt_client.h"
#include "types.h"

typedef void (*handler_t)(const char* msg, uint16 msg_len);

typedef struct {
    const char* topic;
    handler_t handler;
} mqtt_config_t;

void mqtt_init(void);

void send_state(const char* state_str);

const mqtt_config_t* get_mqtt_config_table(void);
uint16 get_mqtt_config_table_size(void);

#endif /* MQTT_H_ */
