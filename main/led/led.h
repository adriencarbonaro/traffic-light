#ifndef LED_H
#define LED_H

#include <stdint.h>

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#define MAX_STEPS 10

typedef uint8_t led_mask_t;

typedef struct mode_t mode_t;

typedef enum
{
    LED_BIT_GREEN,
    LED_BIT_ORANGE,
    LED_BIT_RED,
} led_color_bit_t;

#define LED_NONE 0
#define LED_GREEN BIT(LED_BIT_GREEN)
#define LED_ORANGE BIT(LED_BIT_ORANGE)
#define LED_RED BIT(LED_BIT_RED)
#define LED_ALL LED_GREEN | LED_ORANGE | LED_RED

typedef struct
{
    led_mask_t mask;
    uint16_t duration;
} led_step_t;

void led_init(void);
void led_set(const mode_t* mode);

#endif /* LED_H */
