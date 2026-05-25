#include "persistence.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_littlefs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "persistence";

static nvs_handle_t      s_nvs_config;
static nvs_handle_t      s_nvs_wifi;
static SemaphoreHandle_t s_persistence_mutex;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static void slot_lua_path(uint8_t slot, char *buf, size_t buf_len)
{
    snprintf(buf, buf_len, "/littlefs/scripts/slot_%02u.lua", (unsigned)slot);
}

static void slot_name_path(uint8_t slot, char *buf, size_t buf_len)
{
    snprintf(buf, buf_len, "/littlefs/scripts/slot_%02u.name", (unsigned)slot);
}

static bool slot_valid(uint8_t slot)
{
    return slot >= PERSISTENCE_SCRIPT_SLOT_MIN && slot <= PERSISTENCE_SCRIPT_SLOT_MAX;
}

// ---------------------------------------------------------------------------
// persistence_init
// ---------------------------------------------------------------------------

void persistence_init(void)
{
    // 1. Init NVS flash
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition problem (%s), erasing and reinitializing", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // 2. Mount LittleFS
    esp_vfs_littlefs_conf_t conf = {
        .base_path             = "/littlefs",
        .partition_label       = "littlefs",
        .format_if_mount_failed = true,
        .dont_mount            = false,
    };
    ESP_ERROR_CHECK(esp_vfs_littlefs_register(&conf));

    // 3. Create scripts directory if it doesn't exist
    int rc = mkdir("/littlefs/scripts", 0755);
    if (rc != 0 && errno != EEXIST) {
        ESP_LOGE(TAG, "mkdir /littlefs/scripts failed: errno %d", errno);
    }

    // 4. Open NVS namespaces
    ESP_ERROR_CHECK(nvs_open("config", NVS_READWRITE, &s_nvs_config));
    ESP_ERROR_CHECK(nvs_open("wifi",   NVS_READWRITE, &s_nvs_wifi));

    // 5. Create mutex
    s_persistence_mutex = xSemaphoreCreateMutex();
    configASSERT(s_persistence_mutex != NULL);

    ESP_LOGI(TAG, "persistence initialized");
}

// ---------------------------------------------------------------------------
// Script operations
// ---------------------------------------------------------------------------

esp_err_t persistence_script_write(uint8_t slot, const char *name, const char *src, size_t len)
{
    if (!slot_valid(slot) || name == NULL || src == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_persistence_mutex, portMAX_DELAY);

    char lua_path[64];
    char name_path[64];
    slot_lua_path(slot, lua_path, sizeof(lua_path));
    slot_name_path(slot, name_path, sizeof(name_path));

    esp_err_t ret = ESP_OK;

    // Write source file
    FILE *f = fopen(lua_path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open %s for writing", lua_path);
        ret = ESP_FAIL;
        goto done;
    }
    size_t written = fwrite(src, 1, len, f);
    fclose(f);
    if (written != len) {
        ESP_LOGE(TAG, "Short write to %s: %zu of %zu bytes", lua_path, written, len);
        ret = ESP_FAIL;
        goto done;
    }

    // Write name file
    f = fopen(name_path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open %s for writing", name_path);
        ret = ESP_FAIL;
        goto done;
    }
    fputs(name, f);
    fclose(f);

done:
    xSemaphoreGive(s_persistence_mutex);
    return ret;
}

esp_err_t persistence_script_read(uint8_t slot, char *name_out, char *src_out,
                                   size_t src_max, size_t *len_out)
{
    if (!slot_valid(slot) || src_out == NULL || len_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_persistence_mutex, portMAX_DELAY);

    char lua_path[64];
    char name_path[64];
    slot_lua_path(slot, lua_path, sizeof(lua_path));
    slot_name_path(slot, name_path, sizeof(name_path));

    esp_err_t ret = ESP_OK;

    // Read source file
    FILE *f = fopen(lua_path, "rb");
    if (f == NULL) {
        ret = ESP_ERR_NOT_FOUND;
        goto done;
    }
    size_t nread = fread(src_out, 1, src_max, f);
    fclose(f);
    *len_out = nread;

    // Read name file
    if (name_out != NULL) {
        f = fopen(name_path, "r");
        if (f != NULL) {
            if (fgets(name_out, PERSISTENCE_SCRIPT_NAME_MAX, f) == NULL) {
                name_out[0] = '\0';
            }
            fclose(f);
        } else {
            name_out[0] = '\0';
        }
    }

done:
    xSemaphoreGive(s_persistence_mutex);
    return ret;
}

