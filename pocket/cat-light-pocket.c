#include "ch32v003fun/ch32fun/ch32fun.h"
#include "light_engine.h"
#include "modes.h"
#include "timebase.h"

#define FRAME_MS    30u    /* ~33 Hz */
#define DEBOUNCE_MS 50u
#define HOLD_MS     400u   /* tap < 400ms, hold >= 400ms */

/* PC6 = MODE_BTN, active low, external pull-up */
#define BTN_PORT GPIOC
#define BTN_PIN  6u

/* Accumulate tick differences before conversion: SysTick wraps every ~12 min. */
static uint32_t millis(void)
{
    static timebase_t clock;
    return timebase_update(&clock, funSysTick32(), DELAY_MS_TIME);
}

static int s_mode = MODE_HEARTBEAT;

/* 5 brightness levels — index 2 (0.30) is medium/default */
static const float k_brightness[] = { 0.08f, 0.15f, 0.30f, 0.55f, 1.0f };
static int s_bright_idx = 2;

typedef enum { BTN_NONE, BTN_TAP, BTN_HOLD } btn_event_t;

static btn_event_t button_event(void)
{
    static int      prev_down  = 0;
    static uint32_t down_at    = 0;
    static int      hold_fired = 0;

    uint32_t now  = millis();
    int      down = !(BTN_PORT->INDR & (1u << BTN_PIN));

    if (down && !prev_down) {
        /* debounce: ignore glitches shorter than DEBOUNCE_MS */
        if (now - down_at < DEBOUNCE_MS) return BTN_NONE;
        down_at    = now;
        hold_fired = 0;
        prev_down  = 1;
    } else if (!down && prev_down) {
        prev_down = 0;
        if (!hold_fired && (now - down_at) < HOLD_MS)
            return BTN_TAP;
    } else if (down && prev_down && !hold_fired) {
        if ((now - down_at) >= HOLD_MS) {
            hold_fired = 1;
            return BTN_HOLD;
        }
    }
    return BTN_NONE;
}

int main(void)
{
    SystemInit();
    light_engine_init();
    light_engine_set_brightness(k_brightness[s_bright_idx]);

    light_frame_t frame = {0};
    uint32_t last_tick = millis();

    while (1) {
        uint32_t now = millis();
        uint32_t dt  = now - last_tick;
        if (dt < FRAME_MS) continue;
        last_tick = now;

        switch (button_event()) {
        case BTN_TAP:
            s_mode = (s_mode + 1) % MODE_COUNT;
            break;
        case BTN_HOLD:
            s_bright_idx = (s_bright_idx + 1) % 5;
            light_engine_set_brightness(k_brightness[s_bright_idx]);
            break;
        default:
            break;
        }

        modes_tick((mode_id_t)s_mode, &frame, dt);
        light_engine_write(&frame);
    }
}
