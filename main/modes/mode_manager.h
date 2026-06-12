#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/* Forward-declare the tag only (no `mode_t` typedef) so this header stays
   includable from TUs that also see POSIX <sys/types.h> mode_t. The full
   definition lives in modes.h. */
struct mode_t;

typedef enum
{
    CMD_GET_MODES,
    CMD_SET_MODE,
    CMD_ADD_MODE,
    CMD_CUSTOM_MODE,
    CMD_DELETE_MODE,
    CMD_EDIT_MODE,
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

/* Builds a binary payload describing the currently active mode: just its name,
   length-prefixed as [ name_len : uint16 LE ] [ name ]. If no mode is active,
   name_len is 0. On success sets *out_buf to a heap buffer the caller must
   free() and *out_len to its length. Returns ESP_OK, ESP_ERR_INVALID_ARG, or
   ESP_ERR_NO_MEM. */
esp_err_t mode_manager_get_active(uint8_t** out_buf, size_t* out_len);

/* Removes the mode whose name matches (name, name_len) from task_data, shifting
   the tail down. If it was the active mode, the active pointer is reset to the
   first remaining mode (or NULL). Returns ESP_OK or ESP_ERR_NOT_FOUND. */
esp_err_t mode_manager_delete(const char* name, uint16_t name_len);

/* Replaces the step config (loop / nb_steps / steps) of the existing mode whose
   name matches new_mode->name, keeping its slot. Returns ESP_OK or
   ESP_ERR_NOT_FOUND.
   Uses the `struct mode_t` tag (not a typedef) so this header can be included by
   translation units that also pull in POSIX <sys/types.h> mode_t (e.g. the HTTP
   server) without a name clash. */
esp_err_t mode_manager_edit(const struct mode_t* new_mode);

#endif /* MODE_MANAGER_H */
