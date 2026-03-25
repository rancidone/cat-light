#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#define PERSISTENCE_SCRIPT_SLOT_MIN  1
#define PERSISTENCE_SCRIPT_SLOT_MAX  10
#define PERSISTENCE_SCRIPT_NAME_MAX  64
#define PERSISTENCE_SCRIPT_SRC_MAX   8192   // 8 KB per script

typedef struct {
    uint8_t slot;
    char    name[PERSISTENCE_SCRIPT_NAME_MAX];
    bool    occupied;
} persistence_script_meta_t;

void persistence_init(void);   // mounts LittleFS, opens NVS

// Scripts
esp_err_t persistence_script_write(uint8_t slot, const char *name, const char *src, size_t len);
esp_err_t persistence_script_read(uint8_t slot, char *name_out, char *src_out, size_t src_max, size_t *len_out);
esp_err_t persistence_script_delete(uint8_t slot);
esp_err_t persistence_script_list(persistence_script_meta_t *out, uint8_t max_count, uint8_t *count_out);

// Config (string key/value over NVS)
esp_err_t persistence_config_get(const char *key, char *value_out, size_t len);
esp_err_t persistence_config_set(const char *key, const char *value);

// WiFi credentials (separate NVS namespace)
esp_err_t persistence_wifi_get_credentials(char *ssid_out, size_t ssid_len,
                                            char *pass_out, size_t pass_len);
esp_err_t persistence_wifi_set_credentials(const char *ssid, const char *password);
esp_err_t persistence_wifi_clear_credentials(void);
