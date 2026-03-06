#include "led.h"

typedef enum {
    MODE_ALL_LEDS,
    MODE_STANDARD,
    MODE_BLINK_ORANGE,
} mode_id_t;

const layout_t standard_modes_table[] = {
    {
        .name = "all_leds",
        .stages = {
            { LED_GREEN | LED_ORANGE | LED_RED, 0 },
        },
        .nb_stages = 1,
    },
    {
        .name = "standard",
        .stages = {
            { LED_GREEN, 30000 },
            { LED_ORANGE, 3000 },
            { LED_RED, 30000 },
        },
        .nb_stages = 3,
        .loop = true
    },
    {
        .name = "blinking_orange",
        .stages = {
            { LED_ORANGE, 500 },
            { LED_NONE, 500 },
        },
        .nb_stages = 2,
        .loop = true
    },
};
