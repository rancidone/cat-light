#include "behavior.h"

#include <math.h>
#include "esp_log.h"

static const char *TAG = "behavior";

// ---- LCG RNG ----
static uint32_t lcg_next(uint32_t *state)
{
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

// ---- HSV to RGB ----
static rgb_frame_t hsv_to_rgb(float h, float s, float v)
{
    int i = (int)(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i % 6) {
        case 0: return (rgb_frame_t){v, t, p};
        case 1: return (rgb_frame_t){q, v, p};
        case 2: return (rgb_frame_t){p, v, t};
        case 3: return (rgb_frame_t){p, q, v};
        case 4: return (rgb_frame_t){t, p, v};
        case 5: return (rgb_frame_t){v, p, q};
    }
    return (rgb_frame_t){0};
}

// ---- Solid (scalar) ----
static esp_err_t s_behavior_solid_init(void *v_state, const void *v_config)
{
    ESP_LOGI(TAG, "Initializing behavior [solid]...");
    behavior_solid_state_t *state = (behavior_solid_state_t *)v_state;
    const behavior_solid_config_t *config = (const behavior_solid_config_t *)v_config;
    state->brightness = config->brightness;
    ESP_LOGI(TAG, "Behavior initialized [solid]: brightness=%.2f", state->brightness);
    return ESP_OK;
}

static esp_err_t s_behavior_solid_update(void *v_state, float *output, int dt_ms)
{
    behavior_solid_state_t *state = (behavior_solid_state_t *)v_state;
    *output = state->brightness;
    return ESP_OK;
}

const scalar_behavior_t behavior_solid = {
    .state_size = sizeof(behavior_solid_state_t),
    .init   = s_behavior_solid_init,
    .update = s_behavior_solid_update,
};

// ---- Pulse (scalar) ----
static esp_err_t s_behavior_pulse_init(void *v_state, const void *v_config)
{
    ESP_LOGI(TAG, "Initializing behavior [pulse]...");
    behavior_pulse_state_t *state = (behavior_pulse_state_t *)v_state;
    const behavior_pulse_config_t *config = (const behavior_pulse_config_t *)v_config;
    state->phase = 0.0f;
    state->cycle_speed = config->cycle_speed;
    ESP_LOGI(TAG, "Behavior initialized [pulse]: cycle_speed=%.2f", state->cycle_speed);
    return ESP_OK;
}

static esp_err_t s_behavior_pulse_update(void *v_state, float *output, int dt_ms)
{
    behavior_pulse_state_t *state = (behavior_pulse_state_t *)v_state;
    float dt_sec = dt_ms / 1000.0f;
    state->phase += (float)M_TWOPI * state->cycle_speed * dt_sec;
    if (state->phase >= (float)M_TWOPI) state->phase -= (float)M_TWOPI;
    *output = (sinf(state->phase) + 1.0f) / 2.0f;
    return ESP_OK;
}

const scalar_behavior_t behavior_pulse = {
    .state_size = sizeof(behavior_pulse_state_t),
    .init   = s_behavior_pulse_init,
    .update = s_behavior_pulse_update,
};

// ---- Solid RGB ----
static esp_err_t s_behavior_solid_rgb_init(void *v_state, const void *v_config)
{
    ESP_LOGI(TAG, "Initializing behavior [solid_rgb]...");
    behavior_solid_rgb_state_t *state = (behavior_solid_rgb_state_t *)v_state;
    const behavior_solid_rgb_config_t *config = (const behavior_solid_rgb_config_t *)v_config;
    state->color = config->color;
    ESP_LOGI(TAG, "Behavior initialized [solid_rgb]");
    return ESP_OK;
}

static esp_err_t s_behavior_solid_rgb_update(void *v_state, rgb_frame_t *output, int dt_ms)
{
    behavior_solid_rgb_state_t *state = (behavior_solid_rgb_state_t *)v_state;
    *output = state->color;
    return ESP_OK;
}

const rgb_behavior_t behavior_solid_rgb = {
    .state_size = sizeof(behavior_solid_rgb_state_t),
    .init   = s_behavior_solid_rgb_init,
    .update = s_behavior_solid_rgb_update,
};

// ---- Pulse RGB ----
static esp_err_t s_behavior_pulse_rgb_init(void *v_state, const void *v_config)
{
    ESP_LOGI(TAG, "Initializing behavior [pulse_rgb]...");
    behavior_pulse_rgb_state_t *state = (behavior_pulse_rgb_state_t *)v_state;
    const behavior_pulse_rgb_config_t *config = (const behavior_pulse_rgb_config_t *)v_config;
    state->color       = config->color;
    state->phase       = 0.0f;
    state->cycle_speed = config->cycle_speed;
    ESP_LOGI(TAG, "Behavior initialized [pulse_rgb]: cycle_speed=%.2f", state->cycle_speed);
    return ESP_OK;
}

static esp_err_t s_behavior_pulse_rgb_update(void *v_state, rgb_frame_t *output, int dt_ms)
{
    behavior_pulse_rgb_state_t *state = (behavior_pulse_rgb_state_t *)v_state;
    state->phase += (float)M_TWOPI * state->cycle_speed * (dt_ms / 1000.0f);
    if (state->phase >= (float)M_TWOPI) state->phase -= (float)M_TWOPI;
    float brightness = (sinf(state->phase) + 1.0f) / 2.0f;
    output->r = state->color.r * brightness;
    output->g = state->color.g * brightness;
    output->b = state->color.b * brightness;
    return ESP_OK;
}

const rgb_behavior_t behavior_pulse_rgb = {
    .state_size = sizeof(behavior_pulse_rgb_state_t),
    .init   = s_behavior_pulse_rgb_init,
    .update = s_behavior_pulse_rgb_update,
};

// ---- Flicker RGB ----
static esp_err_t s_behavior_flicker_init(void *v_state, const void *v_config)
{
    ESP_LOGI(TAG, "Initializing behavior [flicker]...");
    behavior_flicker_state_t *state = (behavior_flicker_state_t *)v_state;
    const behavior_flicker_config_t *config = (const behavior_flicker_config_t *)v_config;
    state->color         = config->color;
    state->min_brightness = config->min_brightness;
    state->max_brightness = config->max_brightness;
    state->avg_flicker_hz = config->avg_flicker_hz;
    state->brightness    = config->max_brightness;
    state->ms_until_change = 0;
    state->rng           = 0xDEADBEEFu;
    ESP_LOGI(TAG, "Behavior initialized [flicker]");
    return ESP_OK;
}

static esp_err_t s_behavior_flicker_update(void *v_state, rgb_frame_t *output, int dt_ms)
{
    behavior_flicker_state_t *state = (behavior_flicker_state_t *)v_state;

    state->ms_until_change -= dt_ms;
    if (state->ms_until_change <= 0) {
        float rand_f = (float)(lcg_next(&state->rng) & 0xFFFFu) / 65535.0f;
        state->brightness = state->min_brightness
            + rand_f * (state->max_brightness - state->min_brightness);
        float avg_ms = 1000.0f / state->avg_flicker_hz;
        float rand_t = (float)(lcg_next(&state->rng) & 0xFFFFu) / 65535.0f;
        state->ms_until_change = (int)(avg_ms * (0.5f + rand_t));
    }

    output->r = state->color.r * state->brightness;
    output->g = state->color.g * state->brightness;
    output->b = state->color.b * state->brightness;
    return ESP_OK;
}

const rgb_behavior_t behavior_flicker = {
    .state_size = sizeof(behavior_flicker_state_t),
    .init   = s_behavior_flicker_init,
    .update = s_behavior_flicker_update,
};

// ---- Rainbow ----
static esp_err_t s_behavior_rainbow_init(void *v_state, const void *v_config)
{
    ESP_LOGI(TAG, "Initializing behavior [rainbow]...");
    behavior_rainbow_state_t *state = (behavior_rainbow_state_t *)v_state;
    const behavior_rainbow_config_t *config = (const behavior_rainbow_config_t *)v_config;
    state->hue         = 0.0f;
    state->cycle_speed = config->cycle_speed;
    ESP_LOGI(TAG, "Behavior initialized [rainbow]");
    return ESP_OK;
}

static esp_err_t s_behavior_rainbow_update(void *v_state, rgb_frame_t *output, int dt_ms)
{
    behavior_rainbow_state_t *state = (behavior_rainbow_state_t *)v_state;
    state->hue += state->cycle_speed * (dt_ms / 1000.0f);
    if (state->hue >= 1.0f) state->hue -= 1.0f;
    *output = hsv_to_rgb(state->hue, 1.0f, 1.0f);
    return ESP_OK;
}

const rgb_behavior_t behavior_rainbow = {
    .state_size = sizeof(behavior_rainbow_state_t),
    .init   = s_behavior_rainbow_init,
    .update = s_behavior_rainbow_update,
};
