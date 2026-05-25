#include "ch32v003fun/ch32fun/ch32fun.h"
#include "light_engine.h"

/*
 * Board test firmware — cycles through each LED channel independently.
 * Watch the sequence and verify every channel lights up.
 *
 * Sequence (2 seconds each, repeating):
 *   1. All off
 *   2. Red eye      — solid red
 *   3. Green eye    — solid green
 *   4. Blue eye     — solid blue
 *   5. Whiskers     — full white
 *   6. All on       — white eyes + whiskers
 */

#define STEP_MS 2000u

int main(void)
{
    SystemInit();
    light_engine_init();

    light_frame_t frame = {0};
    int step = 0;

    while (1) {
        switch (step % 6) {
        case 0: frame = (light_frame_t){   0,   0,   0, 0.0f }; break; /* all off    */
        case 1: frame = (light_frame_t){ 255,   0,   0, 0.0f }; break; /* red eye    */
        case 2: frame = (light_frame_t){   0, 255,   0, 0.0f }; break; /* green eye  */
        case 3: frame = (light_frame_t){   0,   0, 255, 0.0f }; break; /* blue eye   */
        case 4: frame = (light_frame_t){   0,   0,   0, 1.0f }; break; /* whiskers   */
        case 5: frame = (light_frame_t){ 255, 255, 255, 1.0f }; break; /* all on     */
        }

        light_engine_write(&frame);
        Delay_Ms(STEP_MS);
        step++;
    }
}
