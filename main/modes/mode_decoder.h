#ifndef MODE_DECODER_H
#define MODE_DECODER_H

#include <stddef.h>
#include <stdint.h>

#include "mode_manager.h"
#include "modes.h"

void parse_command(const uint8_t* data,
                   size_t len,
                   command_id_t* cmd_id,
                   mode_t* mode);

#endif /* MODE_DECODER_H */
