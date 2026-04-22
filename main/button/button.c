#include "button_config.h"
#include "button_gpio.h"
#include "esp_log.h"
#include "iot_button.h"
#include "utils.h"

typedef struct
{
    button_event_t event;
    button_event_args_t* event_args;
    button_cb_t cb;
    void* user_data;
} button_init_t;

static const char* TAG = "button";

/* Static functions ***********************************************************/
static void button_single_click_event_cb(void* arg, void* data)
{
    ESP_LOGI(TAG, "Button single click!");
}

static void button_double_click_event_cb(void* arg, void* data)
{
    ESP_LOGI(TAG, "Button double click!");
}

static void button_long_press_event_cb(void* arg, void* data)
{
    ESP_LOGI(TAG, "Button long press!");
}

static void button_repeat_event_cb(void* arg, void* data)
{
    ESP_LOGI(TAG, "Button press repeat!");
}

/* Public functions ***********************************************************/
void button_init(void)
{
    const button_config_t btn_cfg = {
        .long_press_time = BUTTON_LONG_PRESS_TIME,
        .short_press_time = BUTTON_SHORT_PRESS_TIME,
    };

    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = BUTTON_IO_NUM,
        .active_level = BUTTON_ACTIVE_LEVEL,
        .disable_pull = false,
    };

    button_handle_t btn;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &btn);

    const button_init_t button_init[] = {
        {BUTTON_SINGLE_CLICK, NULL, button_single_click_event_cb, NULL},
        {BUTTON_DOUBLE_CLICK, NULL, button_double_click_event_cb, NULL},
        {BUTTON_LONG_PRESS_START, NULL, button_long_press_event_cb, NULL},
        {BUTTON_PRESS_REPEAT, NULL, button_repeat_event_cb, NULL},
    };

    for (uint16_t i = 0; i < ARRAY_DIM(button_init); i++)
    {
        const button_init_t item = button_init[i];
        ret = iot_button_register_cb(btn,
                                     item.event,
                                     item.event_args,
                                     item.cb,
                                     item.user_data);
        ESP_ERROR_CHECK(ret);
    }
}
