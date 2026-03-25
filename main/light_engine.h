#pragma once
#include <stdint.h>

typedef struct {
    uint8_t eye_r;
    uint8_t eye_g;
    uint8_t eye_b;
    float   whisker; // 0.0–1.0
} light_frame_t;

typedef void (*mode_tick_fn_t)(light_frame_t *frame, uint32_t dt_ms, void *ctx);

void light_engine_init(void);
void light_engine_start(void);
void light_engine_set_mode_tick(mode_tick_fn_t fn, void *ctx);
void light_engine_set_brightness(float brightness);
