idf_component_register(SRCS "main.c" "input.c" "light_engine.c" "behavior.c" "mode_manager.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_ledc esp_driver_gpio)