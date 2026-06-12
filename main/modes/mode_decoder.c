#include "mode_decoder.h"
#include "mode_manager.h"
#include "modes.h"

#include <esp_log.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static char* TAG = "mode_decoder";

static inline size_t read_uint8(const uint8_t* data, uint8_t* dest)
{
    *dest = *data;
    return sizeof(uint8_t);
}

static inline size_t read_uint16(const uint8_t* data, uint16_t* dest)
{
    *dest = data[0] | (data[1] << 8);
    return sizeof(uint16_t);
}

static inline size_t read_uint32(const uint8_t* data, uint32_t* dest)
{
    *dest = (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
            ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
    return sizeof(uint32_t);
}

static inline size_t read_str(const uint8_t* data, uint8_t* dest, uint16_t len)
{
    memcpy(dest, data, len);
    return len;
}

void parse_command(const uint8_t* data,
                   size_t len,
                   command_id_t* cmd_id,
                   mode_t* mode)
{
    uint16_t idx = 0;
    uint8_t tmp = 0;
    idx += read_uint8(&data[idx], &tmp);
    *cmd_id = (command_id_t)tmp;

    switch (*cmd_id)
    {
        case CMD_SET_MODE:
        {
            idx += read_uint16(&data[idx], &mode->name_len);
            idx += read_str(&data[idx], (uint8_t*)&mode->name, mode->name_len);

            break;
        }
        case CMD_ADD_MODE:
        {
            idx += read_uint16(&data[idx], &mode->name_len);
            idx += read_str(&data[idx], (uint8_t*)&mode->name, mode->name_len);
            idx += read_uint8(&data[idx], (uint8_t*)&mode->loop);
            idx += read_uint16(&data[idx], &mode->nb_steps);

            for (uint16_t i = 0; i < mode->nb_steps; i++)
            {
                idx += read_uint8(&data[idx], &mode->steps[i].mask);
                idx += read_uint32(&data[idx], &mode->steps[i].duration);
            }

            break;
        }
        case CMD_CUSTOM_MODE:
        {
            idx += read_uint8(&data[idx], (uint8_t*)&mode->loop);
            idx += read_uint16(&data[idx], &mode->nb_steps);

            for (uint16_t i = 0; i < mode->nb_steps; i++)
            {
                idx += read_uint8(&data[idx], &mode->steps[i].mask);
                idx += read_uint32(&data[idx], &mode->steps[i].duration);
            }

            break;
        }
        case CMD_DELETE_MODE:
        {
            /* Name only, same layout as CMD_SET_MODE. */
            idx += read_uint16(&data[idx], &mode->name_len);
            if (mode->name_len > MAX_NAME_LEN) mode->name_len = MAX_NAME_LEN;
            idx += read_str(&data[idx], (uint8_t*)&mode->name, mode->name_len);

            break;
        }
        case CMD_EDIT_MODE:
        {
            /* Name + full step body, same layout as CMD_ADD_MODE. */
            idx += read_uint16(&data[idx], &mode->name_len);
            if (mode->name_len > MAX_NAME_LEN) mode->name_len = MAX_NAME_LEN;
            idx += read_str(&data[idx], (uint8_t*)&mode->name, mode->name_len);
            idx += read_uint8(&data[idx], (uint8_t*)&mode->loop);
            idx += read_uint16(&data[idx], &mode->nb_steps);
            if (mode->nb_steps > MAX_STEPS) mode->nb_steps = MAX_STEPS;

            for (uint16_t i = 0; i < mode->nb_steps; i++)
            {
                idx += read_uint8(&data[idx], &mode->steps[i].mask);
                idx += read_uint32(&data[idx], &mode->steps[i].duration);
            }

            break;
        }
        default:
            break;
    }

    ESP_LOGI(TAG,
             "name=%.*s, loop=%u, nb_steps=%u",
             mode->name_len,
             mode->name,
             mode->loop,
             mode->nb_steps);
    for (uint16_t i = 0; i < mode->nb_steps; i++)
    {
        ESP_LOGI(TAG, "  step %u", i);
        ESP_LOGI(TAG,
                 "    mask=0x%02x, duration=%lu ms",
                 mode->steps[i].mask,
                 (unsigned long)mode->steps[i].duration);
    }
}
