#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "boot.h"
#include "input.h"
#include "light_engine.h"
#include "mode_manager.h"
#include "persistence.h"
#include "scripting_engine.h"
#include "wifi_web.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing cat-light...");
    input_init();

    // Hard reset: check button held at boot
    if (button_down()) {
        persistence_init();
        persistence_wifi_clear_credentials();
        g_boot_flags.force_softap = true;
    } else {
        persistence_init();
    }

    light_engine_init();
    scripting_engine_init();
    mode_manager_init();   // queries persistence for script list
    mode_manager_start();  // registers tick with light engine
    light_engine_start();  // starts render task
    wifi_web_init();       // starts WiFi + HTTP server last
    ESP_LOGI(TAG, "cat-light initialized.");
}
