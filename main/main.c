#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "input.h"
#include "light_engine.h"
#include "mode_manager.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing cat-light...");
    input_init();
    light_engine_init();
    light_mode_manager_apply(light_engine_get(), LIGHT_MODE_DEBUG_PULSE);
    light_engine_start();
    ESP_LOGI(TAG, "cat-light initialized.");

    while (true) {
        if (button_pressed()) {
            mode_manager_cycle(light_engine_get());
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
