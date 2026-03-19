#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mode_manager.h"
#include "behavior.h"

static const char *TAG = "mode_manager";

// ---- Mode configs ----

static const behavior_solid_rgb_config_t s_idle_eye_config = {
    .color = {.r = 0.0f, .g = 0.05f, .b = 0.45f},
};
static const behavior_solid_config_t s_idle_whisker_config = {
    .brightness = 0.12f,
};

static const behavior_flicker_config_t s_spooky_eye_config = {
    .color         = {.r = 1.0f, .g = 0.0f, .b = 0.0f},
    .min_brightness = 0.15f,
    .max_brightness = 1.0f,
    .avg_flicker_hz = 12.0f,
};
static const behavior_pulse_config_t s_spooky_whisker_config = {
    .cycle_speed = 3.0f,
};

static const behavior_rainbow_config_t s_rainbow_eye_config = {
    .cycle_speed = 0.2f,
};
static const behavior_solid_config_t s_rainbow_whisker_config = {
    .brightness = 0.12f,
};

// ---- Mode table ----

static const light_mode_t LIGHT_MODE_TABLE[LIGHT_MODE_COUNT] = {
    [LIGHT_MODE_IDLE] = {
        .name = "idle",
        .eyes = {
            .kind = EYE_MODE_RGB_DIRECT,
            .rgb_direct = {
                .behavior = &behavior_solid_rgb,
                .config   = &s_idle_eye_config,
            },
        },
        .whiskers = {
            .kind = WHISKER_MODE_SCALAR,
            .scalar = {
                .behavior = &behavior_solid,
                .config   = &s_idle_whisker_config,
            },
        },
    },
    [LIGHT_MODE_SPOOKY] = {
        .name = "spooky",
        .eyes = {
            .kind = EYE_MODE_RGB_DIRECT,
            .rgb_direct = {
                .behavior = &behavior_flicker,
                .config   = &s_spooky_eye_config,
            },
        },
        .whiskers = {
            .kind = WHISKER_MODE_SCALAR,
            .scalar = {
                .behavior = &behavior_pulse,
                .config   = &s_spooky_whisker_config,
            },
        },
    },
    [LIGHT_MODE_RAINBOW] = {
        .name = "rainbow",
        .eyes = {
            .kind = EYE_MODE_RGB_DIRECT,
            .rgb_direct = {
                .behavior = &behavior_rainbow,
                .config   = &s_rainbow_eye_config,
            },
        },
        .whiskers = {
            .kind = WHISKER_MODE_OFF,
        },
    },
};

static light_mode_id_t s_current_mode = LIGHT_MODE_IDLE;

void mode_manager_cycle(light_engine_t *engine)
{
    light_mode_id_t next = (s_current_mode + 1) % LIGHT_MODE_COUNT;
    light_mode_manager_apply(engine, next);
}

esp_err_t light_mode_manager_apply(light_engine_t *engine, light_mode_id_t mode_id)
{
    const light_mode_t *mode = &LIGHT_MODE_TABLE[mode_id];
    s_current_mode = mode_id;
    ESP_LOGI(TAG, "Applying mode: %s", mode->name);
    xSemaphoreTake(engine->behavior_mutex, portMAX_DELAY);

    // Eye behavior
    switch (mode->eyes.kind) {
        case EYE_MODE_RGB_DIRECT:
            engine->rgb_behavior.type   = mode->eyes.rgb_direct.behavior;
            engine->rgb_behavior.config = mode->eyes.rgb_direct.config;
            engine->rgb_behavior.type->init(engine->rgb_behavior.state,
                                            engine->rgb_behavior.config);
            break;
        default:
            memset(&engine->rgb_behavior, 0, sizeof(engine->rgb_behavior));
            engine->frame.rgb = (rgb_frame_t){0};
            break;
    }

    // Whisker behavior
    switch (mode->whiskers.kind) {
        case WHISKER_MODE_SCALAR:
            engine->whisker_behavior.type   = mode->whiskers.scalar.behavior;
            engine->whisker_behavior.config = mode->whiskers.scalar.config;
            engine->whisker_behavior.type->init(engine->whisker_behavior.state,
                                                engine->whisker_behavior.config);
            break;
        default:
            memset(&engine->whisker_behavior, 0, sizeof(engine->whisker_behavior));
            engine->frame.whisker = 0.0f;
            break;
    }

    xSemaphoreGive(engine->behavior_mutex);
    return ESP_OK;
}
