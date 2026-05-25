#pragma once
#include <stdint.h>
#include <stddef.h>
#include "light_engine.h"

typedef enum {
    SCRIPTING_STATUS_IDLE    = 0,
    SCRIPTING_STATUS_RUNNING = 1,
    SCRIPTING_STATUS_SLEEPING = 2,
    SCRIPTING_STATUS_ERROR   = 3,
} scripting_engine_status_t;

void scripting_engine_init(void);
void scripting_engine_load(const char *src, size_t len);
void scripting_engine_tick(light_frame_t *frame, uint32_t now_ms, uint32_t dt_ms);
void scripting_engine_stop(void);
scripting_engine_status_t scripting_engine_status(void);
