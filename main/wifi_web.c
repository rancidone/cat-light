#include "wifi_web.h"
#include "boot.h"
#include "persistence.h"
#include "mode_manager.h"
#include "light_engine.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "wifi_web";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define STA_TIMEOUT_MS     15000

static EventGroupHandle_t s_wifi_event_group = NULL;
static httpd_handle_t     s_server           = NULL;
static int                s_retry_count      = 0;

/* -------------------------------------------------------------------------
 * Content-Type helper
 * -------------------------------------------------------------------------*/
static const char *content_type_for_path(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css")  == 0) return "text/css";
    if (strcmp(ext, ".js")   == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".ico")  == 0) return "image/x-icon";
    return "application/octet-stream";
}

/* -------------------------------------------------------------------------
 * Static asset handler (wildcard GET)
 * -------------------------------------------------------------------------*/
static esp_err_t static_asset_handler(httpd_req_t *req)
{
    char path[640];
    const char *uri = req->uri;
    /* skip leading slash, default to index.html */
    if (strcmp(uri, "/") == 0) uri = "/index.html";
    snprintf(path, sizeof(path), "/littlefs/www%s", uri);

    FILE *f = fopen(path, "r");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_OK;
    }

    httpd_resp_set_type(req, content_type_for_path(path));

    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        httpd_resp_send_chunk(req, buf, (ssize_t)n);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * Slot parser helper
 * -------------------------------------------------------------------------*/
static int parse_slot_from_uri(const char *uri)
{
    /* URI form: /api/scripts/N or /api/scripts/N/name */
    const char *p = strrchr(uri, '/');
    if (!p) return -1;
    /* if last segment is "name", go one back */
    if (strcmp(p, "/name") == 0) {
        const char *q = p - 1;
        while (q > uri && *q != '/') q--;
        p = q;
    }
    int slot = atoi(p + 1);
    if (slot < 1 || slot > 10) return -1;
    return slot;
}

/* -------------------------------------------------------------------------
 * API: GET /api/scripts
 * -------------------------------------------------------------------------*/
static esp_err_t api_scripts_list(httpd_req_t *req)
{
    persistence_script_meta_t meta[10];
    uint8_t count = 0;
    persistence_script_list(meta, 10, &count);

    cJSON *arr = cJSON_CreateArray();
    /* build full 10-slot list */
    bool occupied[11] = {0};
    char names[11][PERSISTENCE_SCRIPT_NAME_MAX];
    for (int i = 0; i < count; i++) {
        occupied[meta[i].slot] = true;
        strncpy(names[meta[i].slot], meta[i].name, PERSISTENCE_SCRIPT_NAME_MAX - 1);
        names[meta[i].slot][PERSISTENCE_SCRIPT_NAME_MAX - 1] = '\0';
    }
    for (int s = 1; s <= 10; s++) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(obj, "slot", s);
        cJSON_AddStringToObject(obj, "name", occupied[s] ? names[s] : "");
        cJSON_AddBoolToObject(obj, "occupied", occupied[s]);
        cJSON_AddItemToArray(arr, obj);
    }

    char *json = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json ? json : "[]");
    free(json);
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * API: GET /api/scripts/:slot
 * -------------------------------------------------------------------------*/
static esp_err_t api_script_get(httpd_req_t *req)
{
    int slot = parse_slot_from_uri(req->uri);
    if (slot < 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad slot");
        return ESP_OK;
    }

    char *src = malloc(PERSISTENCE_SCRIPT_SRC_MAX);
    if (!src) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_OK;
    }
    char name[PERSISTENCE_SCRIPT_NAME_MAX];
    size_t len = 0;
    esp_err_t err = persistence_script_read(slot, name, src, PERSISTENCE_SCRIPT_SRC_MAX, &len);
    if (err != ESP_OK) {
        free(src);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_OK;
    }
    src[len] = '\0';

    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "slot", slot);
    cJSON_AddStringToObject(obj, "name", name);
    cJSON_AddStringToObject(obj, "src", src);
    free(src);

    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json ? json : "{}");
    free(json);
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * API: PUT /api/scripts/:slot
 * -------------------------------------------------------------------------*/
