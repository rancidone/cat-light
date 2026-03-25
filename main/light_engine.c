/**
 * light_engine.c
 * - configure LEDC timers and channels
 * - implement the light engine task
 * - handle frame timing
 * - call mode tick function to determine LEDC channel values
 * - write to LEDC channels
 */
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "light_engine.h"

#define ENGINE_LEDC_SPEED_MODE  LEDC_HIGH_SPEED_MODE
#define ENGINE_LEDC_TIMER       LEDC_TIMER_0
#define ENGINE_LEDC_FREQ_HZ     5000
#define ENGINE_LEDC_DUTY_RES    LEDC_TIMER_12_BIT
#define ENGINE_MAX_DUTY         ((1 << 12) - 1)

#define RED_CHANNEL      LEDC_CHANNEL_0
#define RED_GPIO         GPIO_NUM_17

#define GREEN_CHANNEL    LEDC_CHANNEL_1
#define GREEN_GPIO       GPIO_NUM_18

#define BLUE_CHANNEL     LEDC_CHANNEL_2
#define BLUE_GPIO        GPIO_NUM_19

#define WHISKER_CHANNEL  LEDC_CHANNEL_3
#define WHISKER_GPIO     GPIO_NUM_16

static const char *TAG = "light_engine";

typedef struct {
    SemaphoreHandle_t  mutex;
    light_frame_t      frame;
    float              brightness;   // 0.0–1.0, default 1.0
    mode_tick_fn_t     tick_fn;
    void              *tick_ctx;
} light_engine_t;

static light_engine_t s_engine;

static inline uint32_t s_float_to_duty(float x)
{
    if (x < 0.0f) return 0;
    if (x > 1.0f) return ENGINE_MAX_DUTY;
    return (uint32_t)(x * ENGINE_MAX_DUTY + 0.5f);
}

static void light_engine_task(void *parameters)
{
    (void)parameters;

    TickType_t lastWakeTick = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(30); // 33 Hz
    const uint32_t dt_ms = 30;

    while (true) {
        xSemaphoreTake(s_engine.mutex, portMAX_DELAY);

        if (s_engine.tick_fn != NULL) {
            s_engine.tick_fn(&s_engine.frame, dt_ms, s_engine.tick_ctx);
        }

        float brightness = s_engine.brightness;
        uint32_t duty_r = (uint32_t)(s_engine.frame.eye_r * brightness * ENGINE_MAX_DUTY / 255);
        uint32_t duty_g = (uint32_t)(s_engine.frame.eye_g * brightness * ENGINE_MAX_DUTY / 255);
        uint32_t duty_b = (uint32_t)(s_engine.frame.eye_b * brightness * ENGINE_MAX_DUTY / 255);
        uint32_t duty_w = s_float_to_duty(s_engine.frame.whisker * brightness);

        xSemaphoreGive(s_engine.mutex);

        ledc_set_duty(ENGINE_LEDC_SPEED_MODE, RED_CHANNEL,     duty_r);
        ledc_set_duty(ENGINE_LEDC_SPEED_MODE, GREEN_CHANNEL,   duty_g);
        ledc_set_duty(ENGINE_LEDC_SPEED_MODE, BLUE_CHANNEL,    duty_b);
        ledc_set_duty(ENGINE_LEDC_SPEED_MODE, WHISKER_CHANNEL, duty_w);
        ledc_update_duty(ENGINE_LEDC_SPEED_MODE, RED_CHANNEL);
        ledc_update_duty(ENGINE_LEDC_SPEED_MODE, GREEN_CHANNEL);
        ledc_update_duty(ENGINE_LEDC_SPEED_MODE, BLUE_CHANNEL);
        ledc_update_duty(ENGINE_LEDC_SPEED_MODE, WHISKER_CHANNEL);

        xTaskDelayUntil(&lastWakeTick, period);
    }
}

void light_engine_init(void)
{
    ESP_LOGI(TAG, "Initializing light engine");
    s_engine.mutex      = xSemaphoreCreateMutex();
    s_engine.brightness = 1.0f;
    s_engine.tick_fn    = NULL;
    s_engine.tick_ctx   = NULL;
    memset(&s_engine.frame, 0, sizeof(s_engine.frame));

    ESP_ERROR_CHECK(s_engine.mutex ? ESP_OK : ESP_ERR_NO_MEM);

    ledc_timer_config_t timer_config = {
        .speed_mode      = ENGINE_LEDC_SPEED_MODE,
        .duty_resolution = ENGINE_LEDC_DUTY_RES,
        .timer_num       = ENGINE_LEDC_TIMER,
        .freq_hz         = ENGINE_LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .speed_mode = ENGINE_LEDC_SPEED_MODE,
        .timer_sel  = ENGINE_LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .duty       = 0,
    };

    ESP_LOGI(TAG, "Initializing red channel");
    channel_config.channel  = RED_CHANNEL;
    channel_config.gpio_num = RED_GPIO;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    ESP_LOGI(TAG, "Initializing green channel");
    channel_config.channel  = GREEN_CHANNEL;
    channel_config.gpio_num = GREEN_GPIO;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    ESP_LOGI(TAG, "Initializing blue channel");
    channel_config.channel  = BLUE_CHANNEL;
    channel_config.gpio_num = BLUE_GPIO;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    ESP_LOGI(TAG, "Initializing whisker channel");
    channel_config.channel  = WHISKER_CHANNEL;
    channel_config.gpio_num = WHISKER_GPIO;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    ESP_LOGI(TAG, "Light engine initialized.");
}

void light_engine_start(void)
{
    ESP_LOGI(TAG, "Starting light engine...");
    BaseType_t result = xTaskCreatePinnedToCore(light_engine_task, "light_engine", 4096, NULL, 5, NULL, APP_CPU_NUM);

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create light engine task");
    } else {
        ESP_LOGI(TAG, "Light engine started.");
    }
}

void light_engine_set_mode_tick(mode_tick_fn_t fn, void *ctx)
{
    xSemaphoreTake(s_engine.mutex, portMAX_DELAY);
    s_engine.tick_fn  = fn;
    s_engine.tick_ctx = ctx;
    memset(&s_engine.frame, 0, sizeof(s_engine.frame));
    xSemaphoreGive(s_engine.mutex);
}

void light_engine_set_brightness(float brightness)
{
    xSemaphoreTake(s_engine.mutex, portMAX_DELAY);
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;
    s_engine.brightness = brightness;
    xSemaphoreGive(s_engine.mutex);
}
