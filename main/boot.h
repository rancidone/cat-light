#pragma once
#include <stdbool.h>

typedef struct {
    bool force_softap;   // set by app_main if button held at boot
} boot_flags_t;

extern boot_flags_t g_boot_flags;
