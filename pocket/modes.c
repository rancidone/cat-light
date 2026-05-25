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

static void heartbeat_tick(light_frame_t *frame, uint32_t dt_ms)
{
    /* Two quick pulses then a long pause — lub-dub */
    static uint32_t acc = 0;
    acc += dt_ms;
    uint32_t t = acc % 1000;  /* 1 second cycle */

    float v = 0.0f;
    if      (t < 80)  v = (float)t / 80.0f;            /* lub up   */
    else if (t < 160) v = (float)(160 - t) / 80.0f;    /* lub down */
    else if (t < 240) v = (float)(t - 160) / 80.0f;    /* dub up   */
    else if (t < 320) v = (float)(320 - t) / 80.0f;    /* dub down */
    /* 320–1000 ms: idle */

    frame->eye_r   = (uint8_t)(255 * v);
    frame->eye_g   = 0;
    frame->eye_b   = (uint8_t)(30 * v);  /* faint blue tint */
    frame->whisker = v * 0.3f;
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

    phase += dt_ms * 0.0008f * M_PIf;
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

/* Candle — warm amber flicker on eyes, slow low whisker glow */
static void candle_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t ms_until_change = 0;
    static float    flicker = 1.0f;
    static float    target  = 1.0f;

    if (ms_until_change <= dt_ms) {
        target = 0.4f + (float)(lcg_rand() % 1000) / 1000.0f * 0.6f;
        ms_until_change = 30 + (lcg_rand() % 120);
    } else {
        ms_until_change -= dt_ms;
    }
    /* Smooth toward target */
    flicker += (target - flicker) * (dt_ms * 0.015f);

    frame->eye_r = (uint8_t)(255 * flicker);
    frame->eye_g = (uint8_t)(80  * flicker);
    frame->eye_b = 0;
    frame->whisker = 0.06f;
}

/* Breathing — slow sine pulse in warm white */
static void breathing_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float phase = 0.0f;
    phase += dt_ms * 0.0008f * M_PIf;
    float v = (sinf(phase) + 1.0f) * 0.5f;
    frame->eye_r = (uint8_t)(220 * v);
    frame->eye_g = (uint8_t)(180 * v);
    frame->eye_b = (uint8_t)(120 * v);
    frame->whisker = v * 0.4f;
}

/* Fire — chaotic red/orange with flickering whiskers */
static void fire_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t ms_r = 0, ms_g = 0, ms_w = 0;
    static float vr = 1.0f, vg = 0.3f, vw = 0.5f;
    static float tr = 1.0f, tg = 0.3f, tw = 0.5f;

    if (ms_r <= dt_ms) { tr = 0.6f + (float)(lcg_rand() % 1000)/1000.0f * 0.4f; ms_r = 20 + lcg_rand()%80; } else ms_r -= dt_ms;
    if (ms_g <= dt_ms) { tg = (float)(lcg_rand() % 1000)/1000.0f * 0.45f;        ms_g = 40 + lcg_rand()%100;} else ms_g -= dt_ms;
    if (ms_w <= dt_ms) { tw = (float)(lcg_rand() % 1000)/1000.0f * 0.7f;         ms_w = 25 + lcg_rand()%90; } else ms_w -= dt_ms;

    float k = dt_ms * 0.02f;
    vr += (tr - vr) * k; vg += (tg - vg) * k; vw += (tw - vw) * k;

    frame->eye_r = (uint8_t)(255 * vr);
    frame->eye_g = (uint8_t)(255 * vg);
    frame->eye_b = 0;
    frame->whisker = vw;
}

/* Police — alternating red/blue strobe, whiskers on */
static void police_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t acc = 0;
    acc += dt_ms;
    uint32_t phase = (acc / 100) % 4;  /* 100 ms per phase, 4 phases */
    frame->eye_r   = (phase == 0 || phase == 1) ? 255 : 0;
    frame->eye_b   = (phase == 2 || phase == 3) ? 255 : 0;
    frame->eye_g   = 0;
    frame->whisker = 0.5f;
}

/* Disco — rapid random color pops */
static void disco_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t ms_until_change = 0;
    if (ms_until_change <= dt_ms) {
        uint32_t r = lcg_rand();
        hsv_to_rgb((float)(r % 1000) / 1000.0f, 1.0f, 1.0f,
                   &frame->eye_r, &frame->eye_g, &frame->eye_b);
        ms_until_change = 80 + (lcg_rand() % 120);
    } else {
        ms_until_change -= dt_ms;
    }
    frame->whisker = (lcg_rand() % 2) ? 0.8f : 0.0f;
}

