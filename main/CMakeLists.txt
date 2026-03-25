idf_component_register(
    SRCS "main.c" "boot.c" "input.c" "light_engine.c"
         "persistence.c" "scripting_engine.c" "mode_manager.c"
         "wifi_web.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_ledc esp_driver_gpio nvs_flash joltwallet__littlefs
             georgik__lua esp_wifi esp_event esp_netif esp_http_server json
)
