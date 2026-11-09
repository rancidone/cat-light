#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../timebase.h"

#define TICKS_PER_MS 6000u

static void test_fractional_samples_preserve_elapsed_time(void)
{
    timebase_t clock = {0};
    assert(timebase_update(&clock, 5999, TICKS_PER_MS) == 0);
    assert(timebase_update(&clock, 6000, TICKS_PER_MS) == 1);
    assert(timebase_update(&clock, 9000, TICKS_PER_MS) == 1);
    assert(timebase_update(&clock, 12000, TICKS_PER_MS) == 2);
}

static void test_frame_elapsed_across_hardware_rollover(void)
{
    uint32_t ticks = UINT32_MAX - 90000u;
    timebase_t clock = { .last_ticks = ticks };
    uint32_t before = timebase_update(&clock, ticks, TICKS_PER_MS);
    ticks += 30u * TICKS_PER_MS;
    uint32_t after = timebase_update(&clock, ticks, TICKS_PER_MS);
    assert(after - before == 30u);
}

static void test_hour_of_samples_has_no_drift_across_multiple_wraps(void)
{
    timebase_t clock = {0};
    /* Sample at a fractional-millisecond interval, like the busy main loop. */
    uint64_t ticks = 0;
    const uint64_t end = (uint64_t)3600000 * TICKS_PER_MS;
    while (ticks < end) {
        ticks += 17321;
        assert(timebase_update(&clock, (uint32_t)ticks, TICKS_PER_MS)
               == (uint32_t)(ticks / TICKS_PER_MS));
    }
}

static void test_button_elapsed_across_millisecond_rollover(void)
{
    timebase_t clock = { .ms = UINT32_MAX - 199u };
    uint32_t pressed_at = clock.ms;
    uint32_t now = timebase_update(&clock, 399u * TICKS_PER_MS, TICKS_PER_MS);
    assert(now - pressed_at == 399u);
    now = timebase_update(&clock, 400u * TICKS_PER_MS, TICKS_PER_MS);
    assert(now - pressed_at == 400u);
}

int main(void)
{
    test_fractional_samples_preserve_elapsed_time();
    test_frame_elapsed_across_hardware_rollover();
    test_hour_of_samples_has_no_drift_across_multiple_wraps();
    test_button_elapsed_across_millisecond_rollover();
    puts("Pocket timebase: all checks passed");
}