/* Aurora — slow drifting pastels, gentle whisper of whiskers */
static void aurora_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float hue   = 0.0f;
    static float phase = 0.0f;
    hue   += dt_ms * 0.00003f;
    phase += dt_ms * 0.0005f * M_PIf;
    if (hue >= 1.0f) hue -= 1.0f;
    float v = 0.5f + sinf(phase) * 0.3f;
    hsv_to_rgb(hue, 0.5f, v, &frame->eye_r, &frame->eye_g, &frame->eye_b);
    frame->whisker = (sinf(phase * 0.7f) + 1.0f) * 0.15f;
}

/* Ocean — deep blue/teal sine waves */
static void ocean_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float p1 = 0.0f, p2 = 0.7f;
    p1 += dt_ms * 0.0006f * M_PIf;
    p2 += dt_ms * 0.0004f * M_PIf;
    float wave = (sinf(p1) + sinf(p2) * 0.5f + 1.5f) / 3.0f;
    frame->eye_r = (uint8_t)(10 * wave);
    frame->eye_g = (uint8_t)(120 * wave);
    frame->eye_b = (uint8_t)(255 * wave);
    frame->whisker = wave * 0.2f;
}

/* Lightning — long dark silence then a sharp crack */
static void lightning_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t ms_until_flash = 800;
    static uint32_t flash_remaining = 0;

    if (flash_remaining > 0) {
        flash_remaining = flash_remaining > dt_ms ? flash_remaining - dt_ms : 0;
        float v = (float)flash_remaining / 60.0f;
        frame->eye_r = frame->eye_g = frame->eye_b = (uint8_t)(255 * v);
        frame->whisker = v;
    } else {
        frame->eye_r = frame->eye_g = frame->eye_b = 0;
        frame->whisker = 0.0f;
        if (ms_until_flash <= dt_ms) {
            flash_remaining = 60;
            ms_until_flash = 1500 + (lcg_rand() % 3000);
        } else {
            ms_until_flash -= dt_ms;
        }
    }
}

/* Firefly — random brief blinks against darkness */
static void firefly_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t ms_until_blink = 500;
    static uint32_t blink_remaining = 0;

    if (blink_remaining > 0) {
        blink_remaining = blink_remaining > dt_ms ? blink_remaining - dt_ms : 0;
        float v = sinf((float)blink_remaining * M_PIf / 200.0f);
        frame->eye_r = (uint8_t)(180 * v);
        frame->eye_g = (uint8_t)(255 * v);
        frame->eye_b = (uint8_t)(60  * v);
        frame->whisker = v * 0.15f;
    } else {
        frame->eye_r = frame->eye_g = frame->eye_b = 0;
        frame->whisker = 0.0f;
        if (ms_until_blink <= dt_ms) {
            blink_remaining = 200;
            ms_until_blink = 300 + (lcg_rand() % 1500);
        } else {
            ms_until_blink -= dt_ms;
        }
    }
}

/* SOS — morse code in white, whiskers flash on dashes */
static void sos_tick(light_frame_t *frame, uint32_t dt_ms)
{
    /* . . . — — — . . .  (on_ms, off_ms) */
    static const uint16_t pat[][2] = {
        {100,100},{100,100},{100,300},   /* S */
        {300,100},{300,100},{300,300},   /* O */
        {100,100},{100,100},{100,700},   /* S + word gap */
    };
    static int     idx = 0;
    static uint32_t acc = 0;
    static int     phase = 0; /* 0=on 1=off */

    acc += dt_ms;
    uint32_t dur = pat[idx][phase];
    if (acc >= dur) {
        acc -= dur;
        phase ^= 1;
        if (phase == 0) idx = (idx + 1) % 9;
    }

    uint8_t on = (phase == 0) ? 255 : 0;
    frame->eye_r = frame->eye_g = frame->eye_b = on;
    frame->whisker = (phase == 0 && pat[idx][0] == 300) ? 0.8f : 0.0f;
}

