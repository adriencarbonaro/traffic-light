#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include <stdint.h>

typedef enum
{
    CMD_SET_MODE,
    CMD_ADD_MODE,
    CMD_CUSTOM_MODE,
} command_id_t;

typedef struct
{
    char* mode_name;
    char* data;
} add_mode_event_t;

typedef struct
{
    char* mode_name;
} set_mode_event_t;

typedef struct
{
    command_id_t event_cmd_type;
    uint8_t* event_data;
    uint16_t event_data_len;
} mode_event_t;

void mode_manager_init(void);
void mode_manager_event(void* data, uint16_t data_len);

#endif /* MODE_MANAGER_H */
