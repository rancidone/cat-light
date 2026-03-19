#pragma once

#include <stdint.h>
#include "esp_err.h"

// 0.0 - 1.0 brightness for each channel
typedef struct {
    float r;
    float g;
    float b;
} rgb_frame_t;

typedef struct {
    rgb_frame_t rgb;
    float whisker; // 0.0 - 1.0 brightness
} light_frame_t;

typedef void (behavior_fn_t)(
    void *state,
    uint16_t time_delta_ms,
    light_frame_t *out
);

// ---- Behavior type definitions ----
typedef struct {
    const size_t state_size;
    esp_err_t (*init)(void *state, const void *config);
    esp_err_t (*update)(void *state, float *output, int dt_ms);
} scalar_behavior_t;

typedef struct {
    const size_t state_size;
    esp_err_t (*init)(void *state, const void *config);
    esp_err_t (*update)(void *state, rgb_frame_t *output, int dt_ms);
} rgb_behavior_t;

#define BEHAVIOR_STATE_MAX_SIZE 64
typedef struct {
    const scalar_behavior_t *type;
    const void *config;
    alignas(max_align_t) uint8_t state[BEHAVIOR_STATE_MAX_SIZE];
} scalar_behavior_instance_t;

typedef struct {
    const rgb_behavior_t *type;
    const void *config;
    alignas(max_align_t) uint8_t state[BEHAVIOR_STATE_MAX_SIZE];
} rgb_behavior_instance_t;


typedef enum {
    BEHAVIOR_ID_SOLID,
    BEHAVIOR_ID_PULSE,
    BEHAVIOR_ID_BLINK,
    BEHAVIOR_ID_FLICKER,
    BEHAVIOR_ID_COUNT
} behavior_id_t;

// ---- Behavior config and state definitions ----
typedef struct {
    float cycle_speed; // pulses per second
} behavior_pulse_config_t;

typedef struct { 
    float phase;       // 0 -> 2*PI
    float cycle_speed; // pulses per second
} behavior_pulse_state_t;

typedef struct {
    float brightness; // 0.0 - 1.0
} behavior_solid_config_t;

typedef struct {
    float brightness; // 0.0 - 1.0
} behavior_solid_state_t;

// ---- Solid RGB ----
typedef struct {
    rgb_frame_t color;
} behavior_solid_rgb_config_t;

typedef struct {
    rgb_frame_t color;
} behavior_solid_rgb_state_t;

// ---- Flicker RGB ----
typedef struct {
    rgb_frame_t color;
    float min_brightness; // 0.0 - 1.0
    float max_brightness; // 0.0 - 1.0
    float avg_flicker_hz;
} behavior_flicker_config_t;

typedef struct {
    rgb_frame_t color;
    float min_brightness;
    float max_brightness;
    float avg_flicker_hz;
    float brightness;
    int   ms_until_change;
    uint32_t rng;
} behavior_flicker_state_t;

// ---- Rainbow ----
typedef struct {
    float cycle_speed; // full cycles per second
} behavior_rainbow_config_t;

typedef struct {
    float hue;        // 0.0 - 1.0
    float cycle_speed;
} behavior_rainbow_state_t;