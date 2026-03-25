#include "mode_manager.h"
#include "light_engine.h"
#include "scripting_engine.h"
#include "persistence.h"
#include "input.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_random.h"
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static const char *TAG = "mode_mgr";

/* -------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------*/
typedef enum {
    MODE_KIND_BUILTIN,
    MODE_KIND_SCRIPT,
} mode_kind_t;

typedef struct {
    mode_kind_t  kind;
    const char  *name;
    uint8_t      slot;
    void (*builtin_tick)(light_frame_t *frame, uint32_t dt_ms);
} mode_entry_t;

typedef enum {
    MM_CMD_NEXT,
    MM_CMD_LOAD_DEV,
    MM_CMD_EXIT_DEV,
    MM_CMD_SLOT_SAVED,
    MM_CMD_SLOT_DELETED,
    MM_CMD_SESSION_CONNECT,
    MM_CMD_SESSION_DISCONNECT,
    MM_CMD_DEV_TIMEOUT,
} mm_cmd_type_t;

typedef struct {
    mm_cmd_type_t type;
    uint8_t       slot;
    char         *dev_src;
    size_t        dev_src_len;
} mm_cmd_t;

/* -------------------------------------------------------------------------
 * Internal state
 * -------------------------------------------------------------------------*/
#define MAX_MODES 16

static mode_entry_t  s_modes[MAX_MODES];
static int           s_mode_count  = 0;
static int           s_active_idx  = 0;
static volatile mode_manager_state_t s_state = MM_STATE_NORMAL;
static int           s_dev_idx     = -1;

static QueueHandle_t   s_cmd_queue         = NULL;
static TimerHandle_t   s_dev_timeout_timer = NULL;

static uint32_t s_now_ms = 0;

/* -------------------------------------------------------------------------
 * HSV -> RGB (h=0.0–1.0 full circle, s/v=0.0–1.0)
 * -------------------------------------------------------------------------*/
static void hsv_to_rgb(float h, float s, float v,
                       uint8_t *r_out, uint8_t *g_out, uint8_t *b_out)
{
    float r = 0, g = 0, b = 0;
    if (s == 0.0f) {
        r = g = b = v;
    } else {
        float h6 = h * 6.0f;
        int   i  = (int)h6 % 6;
        float f  = h6 - floorf(h6);
        float p  = v * (1.0f - s);
        float q  = v * (1.0f - s * f);
        float t2 = v * (1.0f - s * (1.0f - f));
        switch (i) {
            case 0: r = v; g = t2; b = p;  break;
            case 1: r = q; g = v;  b = p;  break;
            case 2: r = p; g = v;  b = t2; break;
            case 3: r = p; g = q;  b = v;  break;
            case 4: r = t2;g = p;  b = v;  break;
            default:r = v; g = p;  b = q;  break;
        }
    }
    *r_out = (uint8_t)(r * 255.0f);
    *g_out = (uint8_t)(g * 255.0f);
    *b_out = (uint8_t)(b * 255.0f);
}

/* -------------------------------------------------------------------------
 * Built-in mode tick functions
 * -------------------------------------------------------------------------*/
static void builtin_fade_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float hue = 0.0f;
    hue += dt_ms * 0.00005f;
    if (hue >= 1.0f) hue -= 1.0f;
    hsv_to_rgb(hue, 0.7f, 0.8f, &frame->eye_r, &frame->eye_g, &frame->eye_b);
    frame->whisker = 0.3f;
}

static void builtin_spooky_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t ms_until_change  = 0;
    static float    current_brightness = 1.0f;
    static float    phase = 0.0f;

    if (ms_until_change <= dt_ms) {
        float rand_f = (float)(esp_random() % 1000) / 1000.0f;
        current_brightness = 0.15f + rand_f * 0.85f;
        ms_until_change = 50 + (esp_random() % 251);
    } else {
        ms_until_change -= dt_ms;
    }

    frame->eye_r = (uint8_t)(current_brightness * 255.0f);
    frame->eye_g = 0;
    frame->eye_b = 0;

    phase += dt_ms * 0.002f * (float)M_PI;
    frame->whisker = (sinf(phase) + 1.0f) * 0.5f * 0.8f;
}

static void builtin_rainbow_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float hue = 0.0f;
    hue += dt_ms * 0.0002f;
    if (hue >= 1.0f) hue -= 1.0f;
    hsv_to_rgb(hue, 1.0f, 1.0f, &frame->eye_r, &frame->eye_g, &frame->eye_b);
    frame->whisker = 0.15f;
}

static void builtin_nightlight_tick(light_frame_t *frame, uint32_t dt_ms)
{
    (void)dt_ms;
    frame->eye_r   = 13;
    frame->eye_g   = 10;
    frame->eye_b   = 4;
    frame->whisker = 0.05f;
}

/* -------------------------------------------------------------------------
 * Mode activation
 * -------------------------------------------------------------------------*/
