#include "ch32v003fun/ch32fun/ch32fun.h"
#include "light_engine.h"
#include "modes.h"

#define FRAME_MS     30u    /* ~33 Hz */
#define DEBOUNCE_MS  50u

/* PC6 = MODE_BTN, active low, external pull-up */
#define BTN_PORT GPIOC
#define BTN_PIN  6u

/* SysTick runs at HCLK/8 = 6 MHz; DELAY_MS_TIME = 6000 ticks/ms */
static inline uint32_t millis(void) { return funSysTick32() / DELAY_MS_TIME; }

static int s_mode = MODE_HEARTBEAT;

static int button_pressed(void)
{
    static int      prev_down   = 0;
    static uint32_t debounce_at = 0;
    uint32_t now = millis();
    int down = !(BTN_PORT->INDR & (1u << BTN_PIN));

    if (down && !prev_down && (now - debounce_at) >= DEBOUNCE_MS) {
        prev_down   = 1;
        debounce_at = now;
        return 1;
    }
    if (!down) prev_down = 0;
    return 0;
}

int main(void)
{
    SystemInit();
    light_engine_init();

    light_frame_t frame = {0};
    uint32_t last_tick = millis();

    while (1) {
        uint32_t now = millis();
        uint32_t dt  = now - last_tick;
        if (dt < FRAME_MS) continue;
        last_tick = now;

        if (button_pressed())
            s_mode = (s_mode + 1) % MODE_COUNT;

        modes_tick((mode_id_t)s_mode, &frame, dt);
        light_engine_write(&frame);
    }
}