/* Lava — very slow red/orange blobs */
static void lava_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float p1 = 0.0f, p2 = 1.2f;
    p1 += dt_ms * 0.0002f * M_PIf;
    p2 += dt_ms * 0.00013f * M_PIf;
    float v = (sinf(p1) + sinf(p2) + 2.0f) / 4.0f;
    frame->eye_r = (uint8_t)(255 * v);
    frame->eye_g = (uint8_t)(40  * v * v);
    frame->eye_b = 0;
    frame->whisker = v * 0.1f;
}

/* Alien — eerie green pulse with sharp attack, slow decay */
static void alien_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float phase = 0.0f;
    phase += dt_ms * 0.0012f * M_PIf;
    float raw = sinf(phase);
    float v = raw > 0.0f ? raw * raw : 0.0f; /* rectified square — sharp pulses */
    frame->eye_r = (uint8_t)(20  * v);
    frame->eye_g = (uint8_t)(255 * v);
    frame->eye_b = (uint8_t)(40  * v);
    frame->whisker = v * 0.25f;
}

/* Sunset — hue sweeps red→orange→pink→purple, slow */
static void sunset_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float hue = 0.0f;
    hue += dt_ms * 0.000025f;
    if (hue >= 0.833f) hue = 0.0f; /* wrap at purple back to red */
    hsv_to_rgb(hue, 1.0f, 1.0f, &frame->eye_r, &frame->eye_g, &frame->eye_b);
    frame->whisker = 0.12f;
}

/* Matrix — random flickering greens, like digital rain */
static void matrix_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t ms_until_change = 0;
    static uint8_t  g = 0;
    if (ms_until_change <= dt_ms) {
        g = (lcg_rand() % 3 == 0) ? (uint8_t)(100 + lcg_rand() % 156) : 0;
        ms_until_change = 40 + (lcg_rand() % 120);
    } else {
        ms_until_change -= dt_ms;
    }
    frame->eye_r = 0;
    frame->eye_g = g;
    frame->eye_b = 0;
    frame->whisker = (g > 0) ? 0.05f : 0.0f;
}

/* Comet — sharp rise, exponential decay, slow hue drift */
static void comet_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float    tail  = 0.0f;
    static float    hue   = 0.0f;
    static uint32_t ms_until_strike = 600;

    tail *= (1.0f - dt_ms * 0.005f);
    if (ms_until_strike <= dt_ms) {
        tail = 1.0f;
        hue += 0.13f;
        if (hue >= 1.0f) hue -= 1.0f;
        ms_until_strike = 800 + (lcg_rand() % 1200);
    } else {
        ms_until_strike -= dt_ms;
    }
    hsv_to_rgb(hue, 0.8f, tail, &frame->eye_r, &frame->eye_g, &frame->eye_b);
    frame->whisker = tail * 0.5f;
}

/* Thunderstorm — mostly dark, dim blue ambience, occasional lightning */
static void thunderstorm_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t ms_until_flash  = 2000;
    static uint32_t flash_remaining = 0;
    static float    rumble = 0.0f;

    rumble += dt_ms * 0.002f * M_PIf;
    float ambient = (sinf(rumble) + 1.0f) * 0.03f;

    if (flash_remaining > 0) {
        flash_remaining = flash_remaining > dt_ms ? flash_remaining - dt_ms : 0;
        float v = (float)flash_remaining / 80.0f;
        frame->eye_r = (uint8_t)(200 * v);
        frame->eye_g = (uint8_t)(210 * v);
        frame->eye_b = 255;
        frame->whisker = v * 0.9f;
    } else {
        frame->eye_r = 0;
        frame->eye_g = (uint8_t)(10 * ambient);
        frame->eye_b = (uint8_t)(40 * ambient);
        frame->whisker = ambient * 0.3f;
        if (ms_until_flash <= dt_ms) {
            flash_remaining = 80;
            ms_until_flash = 1000 + (lcg_rand() % 4000);
        } else {
            ms_until_flash -= dt_ms;
        }
    }
}

/* Neon — hard cuts between vivid saturated hues */
static void neon_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static const float hues[] = { 0.0f, 0.33f, 0.55f, 0.75f, 0.92f };
    static uint32_t ms_until_change = 400;
    static int      idx = 0;

    if (ms_until_change <= dt_ms) {
        idx = (idx + 1) % 5;
        ms_until_change = 200 + (lcg_rand() % 400);
    } else {
        ms_until_change -= dt_ms;
    }
    hsv_to_rgb(hues[idx], 1.0f, 1.0f, &frame->eye_r, &frame->eye_g, &frame->eye_b);
    frame->whisker = 0.3f;
}

