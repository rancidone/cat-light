/**
 * light_engine.c
 * - configure LEDC timers and channels
 * - implement the light engine task
 * - handle frame timing
 * - call behavior function to determine LEDC channel values
 * - write to LEDC channels
 */
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
#define ENGINE_MAX_DUTY         ((1 << ENGINE_LEDC_DUTY_RES) - 1)


#define RED_CHANNEL      LEDC_CHANNEL_0
#define RED_GPIO         GPIO_NUM_19

#define GREEN_CHANNEL    LEDC_CHANNEL_1
#define GREEN_GPIO       GPIO_NUM_18

#define BLUE_CHANNEL     LEDC_CHANNEL_2
#define BLUE_GPIO        GPIO_NUM_17

#define WHISKER_CHANNEL  LEDC_CHANNEL_3
#define WHISKER_GPIO     GPIO_NUM_21

static const char *TAG = "light_engine";

static light_engine_t s_engine;

static inline uint32_t s_float_to_duty(float x)
{
    if (x < 0.0f) return 0;
    if (x > 1.0f) return ENGINE_MAX_DUTY;
    return (uint32_t)(x * ENGINE_MAX_DUTY + 0.5f);
}

static void s_write_frame(const light_frame_t *frame)
{
    ledc_set_duty(ENGINE_LEDC_SPEED_MODE, RED_CHANNEL,     s_float_to_duty(frame->rgb.r));
    ledc_set_duty(ENGINE_LEDC_SPEED_MODE, GREEN_CHANNEL,   s_float_to_duty(frame->rgb.g));
    ledc_set_duty(ENGINE_LEDC_SPEED_MODE, BLUE_CHANNEL,    s_float_to_duty(frame->rgb.b));
    ledc_set_duty(ENGINE_LEDC_SPEED_MODE, WHISKER_CHANNEL, s_float_to_duty(frame->whisker));
    ledc_update_duty(ENGINE_LEDC_SPEED_MODE, RED_CHANNEL);
    ledc_update_duty(ENGINE_LEDC_SPEED_MODE, GREEN_CHANNEL);
    ledc_update_duty(ENGINE_LEDC_SPEED_MODE, BLUE_CHANNEL);
    ledc_update_duty(ENGINE_LEDC_SPEED_MODE, WHISKER_CHANNEL);
}

static void light_engine_task(void *parameters) {
    light_engine_t *engine = (light_engine_t *)parameters;

    TickType_t lastWakeTick = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(20); // 50 Hz
    const int dt_ms = 20;

    while (true) {
        xSemaphoreTake(engine->behavior_mutex, portMAX_DELAY);
        if (engine->rgb_behavior.type) {
            engine->rgb_behavior.type->update(
                engine->rgb_behavior.state, &engine->frame.rgb, dt_ms);
        }
        if (engine->whisker_behavior.type) {
            engine->whisker_behavior.type->update(
                engine->whisker_behavior.state, &engine->frame.whisker, dt_ms);
        }
        xSemaphoreGive(engine->behavior_mutex);
        s_write_frame(&engine->frame);
        xTaskDelayUntil(&lastWakeTick, period);
    }
}

void light_engine_init() {
    ESP_LOGI(TAG, "Initializing light engine");
    s_engine.behavior_mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(s_engine.behavior_mutex ? ESP_OK : ESP_ERR_NO_MEM);
    ledc_timer_config_t timer_config = {
        .speed_mode = ENGINE_LEDC_SPEED_MODE,
        .duty_resolution = ENGINE_LEDC_DUTY_RES,
        .timer_num = ENGINE_LEDC_TIMER,
        .freq_hz = ENGINE_LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));
    
    ledc_channel_config_t channel_config = {
        .speed_mode = ENGINE_LEDC_SPEED_MODE,
        .timer_sel = ENGINE_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .sleep_mode = LEDC_SLEEP_MODE_INVALID,
        .duty = 0,
    };

    // configure red channel
    ESP_LOGI(TAG, "Initializing red channel");
    channel_config.channel = RED_CHANNEL;
    channel_config.gpio_num = RED_GPIO;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    // configure green channel
    ESP_LOGI(TAG, "Initializing green channel");
    channel_config.channel = GREEN_CHANNEL;
    channel_config.gpio_num = GREEN_GPIO;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    // configure blue channel
    ESP_LOGI(TAG, "Initializing blue channel");
    channel_config.channel = BLUE_CHANNEL;
    channel_config.gpio_num = BLUE_GPIO;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    // configure whisker channel
    ESP_LOGI(TAG, "Initializing whisker channel");
    channel_config.channel = WHISKER_CHANNEL;
    channel_config.gpio_num = WHISKER_GPIO;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    ESP_LOGI(TAG, "Light engine initialized.");
}

light_engine_t *light_engine_get(void)
{
    return &s_engine;
}

void light_engine_start() {
    ESP_LOGI(TAG, "Starting light engine...");
    BaseType_t result = xTaskCreatePinnedToCore(light_engine_task, "light_engine", 4096, &s_engine, 5, NULL, APP_CPU_NUM);

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create light engine task");
    } else {
        ESP_LOGI(TAG, "Light engine started.");
    }
}

