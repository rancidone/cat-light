#pragma once
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "behavior_types.h"

typedef struct {
    rgb_behavior_instance_t    rgb_behavior;
    scalar_behavior_instance_t whisker_behavior;
    light_frame_t              frame;
    SemaphoreHandle_t          behavior_mutex;
} light_engine_t;

void light_engine_init();
void light_engine_start();
light_engine_t *light_engine_get(void);