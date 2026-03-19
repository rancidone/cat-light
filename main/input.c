#include <stdbool.h>

#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "input.h"

static const char *TAG = "input";

static bool s_prev_down = false;

void input_init() {
    ESP_LOGI(TAG, "Initializing input module...");
    gpio_config_t button_config = {
        .pin_bit_mask   = BUTTON_BIT,
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,  // external 10K pull-up (R27) on board
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE,    // polling, no ISR
    };
    ESP_ERROR_CHECK(gpio_config(&button_config));
    ESP_LOGI(TAG, "Input module initialized.");
}

bool button_down() {
    return gpio_get_level(GPIO_NUM_32) == 0; // active low
}

bool button_pressed() {
    bool down = button_down();
    bool pressed = down && !s_prev_down;
    s_prev_down = down;
    return pressed;
}
