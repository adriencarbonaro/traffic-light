#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "types.h"

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

/* Static functions ***********************************************************/

static void change_mode(uint32 msg)
{
    ESP_LOGI(TAG, "changing mode: %u", msg);
}

/* Public functions ***********************************************************/

void mode_manager_task(void* arg)
{
    while(1)
    {
        /* Add mode */

        /* Change mode */
        uint32 msg;
        if (xTaskNotifyWait(0, 0xffffffff, &msg, pdMS_TO_TICKS(50)))
        {
            change_mode(msg);
        }
    }
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
                NULL);
}
