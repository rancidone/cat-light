#pragma once
#include "light_engine.h"
#include <stdint.h>

typedef enum {
    MODE_HEARTBEAT = 0,
    MODE_SPOOKY,
    MODE_RAINBOW,
    MODE_CANDLE,
    MODE_BREATHING,
    MODE_FIRE,
    MODE_POLICE,
    MODE_DISCO,
    MODE_AURORA,
    MODE_OCEAN,
    MODE_LIGHTNING,
    MODE_FIREFLY,
    MODE_SOS,
    MODE_LAVA,
    MODE_ALIEN,
    MODE_SUNSET,
    MODE_MATRIX,
    MODE_COMET,
    MODE_THUNDERSTORM,
    MODE_NEON,
    MODE_ZOMBIE,
    MODE_CHRISTMAS,
    MODE_GALAXY,
    MODE_STROBE,
    MODE_COUNT,
} mode_id_t;

void modes_tick(mode_id_t mode, light_frame_t *frame, uint32_t dt_ms);
