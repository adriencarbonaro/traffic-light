#ifndef LED_H
#define LED_H

#include "types.h"

#ifndef BIT
#define BIT(x) (1 << (x))
#endif
#define MAX_NB_STAGES 10

typedef uint8_t led_mask_t;

typedef enum {
    LED_BIT_GREEN,
    LED_BIT_ORANGE,
    LED_BIT_RED,
} led_color_bit_t;

#define LED_NONE   0
#define LED_GREEN  BIT(LED_BIT_GREEN)
#define LED_ORANGE BIT(LED_BIT_ORANGE)
#define LED_RED    BIT(LED_BIT_RED)

typedef struct {
    led_mask_t led_mask;
    uint32_t duration_ms;
} layout_stage_t;

typedef struct {
    const char* name;
    layout_stage_t stages[MAX_NB_STAGES];
    uint16_t nb_stages;
    bool loop;
} layout_t;

void led_init(void);

#endif /* LED_H */
