#pragma once
#include <stdint.h>

/* Sample at least once per hardware-counter period (~12 minutes at 6 MHz).
 * Keep fractional milliseconds so frequent sampling does not lose time. */
typedef struct {
    uint32_t last_ticks;
    uint32_t remainder;
    uint32_t ms;
} timebase_t;

static inline uint32_t timebase_update(timebase_t *clock, uint32_t ticks,
                                       uint32_t ticks_per_ms)
{
    uint32_t elapsed = ticks - clock->last_ticks;
    clock->last_ticks = ticks;
    clock->ms += elapsed / ticks_per_ms;
    clock->remainder += elapsed % ticks_per_ms;
    clock->ms += clock->remainder / ticks_per_ms;
    clock->remainder %= ticks_per_ms;
    return clock->ms;
}
