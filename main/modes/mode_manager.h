#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum
{
    CMD_GET_MODES,
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

/* Builds a binary payload describing every configured mode (name + step
   config) from task_data, using the same little-endian wire format as the
   command packets. On success sets *out_buf to a heap buffer the caller must
   free() and *out_len to its length. Returns ESP_OK, ESP_ERR_INVALID_ARG, or
   ESP_ERR_NO_MEM. */
esp_err_t mode_manager_get(uint8_t** out_buf, size_t* out_len);

#endif /* MODE_MANAGER_H */
