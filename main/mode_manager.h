#pragma once
#include "behavior_types.h"
#include "light_engine.h"

typedef enum {
    EYE_MODE_OFF = 0,
    EYE_MODE_SCALAR_TINT,
    EYE_MODE_RGB_DIRECT,
    EYE_MODE_SCRIPT,
} mode_kind_eye_t;

typedef enum {
    WHISKER_MODE_OFF = 0,
    WHISKER_MODE_SCALAR,
    WHISKER_MODE_SCRIPT,
} mode_kind_whisker_t;

typedef struct {
    mode_kind_eye_t kind;

    union {
        struct {
            const scalar_behavior_t *behavior;
            const void *config;
            rgb_frame_t base_color;
        } scalar_tint;

        struct {
            const rgb_behavior_t *behavior;
            const void *config;
        } rgb_direct;
    };
} eye_mode_config_t;

typedef struct {
    mode_kind_whisker_t kind;

    struct {
        const scalar_behavior_t *behavior;
        const void *config;
    } scalar;
} whisker_mode_config_t;

typedef struct {
    const char *name;
    eye_mode_config_t eyes;
    whisker_mode_config_t whiskers;
} light_mode_t;

typedef enum {
    LIGHT_MODE_DEBUG_PULSE = 0,
    LIGHT_MODE_IDLE,
    LIGHT_MODE_SPOOKY,
    LIGHT_MODE_RAINBOW,
    LIGHT_MODE_COUNT
} light_mode_id_t;

esp_err_t light_mode_manager_apply(light_engine_t *engine, light_mode_id_t mode_id);
void      mode_manager_cycle(light_engine_t *engine);