static void activate_mode(int idx)
{
    s_active_idx = idx;
    mode_entry_t *m = &s_modes[idx];
    if (m->kind == MODE_KIND_SCRIPT) {
        static char src_buf[8192];  /* BSS, not stack */
        char name_buf[64];
        size_t len = 0;
        esp_err_t err = persistence_script_read(m->slot, name_buf, src_buf, sizeof(src_buf), &len);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "script slot %d unreadable, skipping", m->slot);
            mode_manager_next();
            return;
        }
        scripting_engine_stop();
        scripting_engine_load(src_buf, len);
    } else {
        scripting_engine_stop();
    }
}

/* -------------------------------------------------------------------------
 * Dev timeout timer callback
 * -------------------------------------------------------------------------*/
static void dev_timeout_cb(TimerHandle_t t)
{
    (void)t;
    mm_cmd_t cmd = { .type = MM_CMD_DEV_TIMEOUT };
    xQueueSend(s_cmd_queue, &cmd, 0);
}

/* -------------------------------------------------------------------------
 * Command handler (called from render task)
 * -------------------------------------------------------------------------*/
static void handle_cmd(mm_cmd_t *cmd)
{
    switch (cmd->type) {

    case MM_CMD_NEXT:
        if (s_state == MM_STATE_DEV) break;
        s_active_idx = (s_active_idx + 1) % s_mode_count;
        activate_mode(s_active_idx);
        break;

    case MM_CMD_LOAD_DEV:
        if (s_dev_idx < 0) {
            if (s_mode_count < MAX_MODES) {
                s_modes[s_mode_count].kind         = MODE_KIND_SCRIPT;
                s_modes[s_mode_count].name         = "Dev";
                s_modes[s_mode_count].slot         = 0;
                s_modes[s_mode_count].builtin_tick = NULL;
                s_dev_idx = s_mode_count;
                s_mode_count++;
            }
        }
        s_active_idx = s_dev_idx;
        scripting_engine_stop();
        scripting_engine_load(cmd->dev_src, cmd->dev_src_len);
        free(cmd->dev_src);
        cmd->dev_src = NULL;
        s_state = MM_STATE_DEV;
        xTimerStop(s_dev_timeout_timer, 0);
        break;

    case MM_CMD_EXIT_DEV:
        scripting_engine_stop();
        if (s_dev_idx >= 0) {
            for (int i = s_dev_idx; i < s_mode_count - 1; i++) {
                s_modes[i] = s_modes[i + 1];
            }
            s_mode_count--;
            s_dev_idx = -1;
        }
        s_active_idx = 0;
        s_state = MM_STATE_NORMAL;
        activate_mode(0);
        xTimerStop(s_dev_timeout_timer, 0);
        break;

    case MM_CMD_SLOT_SAVED: {
        int found = -1;
        for (int i = 0; i < s_mode_count; i++) {
            if (s_modes[i].kind == MODE_KIND_SCRIPT && s_modes[i].slot == cmd->slot) {
                found = i;
                break;
            }
        }
        if (found < 0) {
            int insert_pos = (s_dev_idx >= 0) ? s_dev_idx : s_mode_count;
            if (s_mode_count < MAX_MODES) {
                if (s_dev_idx >= 0) {
                    s_modes[insert_pos + 1] = s_modes[insert_pos];
                    s_dev_idx = insert_pos + 1;
                    if (s_active_idx >= insert_pos) s_active_idx++;
                }
                s_modes[insert_pos].kind         = MODE_KIND_SCRIPT;
                s_modes[insert_pos].name         = "";
                s_modes[insert_pos].slot         = cmd->slot;
                s_modes[insert_pos].builtin_tick = NULL;
                s_mode_count++;
            }
        }
        break;
    }

    case MM_CMD_SLOT_DELETED: {
        int found = -1;
        for (int i = 0; i < s_mode_count; i++) {
            if (s_modes[i].kind == MODE_KIND_SCRIPT && s_modes[i].slot == cmd->slot) {
                found = i;
                break;
            }
        }
        if (found < 0) break;
        bool was_active = (found == s_active_idx);
        if (was_active && s_modes[found].kind == MODE_KIND_SCRIPT) {
            scripting_engine_stop();
        }
        for (int i = found; i < s_mode_count - 1; i++) {
            s_modes[i] = s_modes[i + 1];
        }
        s_mode_count--;
        if (s_dev_idx > found) s_dev_idx--;
        if (was_active) {
            s_active_idx = (found < s_mode_count) ? found : 0;
            activate_mode(s_active_idx);
        } else if (s_active_idx > found) {
            s_active_idx--;
        }
        break;
    }

    case MM_CMD_SESSION_CONNECT:
        xTimerStop(s_dev_timeout_timer, 0);
        break;

    case MM_CMD_SESSION_DISCONNECT:
        if (s_state == MM_STATE_DEV) {
            xTimerStart(s_dev_timeout_timer, 0);
        }
        break;

    case MM_CMD_DEV_TIMEOUT:
        if (s_state == MM_STATE_DEV) {
            s_state = MM_STATE_NORMAL;
            s_active_idx = 0;
            activate_mode(0);
        }
        break;
    }
}

/* -------------------------------------------------------------------------
 * Error tick
 * -------------------------------------------------------------------------*/
