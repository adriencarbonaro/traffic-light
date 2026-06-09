#include "mode_manager.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "mode_decoder.h"
#include "modes.h"

/* Defines ********************************************************************/

/* Static const objects *******************************************************/
static const char* TAG = "mode_manager";
static mode_event_t new_event = {0};
static QueueHandle_t mode_manager_queue = NULL;
static mode_t init_mode = {.name = "init_mode",
                           .nb_steps = 1,
                           .steps = {{.mask = 0x01, .duration = 1000},
                                     {.mask = 0x02, .duration = 1000},
                                     {.mask = 0x04, .duration = 1000}}};
static mode_t custom_mode;

static led_step_t steps_rx[MAX_STEPS] = {0};
static mode_t mode_rx = {0};

/* Static objects *************************************************************/

mode_manager_task_data_t task_data = {0};
TaskHandle_t mode_manager_task_handle = NULL;

/* Static functions ***********************************************************/

/* Public functions ***********************************************************/
esp_err_t mode_manager_set_active(const char* name, uint16_t name_len)
{
    for (int i = 0; i < task_data.count; i++)
    {
        ESP_LOGI(
            TAG,
            "Is it: %.*s, nb_steps=%u, step[0].mask=%02x, step[0].duration=%u",
            task_data.modes[i].name_len,
            task_data.modes[i].name,
            task_data.modes[i].nb_steps,
            task_data.modes[i].steps[0].mask,
            task_data.modes[i].steps[0].duration);
        if (strncmp(task_data.modes[i].name, (const char*)name, name_len) == 0)
        {
            task_data.active_mode = &task_data.modes[i];
            ESP_LOGI(TAG,
                     "Switching active mode for: %.*s, %u, %u",
                     task_data.active_mode->name_len,
                     task_data.active_mode->name,
                     task_data.active_mode->nb_steps,
                     task_data.active_mode->steps[0].duration);
            led_set(task_data.active_mode);
            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
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
             "Adding mode: %.*s, nb_steps=%u, mask[0]=%02x, duration[0]=%u",
             new_mode->name_len,
             new_mode->name,
             new_mode->nb_steps,
             new_mode->steps[0].mask,
             new_mode->steps[0].duration);

    memcpy(&task_data.modes[task_data.count], new_mode, sizeof(mode_t));

    ESP_LOGI(TAG,
             "Added mode: %.*s, nb_steps=%u, mask[0]=%02x, duration[0]=%u",
             task_data.modes[task_data.count].name_len,
             task_data.modes[task_data.count].name,
             task_data.modes[task_data.count].nb_steps,
             task_data.modes[task_data.count].steps[0].mask,
             task_data.modes[task_data.count].steps[0].duration);

    task_data.count++;

    return ESP_OK;
}

esp_err_t mode_manager_set_custom(const mode_t* custom)
{
    memcpy(&custom_mode, custom, sizeof(mode_t));

    task_data.active_mode = &custom_mode;

    return ESP_OK;
}

void mode_manager_task(void* arg)
{
    uint16_t table_len = 0;
    const mode_t* table = get_standard_mode_table(&table_len);

    for (uint16_t i = 0; i < table_len; i++)
    {
        mode_manager_add(&table[i]);
    }

    ESP_LOGI(TAG, "sending");
    led_set(&table[0]);

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
                case CMD_ADD_MODE:
                {
                    ESP_LOGI(TAG, "CMD_ADD_MODE");
                    mode_manager_add(&mode_rx);
                    break;
                }
                case CMD_SET_MODE:
                {
                    ESP_LOGI(TAG, "CMD_SET_MODE");
                    mode_manager_set_active(mode_rx.name, mode_rx.name_len);
                    break;
                }
                case CMD_CUSTOM_MODE:
                {
                    ESP_LOGI(TAG, "CMD_CUSTOM_MODE");
                    mode_manager_set_custom(&mode_rx);
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

    task_data.active_mode = &init_mode;

    mode_manager_queue = xQueueCreate(10, sizeof(mode_event_t));

    xTaskCreate(mode_manager_task,
                "mode_manager_task",
                8192,
                NULL,
                tskIDLE_PRIORITY,
                NULL);
}