static esp_err_t api_script_put(httpd_req_t *req)
{
    int slot = parse_slot_from_uri(req->uri);
    if (slot < 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad slot");
        return ESP_OK;
    }

    int total = req->content_len;
    if (total <= 0 || total > 16384) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return ESP_OK;
    }
    char *body = malloc(total + 1);
    if (!body) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_OK;
    }
    int received = httpd_req_recv(req, body, total);
    if (received <= 0) {
        free(body);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Recv failed");
        return ESP_OK;
    }
    body[received] = '\0';

    cJSON *root = cJSON_Parse(body);
    free(body);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad JSON");
        return ESP_OK;
    }

    const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(root, "name"));
    cJSON *src_item  = cJSON_GetObjectItem(root, "src");
    const char *src  = cJSON_GetStringValue(src_item);

    if (!name || !src) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing name/src");
        return ESP_OK;
    }

    esp_err_t err = persistence_script_write(slot, name, src, strlen(src));
    cJSON_Delete(root);

    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
        return ESP_OK;
    }
    mode_manager_slot_saved(slot);
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * API: DELETE /api/scripts/:slot
 * -------------------------------------------------------------------------*/
static esp_err_t api_script_delete(httpd_req_t *req)
{
    int slot = parse_slot_from_uri(req->uri);
    if (slot < 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad slot");
        return ESP_OK;
    }
    persistence_script_delete(slot);
    mode_manager_slot_deleted(slot);
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * API: PATCH /api/scripts/:slot/name
 * -------------------------------------------------------------------------*/
static esp_err_t api_script_patch_name(httpd_req_t *req)
{
    int slot = parse_slot_from_uri(req->uri);
    if (slot < 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad slot");
        return ESP_OK;
    }

    int total = req->content_len;
    if (total <= 0 || total > 512) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return ESP_OK;
    }
    char *body = malloc(total + 1);
    if (!body) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_OK;
    }
    int received = httpd_req_recv(req, body, total);
    if (received <= 0) { free(body); return ESP_OK; }
    body[received] = '\0';

    cJSON *root = cJSON_Parse(body);
    free(body);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad JSON");
        return ESP_OK;
    }
    const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(root, "name"));
    if (!name) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing name");
        return ESP_OK;
    }

    /* Read existing src, rewrite with new name */
    char *src = malloc(PERSISTENCE_SCRIPT_SRC_MAX);
    if (!src) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_OK;
    }
    char old_name[PERSISTENCE_SCRIPT_NAME_MAX];
    size_t len = 0;
    esp_err_t err = persistence_script_read(slot, old_name, src, PERSISTENCE_SCRIPT_SRC_MAX, &len);
    if (err == ESP_OK) {
        persistence_script_write(slot, name, src, len);
        mode_manager_slot_saved(slot);
    }
    free(src);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * API: GET /api/modes
 * -------------------------------------------------------------------------*/
static esp_err_t api_modes_get(httpd_req_t *req)
{
    mode_manager_mode_info_t modes[16];
    uint8_t count = 0;
    uint8_t active_index = 0;
    if (mode_manager_list_modes(modes, 16, &count, &active_index) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Mode snapshot failed");
        return ESP_OK;
    }

    mode_manager_state_t state = mode_manager_get_state();
    const char *state_str = (state == MM_STATE_DEV) ? "dev" :
                            (state == MM_STATE_ERROR) ? "error" : "normal";

    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    cJSON_AddStringToObject(root, "state", state_str);
    cJSON_AddNumberToObject(root, "active_index", active_index);
    for (uint8_t i = 0; i < count; i++) {
        cJSON *obj = cJSON_CreateObject();
        const char *source = (modes[i].source == MODE_MANAGER_SOURCE_DEV) ? "dev" :
                             (modes[i].source == MODE_MANAGER_SOURCE_BUILTIN) ? "builtin" :
                             "script";
        cJSON_AddNumberToObject(obj, "index", modes[i].index);
        cJSON_AddStringToObject(obj, "name", modes[i].name);
        cJSON_AddStringToObject(obj, "source", source);
        if (modes[i].source != MODE_MANAGER_SOURCE_BUILTIN) {
            cJSON_AddNumberToObject(obj, "slot", modes[i].slot);
        }
        cJSON_AddBoolToObject(obj, "active", modes[i].active);
        cJSON_AddItemToArray(arr, obj);
    }
    cJSON_AddItemToObject(root, "modes", arr);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json ? json : "{}");
    free(json);
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * API: POST /api/modes/active
 * -------------------------------------------------------------------------*/
