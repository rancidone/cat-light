#pragma once
#include <stdint.h>

typedef struct {
    uint8_t eye_r;
    uint8_t eye_g;
    uint8_t eye_b;
    float   whisker; // 0.0–1.0
} light_frame_t;

void light_engine_init(void);
void light_engine_write(const light_frame_t *frame);
