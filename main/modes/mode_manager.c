#include "mode_manager.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "mode_decoder.h"
#include "modes.h"
#include "nvs.h"

/* Defines ********************************************************************/

/* Static const objects *******************************************************/
static const char* TAG = "mode_manager";
static mode_event_t new_event = {0};
static QueueHandle_t mode_manager_queue = NULL;
static mode_t custom_mode;

static led_step_t steps_rx[MAX_STEPS] = {0};
static mode_t mode_rx = {0};

/* Static objects *************************************************************/

mode_manager_task_data_t task_data = {0};
TaskHandle_t mode_manager_task_handle = NULL;

/* Static functions ***********************************************************/

static inline size_t write_uint8(uint8_t* dst, uint8_t v)
{
    dst[0] = v;
    return sizeof(uint8_t);
}

static inline size_t write_uint16(uint8_t* dst, uint16_t v)
{
    dst[0] = (uint8_t)(v & 0xff);
    dst[1] = (uint8_t)((v >> 8) & 0xff);
    return sizeof(uint16_t);
}

static inline size_t write_uint32(uint8_t* dst, uint32_t v)
{
    dst[0] = (uint8_t)(v & 0xff);
    dst[1] = (uint8_t)((v >> 8) & 0xff);
    dst[2] = (uint8_t)((v >> 16) & 0xff);
    dst[3] = (uint8_t)((v >> 24) & 0xff);
    return sizeof(uint32_t);
}

/* Persistence (NVS) **********************************************************/

#define MODES_NVS_NS "tl_modes"
#define MODES_KEY_SCHEMA "schema"
#define MODES_KEY_COUNT "count"
#define MODES_KEY_TABLE "table"
#define MODES_KEY_ACTIVE "active"
#define MODES_ACTIVE_NONE 0xFF

/* Index of the active mode within the table, or MODES_ACTIVE_NONE when the
   active mode isn't a stored one (custom / unset). */
static uint8_t active_index(void)
{
    if (task_data.count > 0 && task_data.active_mode >= &task_data.modes[0] &&
        task_data.active_mode <= &task_data.modes[task_data.count - 1])
    {
        return (uint8_t)(task_data.active_mode - &task_data.modes[0]);
    }
    return MODES_ACTIVE_NONE;
}

/* Persists the whole mode table (+ active index) to flash. Called after the
   table changes (add / edit / delete). The schema key stores sizeof(mode_t) so
   a future layout change is detected on load and triggers a clean re-seed. */