static esp_err_t api_modes_active_post(httpd_req_t *req)
{
    int total = req->content_len;
    if (total <= 0 || total > 128) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return ESP_OK;
    }

    char *body = malloc(total + 1);
    if (!body) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_OK;
    }
    int received = httpd_req_recv(req, body, total);
    if (received <= 0) {
        free(body);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Recv failed");
        return ESP_OK;
    }
    body[received] = '\0';

    cJSON *root = cJSON_Parse(body);
    free(body);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad JSON");
        return ESP_OK;
    }

    cJSON *index_item = cJSON_GetObjectItem(root, "index");
    if (!index_item || !cJSON_IsNumber(index_item)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing index");
        return ESP_OK;
    }

    esp_err_t err = mode_manager_set_active((uint8_t)index_item->valueint);
    cJSON_Delete(root);
    if (err == ESP_ERR_INVALID_ARG) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown mode");
        return ESP_OK;
    }
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Mode switch failed");
        return ESP_OK;
    }

    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * API: POST /api/dev/load
 * -------------------------------------------------------------------------*/
static esp_err_t api_dev_load(httpd_req_t *req)
{
    int total = req->content_len;
    if (total <= 0 || total > 16384) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return ESP_OK;
    }
    char *body = malloc(total + 1);
    if (!body) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_OK;
    }
    int received = httpd_req_recv(req, body, total);
    if (received <= 0) { free(body); return ESP_OK; }
    body[received] = '\0';

    cJSON *root = cJSON_Parse(body);
    free(body);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad JSON");
        return ESP_OK;
    }
    const char *src = cJSON_GetStringValue(cJSON_GetObjectItem(root, "src"));
    if (!src) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing src");
        return ESP_OK;
    }
    mode_manager_load_dev(src, strlen(src));
    mode_manager_on_session_connect();
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * API: POST /api/dev/exit
 * -------------------------------------------------------------------------*/
static esp_err_t api_dev_exit(httpd_req_t *req)
{
    mode_manager_exit_dev();
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * API: GET /api/config
 * -------------------------------------------------------------------------*/
static esp_err_t api_config_get(httpd_req_t *req)
{
    char brightness[32] = "1.0";
    char hostname[64]   = "cat-light";
    persistence_config_get("brightness", brightness, sizeof(brightness));
    persistence_config_get("hostname",   hostname,   sizeof(hostname));

    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "brightness", atof(brightness));
    cJSON_AddStringToObject(obj, "hostname",   hostname);
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json ? json : "{}");
    free(json);
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * API: PUT /api/config
 * -------------------------------------------------------------------------*/
static esp_err_t api_config_put(httpd_req_t *req)
{
    int total = req->content_len;
    if (total <= 0 || total > 512) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return ESP_OK;
    }
    char *body = malloc(total + 1);
    if (!body) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_OK;
    }
    int received = httpd_req_recv(req, body, total);
    if (received <= 0) { free(body); return ESP_OK; }
    body[received] = '\0';

    cJSON *root = cJSON_Parse(body);
    free(body);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad JSON");
        return ESP_OK;
    }

    cJSON *brightness_item = cJSON_GetObjectItem(root, "brightness");
    if (brightness_item && cJSON_IsNumber(brightness_item)) {
        float b = (float)brightness_item->valuedouble;
        char val[32];
        snprintf(val, sizeof(val), "%.4f", b);
        persistence_config_set("brightness", val);
        light_engine_set_brightness(b);
    }

    cJSON *hostname_item = cJSON_GetObjectItem(root, "hostname");
    if (hostname_item) {
        const char *h = cJSON_GetStringValue(hostname_item);
        if (h) persistence_config_set("hostname", h);
    }

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * API: POST /api/wifi/credentials
 * -------------------------------------------------------------------------*/
static esp_err_t api_wifi_credentials(httpd_req_t *req)
{
    int total = req->content_len;
    if (total <= 0 || total > 256) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return ESP_OK;
    }
    char *body = malloc(total + 1);
    if (!body) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_OK;
    }
    int received = httpd_req_recv(req, body, total);
    if (received <= 0) { free(body); return ESP_OK; }
    body[received] = '\0';

    cJSON *root = cJSON_Parse(body);
    free(body);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad JSON");
        return ESP_OK;
    }

    const char *ssid = cJSON_GetStringValue(cJSON_GetObjectItem(root, "ssid"));
    const char *pass = cJSON_GetStringValue(cJSON_GetObjectItem(root, "password"));
    if (!ssid || !pass) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing ssid/password");
        return ESP_OK;
    }
    persistence_wifi_set_credentials(ssid, pass);
    cJSON_Delete(root);

    /* Note: client will lose connection on reconnect */
    httpd_resp_sendstr(req, "{\"message\":\"reconnecting\"}");

    /* Schedule reconnect — simple approach: just reconnect inline */
    esp_wifi_disconnect();
    wifi_config_t wifi_cfg = {0};
    strncpy((char *)wifi_cfg.sta.ssid,     ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, pass, sizeof(wifi_cfg.sta.password) - 1);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    esp_wifi_connect();

    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * HTTP server setup
 * -------------------------------------------------------------------------*/
