#include "led.h"

void run_layout()
{
    layout_stage_t stage = current_layout->stages[stage_index];

    set_led(stage.color);

    if (time_elapsed(stage_start_time) >= stage.duration_ms) {

        stage_index++;

        if (stage_index >= current_layout->stage_count) {

            if (current_layout->loop)
                stage_index = 0;
            else
                current_layout = NULL;
        }

        stage_start_time = now();
    }
}

void led_task(void)
{
    while (1)
    {
        if (xQueueReceive(led_queue, &cmd, 0)) {
            current_layout = cmd.layout;
            stage_index = 0;
            stage_start_time = now();
        }

        if (current_layout != NULL) {
            run_layout();
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
