#include "modes.h"
#include <math.h>

#define M_PIf 3.14159265f

/* Simple LCG for spooky mode randomness */
static uint32_t s_rand = 0xdeadbeef;
static uint32_t lcg_rand(void)
{
    s_rand = s_rand * 1664525u + 1013904223u;
    return s_rand;
}

static void hsv_to_rgb(float h, float s, float v,
                       uint8_t *r, uint8_t *g, uint8_t *b)
{
    float fr = 0, fg = 0, fb = 0;
    if (s == 0.0f) {
        fr = fg = fb = v;
    } else {
        float h6 = h * 6.0f;
        int   i  = (int)h6 % 6;
        float f  = h6 - floorf(h6);
        float p  = v * (1.0f - s);
        float q  = v * (1.0f - s * f);
        float t  = v * (1.0f - s * (1.0f - f));
        switch (i) {
            case 0: fr=v;  fg=t;  fb=p;  break;
            case 1: fr=q;  fg=v;  fb=p;  break;
            case 2: fr=p;  fg=v;  fb=t;  break;
            case 3: fr=p;  fg=q;  fb=v;  break;
            case 4: fr=t;  fg=p;  fb=v;  break;
            default:fr=v;  fg=p;  fb=q;  break;
        }
    }
    *r = (uint8_t)(fr * 255.0f);
    *g = (uint8_t)(fg * 255.0f);
    *b = (uint8_t)(fb * 255.0f);
}

/* -------------------------------------------------------------------------
 * Mode tick functions — each owns its own static state
 * -------------------------------------------------------------------------*/

static void fade_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float hue = 0.0f;
    hue += dt_ms * 0.00005f;
    if (hue >= 1.0f) hue -= 1.0f;
    hsv_to_rgb(hue, 0.7f, 0.8f, &frame->eye_r, &frame->eye_g, &frame->eye_b);
    frame->whisker = 0.3f;
}

static void spooky_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t ms_until_change = 0;
    static float    brightness = 1.0f;
    static float    phase = 0.0f;

    if (ms_until_change <= dt_ms) {
        float rand_f = (float)(lcg_rand() % 1000) / 1000.0f;
        brightness = 0.15f + rand_f * 0.85f;
        ms_until_change = 50 + (lcg_rand() % 251);
    } else {
        ms_until_change -= dt_ms;
    }

    frame->eye_r = (uint8_t)(brightness * 255.0f);
    frame->eye_g = 0;
    frame->eye_b = 0;

    phase += dt_ms * 0.002f * M_PIf;
    frame->whisker = (sinf(phase) + 1.0f) * 0.5f * 0.8f;
}

static void rainbow_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float hue = 0.0f;
    hue += dt_ms * 0.0002f;
    if (hue >= 1.0f) hue -= 1.0f;
    hsv_to_rgb(hue, 1.0f, 1.0f, &frame->eye_r, &frame->eye_g, &frame->eye_b);
    frame->whisker = 0.15f;
}

/* -------------------------------------------------------------------------
 * Public
 * -------------------------------------------------------------------------*/

void modes_tick(mode_id_t mode, light_frame_t *frame, uint32_t dt_ms)
{
    switch (mode) {
        case MODE_FADE:    fade_tick(frame, dt_ms);    break;
        case MODE_SPOOKY:  spooky_tick(frame, dt_ms);  break;
        case MODE_RAINBOW: rainbow_tick(frame, dt_ms); break;
        default:           break;
    }
}
