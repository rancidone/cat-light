#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"
#include "light_engine.h"

#define MODE_MANAGER_NAME_MAX 64

typedef enum {
    MODE_MANAGER_SOURCE_BUILTIN,
    MODE_MANAGER_SOURCE_SCRIPT,
    MODE_MANAGER_SOURCE_DEV,
} mode_manager_mode_source_t;

typedef struct {
    uint8_t index;
    uint8_t slot;
    bool active;
    mode_manager_mode_source_t source;
    char name[MODE_MANAGER_NAME_MAX];
} mode_manager_mode_info_t;

typedef enum {
    MM_STATE_NORMAL,
    MM_STATE_DEV,
    MM_STATE_ERROR,
} mode_manager_state_t;

void mode_manager_init(void);
void mode_manager_start(void);   // registers tick with light engine

void mode_manager_next(void);
void mode_manager_slot_saved(uint8_t slot);
void mode_manager_slot_deleted(uint8_t slot);
void mode_manager_load_dev(const char *src, size_t len);
void mode_manager_exit_dev(void);
void mode_manager_on_session_connect(void);
void mode_manager_on_session_disconnect(void);
mode_manager_state_t mode_manager_get_state(void);
esp_err_t mode_manager_list_modes(mode_manager_mode_info_t *out, uint8_t max_count,
                                  uint8_t *count_out, uint8_t *active_index_out);
esp_err_t mode_manager_set_active(uint8_t index);