static void start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&s_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }

    /* API routes — registered before wildcard */
    httpd_uri_t routes[] = {
        { .uri = "/api/scripts",             .method = HTTP_GET,    .handler = api_scripts_list         },
        { .uri = "/api/scripts/*",           .method = HTTP_GET,    .handler = api_script_get           },
        { .uri = "/api/scripts/*",           .method = HTTP_PUT,    .handler = api_script_put           },
        { .uri = "/api/scripts/*",           .method = HTTP_DELETE, .handler = api_script_delete        },
        { .uri = "/api/scripts/*/name",      .method = HTTP_PATCH,  .handler = api_script_patch_name    },
        { .uri = "/api/modes",               .method = HTTP_GET,    .handler = api_modes_get            },
        { .uri = "/api/modes/active",        .method = HTTP_POST,   .handler = api_modes_active_post    },
        { .uri = "/api/dev/load",            .method = HTTP_POST,   .handler = api_dev_load             },
        { .uri = "/api/dev/exit",            .method = HTTP_POST,   .handler = api_dev_exit             },
        { .uri = "/api/config",              .method = HTTP_GET,    .handler = api_config_get           },
        { .uri = "/api/config",              .method = HTTP_PUT,    .handler = api_config_put           },
        { .uri = "/api/wifi/credentials",    .method = HTTP_POST,   .handler = api_wifi_credentials     },
        /* Wildcard last */
        { .uri = "/*",                       .method = HTTP_GET,    .handler = static_asset_handler     },
    };

    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        httpd_register_uri_handler(s_server, &routes[i]);
    }

    ESP_LOGI(TAG, "HTTP server started on port 80");
}

/* -------------------------------------------------------------------------
 * WiFi event handlers
 * -------------------------------------------------------------------------*/
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t event_id, void *data)
{
    if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_count < 1) {
            s_retry_count++;
            ESP_LOGI(TAG, "Retrying WiFi connection...");
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "SoftAP: client connected");
    } else if (base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "SoftAP: client disconnected");
    }
}

/* -------------------------------------------------------------------------
 * SoftAP mode
 * -------------------------------------------------------------------------*/
static void start_softap(void)
{
    ESP_LOGI(TAG, "Starting SoftAP");
    esp_netif_create_default_wifi_ap();

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid            = "cat-light",
            .ssid_len        = 9,
            .channel         = 6,
            .password        = "catlight1",
            .max_connection  = 4,
            .authmode        = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "SoftAP started: SSID=cat-light PW=catlight1");
    start_http_server();
}

/* -------------------------------------------------------------------------
 * STA mode
 * -------------------------------------------------------------------------*/
static void attempt_sta(void)
{
    char ssid[64] = {0};
    char pass[64] = {0};
    if (persistence_wifi_get_credentials(ssid, sizeof(ssid), pass, sizeof(pass)) != ESP_OK) {
        ESP_LOGI(TAG, "No credentials, falling back to SoftAP");
        start_softap();
        return;
    }

    ESP_LOGI(TAG, "Attempting STA connection to %s", ssid);
    esp_netif_create_default_wifi_sta();

    wifi_config_t sta_cfg = {0};
    strncpy((char *)sta_cfg.sta.ssid,     ssid, sizeof(sta_cfg.sta.ssid) - 1);
    strncpy((char *)sta_cfg.sta.password, pass, sizeof(sta_cfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE, pdFALSE,
                                            pdMS_TO_TICKS(STA_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to %s", ssid);
        start_http_server();
    } else {
        ESP_LOGW(TAG, "STA connection failed/timed out, falling back to SoftAP");
        esp_wifi_stop();
        start_softap();
    }
}

/* -------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------*/
void wifi_web_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                wifi_event_handler, NULL));

    if (g_boot_flags.force_softap) {
        start_softap();
    } else {
        char ssid[64] = {0};
        char pass[64] = {0};
        if (persistence_wifi_get_credentials(ssid, sizeof(ssid), pass, sizeof(pass)) != ESP_OK) {
            start_softap();
        } else {
            attempt_sta();
        }
    }
}
