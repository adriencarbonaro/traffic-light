#include "led.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "string.h"

#include "modes.h"

#define IO_RED 9
#define IO_ORANGE 8
#define IO_GREEN 20

#define GPIO_OUTPUT_PIN_MASK BIT(IO_RED) | BIT(IO_ORANGE) | BIT(IO_GREEN);

typedef struct
{
    mode_t current_mode;
    uint16_t step_index;
    uint32_t step_start_time;
} task_data_t;

static task_data_t task_data = {0};
static mode_t new_mode = {0};
static QueueHandle_t led_queue = NULL;

static const char* TAG = "led_task";

static void apply_mask(led_mask_t mask)
{
    gpio_set_level(IO_GREEN, (mask & 0x01) ? 1 : 0);
    gpio_set_level(IO_ORANGE, (mask & 0x02) ? 1 : 0);
    gpio_set_level(IO_RED, (mask & 0x04) ? 1 : 0);
}

void clear_mode(mode_t* mode)
{
    mode->loop = false;
    memset(mode->name, 0, mode->name_len);
    mode->name_len = 0;
    for (uint16_t i = 0; i < mode->nb_steps; i++)
    {
        memset(&mode->steps[i], 0, sizeof(led_step_t));
    }
    mode->nb_steps = 0;
}

void run_layout()
{
    mode_t* mode = &task_data.current_mode;
    led_step_t* steps = &mode->steps[task_data.step_index];

    if ((pdTICKS_TO_MS(xTaskGetTickCount()) - task_data.step_start_time) >=
        steps->duration)
    {
        task_data.step_index++;

        if (task_data.step_index >= mode->nb_steps)
        {
            if (mode->loop)
                task_data.step_index = 0;
            else
            {
                clear_mode(mode);
                return;
            }
        }

        task_data.step_start_time = pdTICKS_TO_MS(xTaskGetTickCount());

        led_mask_t mask = mode->steps[task_data.step_index].mask;
        apply_mask(mask);
    }
}

void led_task(void* arg)
{
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_MASK;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    apply_mask(LED_GREEN | LED_ORANGE | LED_RED);

    ESP_LOGI(TAG, "preparing to receive");
    while (1)
    {
        if (xQueueReceive(led_queue, &new_mode, 0))
        {
            ESP_LOGI(TAG,
                     "receiving (nb_steps=%u, duration_first_step=%u)",
                     new_mode.nb_steps,
                     new_mode.steps[0].duration);
            task_data.current_mode = new_mode;
            task_data.step_index = 0;
            task_data.step_start_time = pdTICKS_TO_MS(xTaskGetTickCount());

            apply_mask(task_data.current_mode.steps[0].mask);
        }

        if (task_data.current_mode.nb_steps > 0) run_layout();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void led_set(const mode_t* mode) { xQueueSend(led_queue, mode, 0); }

void led_init(void)
{
    memset(&task_data, 0, sizeof(task_data_t));
    led_queue = xQueueCreate(10, sizeof(mode_t));

    xTaskCreate(led_task, "led_task", 8192, NULL, tskIDLE_PRIORITY, NULL);

    uint16_t table_len = 0;
    const mode_t* table = get_standard_mode_table(&table_len);
    ESP_LOGI(TAG, "table=%p, table_len=%u", table, table_len);
}
