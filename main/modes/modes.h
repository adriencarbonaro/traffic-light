#ifndef MODE_H_
#define MODE_H_

#include <stdbool.h>
#include <stdint.h>

#include "led.h"

#define MAX_MODES 10
#define MAX_NAME_LEN 16

typedef struct mode_t
{
    char name[MAX_NAME_LEN];
    uint16_t name_len;
    bool loop;
    uint16_t nb_steps;
    led_step_t steps[MAX_STEPS];
} mode_t;

typedef struct
{
    mode_t modes[MAX_MODES];
    uint8_t count;
    mode_t* active_mode;
} mode_manager_task_data_t;

const mode_t* get_standard_mode_table(uint16_t* table_len);

#endif /* MODE_H_ */