/* Zombie — sickly green/grey slow pulse */
static void zombie_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float phase = 0.0f;
    phase += dt_ms * 0.0007f * M_PIf;
    float v = (sinf(phase) + 1.0f) * 0.5f;
    frame->eye_r = (uint8_t)(40  * v);
    frame->eye_g = (uint8_t)(160 * v);
    frame->eye_b = (uint8_t)(20  * v);
    frame->whisker = v * 0.08f;
}

/* Christmas — alternating red/green */
static void christmas_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t acc = 0;
    acc += dt_ms;
    int red = (acc / 600) % 2;
    frame->eye_r   = red ? 255 : 0;
    frame->eye_g   = red ? 0   : 200;
    frame->eye_b   = 0;
    frame->whisker = 0.25f;
}

/* Galaxy — slow purple/blue drift with twinkling whiskers */
static void galaxy_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static float hue   = 0.67f;
    static float phase = 0.0f;
    hue   += dt_ms * 0.000015f;
    phase += dt_ms * 0.0009f * M_PIf;
    if (hue >= 0.85f) hue = 0.67f;
    float v = 0.6f + sinf(phase) * 0.25f;
    hsv_to_rgb(hue, 0.9f, v, &frame->eye_r, &frame->eye_g, &frame->eye_b);
    frame->whisker = (sinf(phase * 1.7f) + 1.0f) * 0.1f;
}

/* Strobe — pure white flash at ~8 Hz */
static void strobe_tick(light_frame_t *frame, uint32_t dt_ms)
{
    static uint32_t acc = 0;
    acc += dt_ms;
    int on = ((acc / 60) % 2) == 0;  /* 60ms on, 60ms off */
    frame->eye_r = frame->eye_g = frame->eye_b = on ? 255 : 0;
    frame->whisker = on ? 1.0f : 0.0f;
}

/* -------------------------------------------------------------------------
 * Public
 * -------------------------------------------------------------------------*/

void modes_tick(mode_id_t mode, light_frame_t *frame, uint32_t dt_ms)
{
    switch (mode) {
        case MODE_HEARTBEAT: heartbeat_tick(frame, dt_ms);  break;
        case MODE_SPOOKY:    spooky_tick(frame, dt_ms);    break;
        case MODE_RAINBOW:   rainbow_tick(frame, dt_ms);   break;
        case MODE_CANDLE:    candle_tick(frame, dt_ms);    break;
        case MODE_BREATHING: breathing_tick(frame, dt_ms); break;
        case MODE_FIRE:      fire_tick(frame, dt_ms);      break;
        case MODE_POLICE:    police_tick(frame, dt_ms);    break;
        case MODE_DISCO:     disco_tick(frame, dt_ms);     break;
        case MODE_AURORA:        aurora_tick(frame, dt_ms);        break;
        case MODE_OCEAN:         ocean_tick(frame, dt_ms);         break;
        case MODE_LIGHTNING:     lightning_tick(frame, dt_ms);     break;
        case MODE_FIREFLY:       firefly_tick(frame, dt_ms);       break;
        case MODE_SOS:           sos_tick(frame, dt_ms);           break;
        case MODE_LAVA:          lava_tick(frame, dt_ms);          break;
        case MODE_ALIEN:         alien_tick(frame, dt_ms);         break;
        case MODE_SUNSET:        sunset_tick(frame, dt_ms);        break;
        case MODE_MATRIX:        matrix_tick(frame, dt_ms);        break;
        case MODE_COMET:         comet_tick(frame, dt_ms);         break;
        case MODE_THUNDERSTORM:  thunderstorm_tick(frame, dt_ms);  break;
        case MODE_NEON:          neon_tick(frame, dt_ms);          break;
        case MODE_ZOMBIE:        zombie_tick(frame, dt_ms);        break;
        case MODE_CHRISTMAS:     christmas_tick(frame, dt_ms);     break;
        case MODE_GALAXY:        galaxy_tick(frame, dt_ms);        break;
        case MODE_STROBE:        strobe_tick(frame, dt_ms);        break;
        default:                 break;
    }
}
