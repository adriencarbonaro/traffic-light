#include "esp_log.h"
#include "freertos/FreeRTOS.h"
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

/* Static objects *************************************************************/

mode_manager_task_data_t task_data = {0};
TaskHandle_t mode_manager_task_handle = NULL;

/* Static functions ***********************************************************/

static void change_mode(mode_t msg)
{
    ESP_LOGI(TAG, "changing mode: %u", msg);
}

/* Public functions ***********************************************************/

void mode_manager_task(void* arg)
{
    while(1)
    {
        ESP_LOGI(TAG, "run");

        if (xTaskNotifyWait(0, 0xffffffff, &msg, pdMS_TO_TICKS(200)))
        {
            mode_event_t* event = (mode_event_t*)msg;
            switch (event->event_id)
            {
                case MODE_EVENT_ADD:
                {
                    ESP_LOGI(TAG, "MODE_EVENT_ADD");
                    break;
                }
                case MODE_EVENT_SET:
                {
                    ESP_LOGI(TAG, "MODE_EVENT_SET");
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

void mode_manager_event(mode_event_id id, void *data, uint16 data_len)
{
    struct { mode_event_id id; void* data; } event;
    event.id = id;
    memcpy(event.data, data, data_len);
    xTaskNotify(mode_manager_task_handle, &event, eSetValueWithOverwrite);
}

void mode_manager_init(void)
{
    memset(&task_data, 0, sizeof(mode_manager_task_data_t));

    task_data.mode = MODE_INIT;

    xTaskCreate(mode_manager_task,
                "mode_manager_task",
                8192,
                NULL,
                tskIDLE_PRIORITY,
                &mode_manager_task_handle);
}
