#pragma once
#include "light_engine.h"
#include <stdint.h>

typedef enum {
    MODE_FADE = 0,
    MODE_SPOOKY,
    MODE_RAINBOW,
    MODE_COUNT,
} mode_id_t;

void modes_tick(mode_id_t mode, light_frame_t *frame, uint32_t dt_ms);
