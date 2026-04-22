#include "led.h"
#include "modes.h"
#include "utils.h"

typedef enum
{
    MODE_ALL_LEDS,
    MODE_STANDARD,
    MODE_BLINK_ORANGE,
} mode_id_t;

#define MODE(name_, loop_, ...)                                               \
    {                                                                         \
        .name = name_, .steps = {__VA_ARGS__},                                \
        .nb_steps = sizeof((led_step_t[]){__VA_ARGS__}) / sizeof(led_step_t), \
        .loop = loop_                                                         \
    }

static const mode_t standard_modes_table[] = {
    MODE("test_leds",
         true,
         {.mask = LED_GREEN, .duration = 500},
         {.mask = LED_ORANGE, .duration = 500},
         {.mask = LED_RED, .duration = 500},
         {.mask = LED_ALL, .duration = 500},
         {.mask = LED_NONE, .duration = 500}),
    MODE("all_leds", true, {.mask = LED_ALL, .duration = 3000}),
    MODE("standard",
         true,
         {.mask = LED_GREEN, .duration = 30000},
         {.mask = LED_ORANGE, .duration = 3000},
         {.mask = LED_RED, .duration = 30000}),
    MODE("blinking_orange",
         true,
         {.mask = LED_ORANGE, .duration = 500},
         {.mask = LED_NONE, .duration = 500}),
};

const mode_t* get_standard_mode_table(uint16_t* table_len)
{
    *table_len = ARRAY_DIM(standard_modes_table);
    return standard_modes_table;
}