esp_err_t persistence_script_delete(uint8_t slot)
{
    if (!slot_valid(slot)) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_persistence_mutex, portMAX_DELAY);

    char lua_path[64];
    char name_path[64];
    slot_lua_path(slot, lua_path, sizeof(lua_path));
    slot_name_path(slot, name_path, sizeof(name_path));

    // Idempotent: ignore errors if files don't exist
    unlink(lua_path);
    unlink(name_path);

    xSemaphoreGive(s_persistence_mutex);
    return ESP_OK;
}

esp_err_t persistence_script_list(persistence_script_meta_t *out, uint8_t max_count,
                                   uint8_t *count_out)
{
    if (out == NULL || count_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_persistence_mutex, portMAX_DELAY);

    uint8_t found = 0;
    for (uint8_t slot = PERSISTENCE_SCRIPT_SLOT_MIN;
         slot <= PERSISTENCE_SCRIPT_SLOT_MAX && found < max_count;
         slot++) {

        char lua_path[64];
        slot_lua_path(slot, lua_path, sizeof(lua_path));

        FILE *f = fopen(lua_path, "rb");
        if (f == NULL) {
            continue;
        }
        fclose(f);

        out[found].slot     = slot;
        out[found].occupied = true;

        // Read name
        char name_path[64];
        slot_name_path(slot, name_path, sizeof(name_path));
        f = fopen(name_path, "r");
        if (f != NULL) {
            if (fgets(out[found].name, PERSISTENCE_SCRIPT_NAME_MAX, f) == NULL) {
                out[found].name[0] = '\0';
            }
            fclose(f);
        } else {
            out[found].name[0] = '\0';
        }

        found++;
    }

    *count_out = found;

    xSemaphoreGive(s_persistence_mutex);
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Config (NVS)
// ---------------------------------------------------------------------------

esp_err_t persistence_config_get(const char *key, char *value_out, size_t len)
{
    if (key == NULL || value_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_persistence_mutex, portMAX_DELAY);
    esp_err_t err = nvs_get_str(s_nvs_config, key, value_out, &len);
    xSemaphoreGive(s_persistence_mutex);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_ERR_NOT_FOUND;
    }
    return err;
}

esp_err_t persistence_config_set(const char *key, const char *value)
{
    if (key == NULL || value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_persistence_mutex, portMAX_DELAY);
    esp_err_t err = nvs_set_str(s_nvs_config, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(s_nvs_config);
    }
    xSemaphoreGive(s_persistence_mutex);
    return err;
}

// ---------------------------------------------------------------------------
// WiFi credentials (NVS)
// ---------------------------------------------------------------------------

esp_err_t persistence_wifi_get_credentials(char *ssid_out, size_t ssid_len,
                                            char *pass_out, size_t pass_len)
{
    if (ssid_out == NULL || pass_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_persistence_mutex, portMAX_DELAY);

    esp_err_t err = nvs_get_str(s_nvs_wifi, "ssid", ssid_out, &ssid_len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        xSemaphoreGive(s_persistence_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    if (err != ESP_OK) {
        xSemaphoreGive(s_persistence_mutex);
        return err;
    }

    err = nvs_get_str(s_nvs_wifi, "password", pass_out, &pass_len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = ESP_ERR_NOT_FOUND;
    }

    xSemaphoreGive(s_persistence_mutex);
    return err;
}

esp_err_t persistence_wifi_set_credentials(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_persistence_mutex, portMAX_DELAY);

    esp_err_t err = nvs_set_str(s_nvs_wifi, "ssid", ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(s_nvs_wifi, "password", password);
    }
    if (err == ESP_OK) {
        err = nvs_commit(s_nvs_wifi);
    }

    xSemaphoreGive(s_persistence_mutex);
    return err;
}

esp_err_t persistence_wifi_clear_credentials(void)
{
    xSemaphoreTake(s_persistence_mutex, portMAX_DELAY);

    // Erase both keys; ignore NOT_FOUND — we want idempotent behavior
    esp_err_t err1 = nvs_erase_key(s_nvs_wifi, "ssid");
    esp_err_t err2 = nvs_erase_key(s_nvs_wifi, "password");

    esp_err_t err = ESP_OK;
    if (err1 != ESP_OK && err1 != ESP_ERR_NVS_NOT_FOUND) {
        err = err1;
    } else if (err2 != ESP_OK && err2 != ESP_ERR_NVS_NOT_FOUND) {
        err = err2;
    }

    if (err == ESP_OK) {
        err = nvs_commit(s_nvs_wifi);
    }

    xSemaphoreGive(s_persistence_mutex);
    return err;
}