static void error_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t acc = 0;
    acc += dt_ms;
    bool eye_on    = (acc % 1000) < 500;
    frame->eye_r   = eye_on ? 255 : 0;
    frame->eye_g   = 0;
    frame->eye_b   = 0;
    frame->whisker = 0.0f;
}

/* -------------------------------------------------------------------------
 * Render tick (mode_tick_fn_t)
 * -------------------------------------------------------------------------*/
static void mode_manager_tick(light_frame_t *frame, uint32_t dt_ms, void *ctx)
{
    (void)ctx;
    s_now_ms += dt_ms;

    mm_cmd_t cmd;
    while (xQueueReceive(s_cmd_queue, &cmd, 0) == pdTRUE) {
        handle_cmd(&cmd);
    }

    /* Button ownership */
    if (button_pressed() && s_state == MM_STATE_NORMAL) {
        s_active_idx = (s_active_idx + 1) % s_mode_count;
        activate_mode(s_active_idx);
    }

    switch (s_state) {
    case MM_STATE_ERROR:
        error_tick(frame, dt_ms);
        return;

    case MM_STATE_NORMAL:
    case MM_STATE_DEV: {
        mode_entry_t *m = &s_modes[s_active_idx];
        if (m->kind == MODE_KIND_BUILTIN) {
            m->builtin_tick(frame, dt_ms);
        } else {
            scripting_engine_tick(frame, s_now_ms, dt_ms);
            if (scripting_engine_status() == SCRIPTING_STATUS_ERROR) {
                s_state = MM_STATE_ERROR;
            }
        }
        break;
    }
    }
}

/* -------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------*/

void mode_manager_init(void)
{
    s_cmd_queue = xQueueCreate(8, sizeof(mm_cmd_t));
    s_dev_timeout_timer = xTimerCreate("dev_timeout",
                                        pdMS_TO_TICKS(30 * 60 * 1000),
                                        pdFALSE, NULL, dev_timeout_cb);

    s_modes[0] = (mode_entry_t){ MODE_KIND_BUILTIN, "Fade",        0, builtin_fade_tick       };
    s_modes[1] = (mode_entry_t){ MODE_KIND_BUILTIN, "Spooky",      0, builtin_spooky_tick     };
    s_modes[2] = (mode_entry_t){ MODE_KIND_BUILTIN, "Rainbow",     0, builtin_rainbow_tick    };
    s_modes[3] = (mode_entry_t){ MODE_KIND_BUILTIN, "Night Light", 0, builtin_nightlight_tick };
    s_mode_count = 4;

    persistence_script_meta_t meta[10];
    uint8_t count = 0;
    persistence_script_list(meta, 10, &count);
    for (int i = 0; i < count && s_mode_count < MAX_MODES - 1; i++) {
        s_modes[s_mode_count].kind         = MODE_KIND_SCRIPT;
        s_modes[s_mode_count].name         = "";
        s_modes[s_mode_count].slot         = meta[i].slot;
        s_modes[s_mode_count].builtin_tick = NULL;
        s_mode_count++;
    }

    s_active_idx = 0;
    s_state      = MM_STATE_NORMAL;
    s_dev_idx    = -1;
}

void mode_manager_start(void)
{
    light_engine_set_mode_tick(mode_manager_tick, NULL);
}

void mode_manager_next(void)
{
    mm_cmd_t cmd = { .type = MM_CMD_NEXT };
    xQueueSend(s_cmd_queue, &cmd, 0);
}

void mode_manager_slot_saved(uint8_t slot)
{
    mm_cmd_t cmd = { .type = MM_CMD_SLOT_SAVED, .slot = slot };
    xQueueSend(s_cmd_queue, &cmd, 0);
}

void mode_manager_slot_deleted(uint8_t slot)
{
    mm_cmd_t cmd = { .type = MM_CMD_SLOT_DELETED, .slot = slot };
    xQueueSend(s_cmd_queue, &cmd, 0);
}

void mode_manager_load_dev(const char *src, size_t len)
{
    char *copy = malloc(len);
    if (!copy) {
        ESP_LOGE(TAG, "OOM for dev src");
        return;
    }
    memcpy(copy, src, len);
    mm_cmd_t cmd = {
        .type        = MM_CMD_LOAD_DEV,
        .dev_src     = copy,
        .dev_src_len = len,
    };
    xQueueSend(s_cmd_queue, &cmd, 0);
}

void mode_manager_exit_dev(void)
{
    mm_cmd_t cmd = { .type = MM_CMD_EXIT_DEV };
    xQueueSend(s_cmd_queue, &cmd, 0);
}

void mode_manager_on_session_connect(void)
{
    mm_cmd_t cmd = { .type = MM_CMD_SESSION_CONNECT };
    xQueueSend(s_cmd_queue, &cmd, 0);
}

void mode_manager_on_session_disconnect(void)
{
    mm_cmd_t cmd = { .type = MM_CMD_SESSION_DISCONNECT };
    xQueueSend(s_cmd_queue, &cmd, 0);
}

mode_manager_state_t mode_manager_get_state(void)
{
    return s_state;
}
