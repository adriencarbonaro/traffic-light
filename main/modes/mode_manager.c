#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "types.h"

#include "mode_manager.h"

/* Defines ********************************************************************/

typedef enum {
    MODE_INIT,
} mode_t;

typedef struct {
    mode_t mode;
} mode_manager_task_data_t;

/* Static const objects *******************************************************/
static const char* TAG = "mode_manager";
static mode_event_t new_event = {0};
static QueueHandle_t mode_manager_queue = NULL;

/* Static objects *************************************************************/

mode_manager_task_data_t task_data = {0};
TaskHandle_t mode_manager_task_handle = NULL;

/* Static functions ***********************************************************/

/* Public functions ***********************************************************/

void mode_manager_task(void* arg)
{
    while(1)
    {
        ESP_LOGI(TAG, "run");

        if (xQueueReceive(mode_manager_queue, &new_event, pdMS_TO_TICKS(200)))
        {
            switch (new_event.event_id)
            {
                case MODE_EVENT_ADD:
                {
                    ESP_LOGI(TAG, "MODE_EVENT_ADD, event_id=%u, event_data=%.*s", new_event.event_id, new_event.event_data_len, new_event.event_data);
                    break;
                }
                case MODE_EVENT_SET:
                {
                    ESP_LOGI(TAG, "MODE_EVENT_SET, event_id=%u, event_data=%.*s", new_event.event_id, new_event.event_data_len, new_event.event_data);
                    break;
                }
                default:
                {
                    ESP_LOGI(TAG, "unknown mode event");
                    break;
                }
            }
            // change_mode(mode);
        }
    }
}

void mode_manager_event(mode_event_id id, void* data, uint16 data_len)
{
    mode_event_t event;
    event.event_id = id;
    event.event_data = (char*)malloc(data_len * sizeof(char));
    if (!event.event_data)
    {
        ESP_LOGE(TAG, "failed to allocate memory");
        return;
    }
    memcpy(event.event_data, (char*)data, data_len);
    event.event_data_len = data_len;
    xQueueSend(mode_manager_queue, &event, 0);
}

void mode_manager_init(void)
{
    memset(&task_data, 0, sizeof(mode_manager_task_data_t));

    task_data.mode = MODE_INIT;

    mode_manager_queue = xQueueCreate(10, sizeof(mode_event_t));

    xTaskCreate(mode_manager_task,
                "mode_manager_task",
                8192,
                NULL,
                tskIDLE_PRIORITY,
                NULL);
}
