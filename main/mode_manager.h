#pragma once
#include <stdint.h>
#include <stddef.h>
#include "light_engine.h"

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
