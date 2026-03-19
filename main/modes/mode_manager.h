#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include "types.h"

typedef enum
{
    MODE_EVENT_SET,
    MODE_EVENT_ADD,
} mode_event_id;

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
    mode_event_id event_id;
    char* event_data;
    uint16_t event_data_len;
} mode_event_t;

void mode_manager_init(void);
void mode_manager_event(mode_event_id id, void* data, uint16 data_len);

#endif /* MODE_MANAGER_H */