static void modes_save(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(MODES_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "nvs_open (save) failed: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_u32(h, MODES_KEY_SCHEMA, (uint32_t)sizeof(mode_t));
    if (err == ESP_OK) err = nvs_set_u8(h, MODES_KEY_COUNT, task_data.count);
    if (err == ESP_OK && task_data.count > 0)
    {
        err = nvs_set_blob(h,
                           MODES_KEY_TABLE,
                           task_data.modes,
                           (size_t)task_data.count * sizeof(mode_t));
    }
    if (err == ESP_OK) err = nvs_set_u8(h, MODES_KEY_ACTIVE, active_index());
    if (err == ESP_OK) err = nvs_commit(h);

    if (err != ESP_OK)
        ESP_LOGW(TAG, "saving modes failed: %s", esp_err_to_name(err));
    else
        ESP_LOGI(TAG, "saved %u mode(s) to flash", task_data.count);

    nvs_close(h);
}

/* Persists only the active-mode index — cheap path for set / next / custom,
   which don't touch the table. */
static void active_save(void)
{
    nvs_handle_t h;
    if (nvs_open(MODES_NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    if (nvs_set_u8(h, MODES_KEY_ACTIVE, active_index()) == ESP_OK)
    {
        nvs_commit(h);
    }
    nvs_close(h);
}

/* Restores the mode table from flash into task_data. Returns ESP_OK if a
   compatible table was found (even empty — meaning the user deleted all of
   them); otherwise an error, with task_data left zeroed. */
static esp_err_t modes_load(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(MODES_NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err; /* ESP_ERR_NVS_NOT_FOUND on first boot */

    uint32_t schema = 0;
    err = nvs_get_u32(h, MODES_KEY_SCHEMA, &schema);
    if (err != ESP_OK || schema != (uint32_t)sizeof(mode_t))
    {
        ESP_LOGW(TAG, "no compatible persisted modes (schema mismatch)");
        nvs_close(h);
        return ESP_ERR_INVALID_VERSION;
    }

    uint8_t count = 0;
    err = nvs_get_u8(h, MODES_KEY_COUNT, &count);
    if (err != ESP_OK)
    {
        nvs_close(h);
        return err;
    }
    if (count > MAX_MODES) count = MAX_MODES;

    if (count > 0)
    {
        size_t expected = (size_t)count * sizeof(mode_t);
        size_t len = expected;
        err = nvs_get_blob(h, MODES_KEY_TABLE, task_data.modes, &len);
        if (err != ESP_OK || len != expected)
        {
            ESP_LOGW(TAG,
                     "persisted table unreadable: %s",
                     esp_err_to_name(err));
            memset(&task_data, 0, sizeof(task_data));
            nvs_close(h);
            return ESP_FAIL;
        }
    }
    task_data.count = count;

    uint8_t active = MODES_ACTIVE_NONE;
    nvs_get_u8(h, MODES_KEY_ACTIVE, &active); /* optional */
    if (active != MODES_ACTIVE_NONE && active < task_data.count)
        task_data.active_mode = &task_data.modes[active];
    else
        task_data.active_mode =
            task_data.count > 0 ? &task_data.modes[0] : NULL;

    nvs_close(h);
    ESP_LOGI(TAG,
             "restored %u mode(s) from flash (active=%u)",
             task_data.count,
             active);
    return ESP_OK;
}

/* Public functions ***********************************************************/
esp_err_t mode_manager_set_active(const char* name, uint16_t name_len)
{
    for (int i = 0; i < task_data.count; i++)
    {
        ESP_LOGI(
            TAG,
            "Is it: %.*s, nb_steps=%u, step[0].mask=%02x, step[0].duration=%lu",
            task_data.modes[i].name_len,
            task_data.modes[i].name,
            task_data.modes[i].nb_steps,
            task_data.modes[i].steps[0].mask,
            (unsigned long)task_data.modes[i].steps[0].duration);
        if (strncmp(task_data.modes[i].name, (const char*)name, name_len) == 0)
        {
            task_data.active_mode = &task_data.modes[i];
            ESP_LOGI(TAG,
                     "Switching active mode for: %.*s, %u, %lu",
                     task_data.active_mode->name_len,
                     task_data.active_mode->name,
                     task_data.active_mode->nb_steps,
                     (unsigned long)task_data.active_mode->steps[0].duration);
            led_set(task_data.active_mode);
            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t mode_manager_get(uint8_t** out_buf, size_t* out_len)
{
    if (out_buf == NULL || out_len == NULL) return ESP_ERR_INVALID_ARG;

    /* Upper bound: a uint16 mode count, then for each mode its name_len(2) +
       name(MAX_NAME_LEN) + loop(1) + nb_steps(2) and up to MAX_STEPS steps,
       each a mask(1) + duration(4). All fields little-endian. */
    size_t cap =
        sizeof(uint16_t) + (size_t)task_data.count *
                               (2 + MAX_NAME_LEN + 1 + 2 + MAX_STEPS * (1 + 4));
    uint8_t* buf = (uint8_t*)malloc(cap);
    if (buf == NULL)
    {
        ESP_LOGE(TAG,
                 "failed to allocate %u bytes for modes payload",
                 (unsigned)cap);
        return ESP_ERR_NO_MEM;
    }

    size_t off = 0;
    off += write_uint16(&buf[off], task_data.count);

    for (int i = 0; i < task_data.count; i++)
    {
        const mode_t* m = &task_data.modes[i];
        uint16_t name_len =
            m->name_len > MAX_NAME_LEN ? MAX_NAME_LEN : m->name_len;

        off += write_uint16(&buf[off], name_len);
        memcpy(&buf[off], m->name, name_len);
        off += name_len;
        off += write_uint8(&buf[off], (uint8_t)m->loop);
        off += write_uint16(&buf[off], m->nb_steps);

        for (uint16_t s = 0; s < m->nb_steps; s++)
        {
            off += write_uint8(&buf[off], m->steps[s].mask);
            off += write_uint32(&buf[off], m->steps[s].duration);
        }
    }

    *out_buf = buf;
    *out_len = off;
    return ESP_OK;
}

esp_err_t mode_manager_add(const mode_t* new_mode)
{
    if (task_data.count >= MAX_MODES) return ESP_ERR_NO_MEM;

    for (int i = 0; i < task_data.count; i++)
    {
        if (strncmp(task_data.modes[i].name, new_mode->name, MAX_NAME_LEN) == 0)
        {
            return ESP_ERR_INVALID_STATE;
        }
    }

    ESP_LOGI(TAG,
             "Adding mode: %.*s, nb_steps=%u, mask[0]=%02x, duration[0]=%lu",
             new_mode->name_len,
             new_mode->name,
             new_mode->nb_steps,
             new_mode->steps[0].mask,
             (unsigned long)new_mode->steps[0].duration);

    memcpy(&task_data.modes[task_data.count], new_mode, sizeof(mode_t));

    ESP_LOGI(TAG,
             "Added mode: %.*s, nb_steps=%u, mask[0]=%02x, duration[0]=%lu",
             task_data.modes[task_data.count].name_len,
             task_data.modes[task_data.count].name,
             task_data.modes[task_data.count].nb_steps,
             task_data.modes[task_data.count].steps[0].mask,
             (unsigned long)task_data.modes[task_data.count].steps[0].duration);

    task_data.count++;

    return ESP_OK;
}

esp_err_t mode_manager_get_active(uint8_t** out_buf, size_t* out_len)
{
    if (out_buf == NULL || out_len == NULL) return ESP_ERR_INVALID_ARG;

    const mode_t* m = task_data.active_mode;
    uint16_t name_len = 0;
    if (m != NULL)
    {
        name_len = m->name_len > MAX_NAME_LEN ? MAX_NAME_LEN : m->name_len;
    }

    uint8_t* buf = (uint8_t*)malloc(sizeof(uint16_t) + name_len);
    if (buf == NULL) return ESP_ERR_NO_MEM;

    size_t off = 0;
    off += write_uint16(&buf[off], name_len);
    if (m != NULL && name_len > 0)
    {
        memcpy(&buf[off], m->name, name_len);
        off += name_len;
    }

    *out_buf = buf;
    *out_len = off;
    return ESP_OK;
}

esp_err_t mode_manager_delete(const char* name, uint16_t name_len)
{
    (void)name_len;

    for (int i = 0; i < task_data.count; i++)
    {
        if (strncmp(task_data.modes[i].name, name, MAX_NAME_LEN) != 0) continue;

        /* Track the active mode by index so we can re-point it after the
           in-place shift (the pointer alone would silently follow the slot). */
        int active_idx = -1;
        if (task_data.count > 0 &&
            task_data.active_mode >= &task_data.modes[0] &&
            task_data.active_mode <= &task_data.modes[task_data.count - 1])
        {
            active_idx = (int)(task_data.active_mode - &task_data.modes[0]);
        }

        ESP_LOGI(TAG,
                 "Deleting mode: %.*s (slot %d)",
                 task_data.modes[i].name_len,
                 task_data.modes[i].name,
                 i);

        for (int j = i; j < task_data.count - 1; j++)
        {
            task_data.modes[j] = task_data.modes[j + 1];
        }
        task_data.count--;
        memset(&task_data.modes[task_data.count], 0, sizeof(mode_t));

        if (active_idx == i)
        {
            /* The active mode was removed: fall back to the first remaining. */
            task_data.active_mode =
                task_data.count > 0 ? &task_data.modes[0] : NULL;
            if (task_data.active_mode) led_set(task_data.active_mode);
        }
        else if (active_idx > i)
        {
            task_data.active_mode = &task_data.modes[active_idx - 1];
        }

        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t mode_manager_edit(const mode_t* new_mode)
{
    for (int i = 0; i < task_data.count; i++)
    {
        if (strncmp(task_data.modes[i].name, new_mode->name, MAX_NAME_LEN) != 0)
        {
            continue;
        }

        ESP_LOGI(TAG,
                 "Editing mode: %.*s, nb_steps=%u",
                 task_data.modes[i].name_len,
                 task_data.modes[i].name,
                 new_mode->nb_steps);

        task_data.modes[i].loop = new_mode->loop;
        task_data.modes[i].nb_steps = new_mode->nb_steps;
        memcpy(task_data.modes[i].steps,
               new_mode->steps,
               sizeof(task_data.modes[i].steps));

        if (task_data.active_mode == &task_data.modes[i])
        {
            led_set(task_data.active_mode);
        }

        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t mode_manager_set_custom(const mode_t* custom)
{
    memcpy(&custom_mode, custom, sizeof(mode_t));

    task_data.active_mode = &custom_mode;

    return ESP_OK;
}

/* Advances the active mode to the next one in the table, wrapping around. If
   the active mode isn't part of the table (e.g. custom / unset), starts at the
   first. Runs on the mode_manager task, so task_data is touched from a single
   context. */
static void mode_manager_next(void)
{
    if (task_data.count == 0) return;

    int idx = -1;
    if (task_data.active_mode >= &task_data.modes[0] &&
        task_data.active_mode <= &task_data.modes[task_data.count - 1])
    {
        idx = (int)(task_data.active_mode - &task_data.modes[0]);
    }

    int next = (idx + 1) % task_data.count; /* idx == -1 -> 0 */
    task_data.active_mode = &task_data.modes[next];

    ESP_LOGI(TAG,
             "Next mode: %.*s (slot %d)",
             task_data.active_mode->name_len,
             task_data.active_mode->name,
             next);

    led_set(task_data.active_mode);
}

void mode_manager_task(void* arg)
{
    if (modes_load() != ESP_OK)
    {
        /* First boot (or incompatible saved data): seed with the standard
           modes and persist them. */
        ESP_LOGI(TAG, "no saved modes, seeding standard table");
        uint16_t table_len = 0;
        const mode_t* table = get_standard_mode_table(&table_len);
        for (uint16_t i = 0; i < table_len; i++)
        {
            mode_manager_add(&table[i]);
        }
        /* Point the active mode at the first entry so it matches the lamps —
           otherwise the first button click only re-syncs the pointer. */
        if (task_data.count > 0) task_data.active_mode = &task_data.modes[0];
        modes_save();
    }

    /* Reflect the active mode on the lamps (stays off if the table is empty).
     */
    if (task_data.active_mode != NULL) led_set(task_data.active_mode);

    while (1)
    {
        if (xQueueReceive(mode_manager_queue, &new_event, pdMS_TO_TICKS(600)))
        {
            memset(&mode_rx, 0, sizeof(mode_t));
            parse_command(new_event.event_data,
                          new_event.event_data_len,
                          &new_event.event_cmd_type,
                          &mode_rx);

            switch (new_event.event_cmd_type)
            {
                case CMD_GET_MODES:
                {
                    /* Modes are served synchronously by the GET /get_modes
                       HTTP endpoint, which reads task_data directly. Nothing
                       to do here. */
                    ESP_LOGI(TAG, "CMD_GET_MODES (served via /get_modes)");
                    break;
                }
                case CMD_ADD_MODE:
                {
                    ESP_LOGI(TAG, "CMD_ADD_MODE");
                    if (mode_manager_add(&mode_rx) == ESP_OK) modes_save();
                    break;
                }
                case CMD_SET_MODE:
                {
                    ESP_LOGI(TAG, "CMD_SET_MODE");
                    if (mode_manager_set_active(mode_rx.name,
                                                mode_rx.name_len) == ESP_OK)
                        active_save();
                    break;
                }
                case CMD_CUSTOM_MODE:
                {
                    ESP_LOGI(TAG, "CMD_CUSTOM_MODE");
                    mode_manager_set_custom(&mode_rx);
                    active_save();
                    break;
                }
                case CMD_DELETE_MODE:
                {
                    ESP_LOGI(TAG, "CMD_DELETE_MODE");
                    if (mode_manager_delete(mode_rx.name, mode_rx.name_len) ==
                        ESP_OK)
                        modes_save();
                    break;
                }
                case CMD_EDIT_MODE:
                {
                    ESP_LOGI(TAG, "CMD_EDIT_MODE");
                    if (mode_manager_edit(&mode_rx) == ESP_OK) modes_save();
                    break;
                }
                case CMD_NEXT_MODE:
                {
                    ESP_LOGI(TAG, "CMD_NEXT_MODE");
                    mode_manager_next();
                    active_save();
                    break;
                }
                default:
                {
                    ESP_LOGI(TAG, "unknown mode event");
                    break;
                }
            }
            // change_mode(mode);
            free(new_event.event_data);
        }
    }
}

void mode_manager_event(void* data, uint16_t data_len)
{
    mode_event_t event;
    event.event_data = (uint8_t*)malloc(data_len * sizeof(char));
    if (!event.event_data)
    {
        ESP_LOGE(TAG, "failed to allocate memory");
        return;
    }
    memcpy(event.event_data, data, data_len);
    event.event_data_len = data_len;
    xQueueSend(mode_manager_queue, &event, 0);
}

void mode_manager_init(void)
{
    memset(&task_data, 0, sizeof(mode_manager_task_data_t));

    /* active_mode stays NULL until the task loads the standard modes. If no
       mode is available, the lamps simply stay off. */
    mode_manager_queue = xQueueCreate(10, sizeof(mode_event_t));

    xTaskCreate(mode_manager_task,
                "mode_manager_task",
                8192,
                NULL,
                tskIDLE_PRIORITY,
                NULL);
}
