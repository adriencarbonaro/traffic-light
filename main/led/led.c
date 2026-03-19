#include "led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "string.h"
#include "time.h"

typedef struct
{
    layout_t current_layout;
    uint16_t stage_index;
    uint32_t stage_start_time;
} task_data_t;

static task_data_t task_data = {0};
static layout_t new_layout = {0};
static QueueHandle_t led_queue = NULL;

static const char* TAG = "led_task";

void clear_layout(layout_t* layout)
{
    layout->loop = false;
    layout->name = NULL;
    for (uint16_t i = 0; i < layout->nb_stages; i++)
    {
        memset(&layout->stages[i], 0, sizeof(layout_stage_t));
    }
    layout->nb_stages = 0;
}

void run_layout()
{
    layout_t current_layout = task_data.current_layout;
    layout_stage_t stage = current_layout.stages[task_data.stage_index];

    if ((xTaskGetTickCount() - task_data.stage_start_time) >= stage.duration_ms)
    {
        task_data.stage_index++;

        if (task_data.stage_index >= current_layout.nb_stages)
        {
            if (current_layout.loop)
                task_data.stage_index = 0;
            else
                clear_layout(&task_data.current_layout);
        }

        task_data.stage_start_time = xTaskGetTickCount();
    }
}

void led_task(void* arg)
{
    while (1)
    {
        if (xQueueReceive(led_queue, &new_layout, 0))
        {
            ESP_LOGI(TAG, "new layout: %s", new_layout.name);
            task_data.current_layout = new_layout;
            task_data.stage_index = 0;
            task_data.stage_start_time = xTaskGetTickCount();
        }

        if (task_data.current_layout.nb_stages > 0) run_layout();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void led_init(void)
{
    memset(&task_data, 0, sizeof(task_data_t));
    led_queue = xQueueCreate(10, sizeof(layout_t));

    xTaskCreate(led_task, "led_task", 8192, NULL, tskIDLE_PRIORITY, NULL);
}
