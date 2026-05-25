#include "scripting_engine.h"
#include "esp_log.h"
#include "esp_random.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <string.h>
#include <math.h>

static const char *TAG = "scripting";

/* -------------------------------------------------------------------------
 * Internal state
 * -------------------------------------------------------------------------*/
static lua_State *s_L = NULL;
static scripting_engine_status_t s_status = SCRIPTING_STATUS_IDLE;

typedef enum { TIER_NONE, TIER_LOOP, TIER_FRAME } script_tier_t;
static script_tier_t s_tier = TIER_NONE;

static uint32_t s_start_ms       = 0;
static uint32_t s_sleep_until_ms = 0;
static uint32_t s_current_tick_ms = 0;  // updated each tick; used by sleep()

static light_frame_t *s_frame = NULL;   // valid only during tick

/* -------------------------------------------------------------------------
 * HSV -> RGB helper (h=0-360, s/v=0.0-1.0, returns r/g/b 0-255)
 * -------------------------------------------------------------------------*/
static void hsv_to_rgb(float h, float s, float v,
                       uint8_t *r_out, uint8_t *g_out, uint8_t *b_out)
{
    float r = 0, g = 0, b = 0;
    if (s == 0.0f) {
        r = g = b = v;
    } else {
        int   i  = (int)(h / 60.0f) % 6;
        float f  = (h / 60.0f) - floorf(h / 60.0f);
        float p  = v * (1.0f - s);
        float q  = v * (1.0f - s * f);
        float t  = v * (1.0f - s * (1.0f - f));
        switch (i) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
    }
    *r_out = (uint8_t)(r * 255.0f);
    *g_out = (uint8_t)(g * 255.0f);
    *b_out = (uint8_t)(b * 255.0f);
}

/* -------------------------------------------------------------------------
 * C primitives
 * -------------------------------------------------------------------------*/

static int l_eye_set(lua_State *L)
{
    if (!s_frame) return 0;
    int r = (int)luaL_checknumber(L, 1);
    int g = (int)luaL_checknumber(L, 2);
    int b = (int)luaL_checknumber(L, 3);
    s_frame->eye_r = (uint8_t)(r < 0 ? 0 : r > 255 ? 255 : r);
    s_frame->eye_g = (uint8_t)(g < 0 ? 0 : g > 255 ? 255 : g);
    s_frame->eye_b = (uint8_t)(b < 0 ? 0 : b > 255 ? 255 : b);
    return 0;
}

static int l_eye_get(lua_State *L)
{
    if (!s_frame) {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
    } else {
        lua_pushnumber(L, s_frame->eye_r);
        lua_pushnumber(L, s_frame->eye_g);
        lua_pushnumber(L, s_frame->eye_b);
    }
    return 3;
}

static int l_eye_flicker(lua_State *L)
{
    if (!s_frame) return 0;
    int r = (int)luaL_checknumber(L, 1);
    int g = (int)luaL_checknumber(L, 2);
    int b = (int)luaL_checknumber(L, 3);
    float scale = 0.2f + (float)(esp_random() % 1000) / 1250.0f;
    s_frame->eye_r = (uint8_t)((r < 0 ? 0 : r > 255 ? 255 : r) * scale);
    s_frame->eye_g = (uint8_t)((g < 0 ? 0 : g > 255 ? 255 : g) * scale);
    s_frame->eye_b = (uint8_t)((b < 0 ? 0 : b > 255 ? 255 : b) * scale);
    return 0;
}

static int l_eye_rainbow(lua_State *L)
{
    if (!s_frame) return 0;
    uint32_t period_ms = (uint32_t)luaL_checknumber(L, 1);
    if (period_ms == 0) period_ms = 1;
    uint32_t elapsed = s_current_tick_ms - s_start_ms;
    float hue = (float)(elapsed % period_ms) / (float)period_ms * 360.0f;
    hsv_to_rgb(hue, 1.0f, 1.0f, &s_frame->eye_r, &s_frame->eye_g, &s_frame->eye_b);
    return 0;
}

static int l_whisker_set(lua_State *L)
{
    if (!s_frame) return 0;
    float v = (float)luaL_checknumber(L, 1);
    s_frame->whisker = v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
    return 0;
}

static int l_whisker_get(lua_State *L)
{
    lua_pushnumber(L, s_frame ? s_frame->whisker : 0.0f);
    return 1;
}

static int l_sleep(lua_State *L)
{
    uint32_t ms = (uint32_t)luaL_checknumber(L, 1);
    s_sleep_until_ms = s_current_tick_ms + ms;
    return lua_yield(L, 0);
}

static int l_time(lua_State *L)
{
    lua_pushnumber(L, (double)(s_current_tick_ms - s_start_ms));
    return 1;
}

static int l_hsv(lua_State *L)
{
    float h = (float)luaL_checknumber(L, 1);
    float s = (float)luaL_checknumber(L, 2);
    float v = (float)luaL_checknumber(L, 3);
    uint8_t r, g, b;
    hsv_to_rgb(h, s, v, &r, &g, &b);
    lua_pushnumber(L, r);
    lua_pushnumber(L, g);
    lua_pushnumber(L, b);
    return 3;
}

/* -------------------------------------------------------------------------
 * Preloaded Lua source for fade/pulse
 * -------------------------------------------------------------------------*/
static const char *FADE_LUA_SRC =
    "function eye.fade(r, g, b, ms)\n"
    "    local steps = math.max(1, math.floor(ms / 30))\n"
    "    local step_ms = math.floor(ms / steps)\n"
    "    local sr, sg, sb = eye._get()\n"
    "    for i = 1, steps do\n"
    "        local t = i / steps\n"
    "        eye.set(\n"
    "            math.floor(sr + (r - sr) * t),\n"
    "            math.floor(sg + (g - sg) * t),\n"
    "            math.floor(sb + (b - sb) * t)\n"
    "        )\n"
    "        sleep(step_ms)\n"
    "    end\n"
    "end\n"
    "function eye.pulse(r, g, b, ms)\n"
    "    eye.fade(r, g, b, ms / 2)\n"
    "    eye.fade(0, 0, 0, ms / 2)\n"
    "end\n"
    "function whisker.fade(brightness, ms)\n"
    "    local steps = math.max(1, math.floor(ms / 30))\n"
    "    local step_ms = math.floor(ms / steps)\n"
    "    local sw = whisker._get()\n"
    "    for i = 1, steps do\n"
    "        local t = i / steps\n"
    "        whisker.set(sw + (brightness - sw) * t)\n"
    "        sleep(step_ms)\n"
    "    end\n"
    "end\n"
    "function whisker.pulse(brightness, ms)\n"
    "    whisker.fade(brightness, ms / 2)\n"
    "    whisker.fade(0.0, ms / 2)\n"
    "end\n";

/* -------------------------------------------------------------------------
 * Register primitives into a fresh lua_State
 * -------------------------------------------------------------------------*/
static void register_primitives(lua_State *L)
{
    /* eye table */
    lua_newtable(L);
    lua_pushcfunction(L, l_eye_set);     lua_setfield(L, -2, "set");
    lua_pushcfunction(L, l_eye_get);     lua_setfield(L, -2, "_get");
    lua_pushcfunction(L, l_eye_flicker); lua_setfield(L, -2, "flicker");
    lua_pushcfunction(L, l_eye_rainbow); lua_setfield(L, -2, "rainbow");
    lua_setglobal(L, "eye");

    /* whisker table */
    lua_newtable(L);
    lua_pushcfunction(L, l_whisker_set); lua_setfield(L, -2, "set");
    lua_pushcfunction(L, l_whisker_get); lua_setfield(L, -2, "_get");
    lua_setglobal(L, "whisker");

    /* globals */
    lua_pushcfunction(L, l_sleep); lua_setglobal(L, "sleep");
    lua_pushcfunction(L, l_time);  lua_setglobal(L, "time");
    lua_pushcfunction(L, l_hsv);   lua_setglobal(L, "hsv");
}

static void open_safe_libs(lua_State *L)
{
    luaL_requiref(L, "_G",      luaopen_base,   1); lua_pop(L, 1);
    luaL_requiref(L, "math",    luaopen_math,   1); lua_pop(L, 1);
    luaL_requiref(L, "string",  luaopen_string, 1); lua_pop(L, 1);
    luaL_requiref(L, "table",   luaopen_table,  1); lua_pop(L, 1);
}

static lua_State *new_vm(void)
{
    lua_State *L = luaL_newstate();
    if (!L) return NULL;
    open_safe_libs(L);
    register_primitives(L);
    if (luaL_dostring(L, FADE_LUA_SRC) != LUA_OK) {
        ESP_LOGE(TAG, "fade preload error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return L;
}

/* -------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------*/

void scripting_engine_init(void)
{
    s_L = new_vm();
    s_status = SCRIPTING_STATUS_IDLE;
    s_tier   = TIER_NONE;
}

void scripting_engine_load(const char *src, size_t len)
{
    /* Fresh VM */
    if (s_L) { lua_close(s_L); s_L = NULL; }
    s_L = new_vm();
    if (!s_L) {
        ESP_LOGE(TAG, "failed to create lua_State");
        s_status = SCRIPTING_STATUS_ERROR;
        return;
    }

    s_tier           = TIER_NONE;
    s_sleep_until_ms = 0;
    s_start_ms       = s_current_tick_ms;

    /* Load and execute chunk to define globals */
    if (luaL_loadbuffer(s_L, src, len, "script") != LUA_OK) {
        ESP_LOGE(TAG, "load error: %s", lua_tostring(s_L, -1));
        lua_pop(s_L, 1);
        s_status = SCRIPTING_STATUS_ERROR;
        return;
    }
    if (lua_pcall(s_L, 0, 0, 0) != LUA_OK) {
        ESP_LOGE(TAG, "exec error: %s", lua_tostring(s_L, -1));
        lua_pop(s_L, 1);
        s_status = SCRIPTING_STATUS_ERROR;
        return;
    }

    /* Detect tier: on_frame takes priority */
    lua_getglobal(s_L, "on_frame");
    if (lua_isfunction(s_L, -1)) {
        s_tier = TIER_FRAME;
        lua_pop(s_L, 1);
    } else {
        lua_pop(s_L, 1);
        lua_getglobal(s_L, "on_loop");
        if (lua_isfunction(s_L, -1)) {
            s_tier = TIER_LOOP;
        }
        lua_pop(s_L, 1);
    }

    /* Call on_start if defined */
    lua_getglobal(s_L, "on_start");
    if (lua_isfunction(s_L, -1)) {
        if (lua_pcall(s_L, 0, 0, 0) != LUA_OK) {
            ESP_LOGE(TAG, "on_start error: %s", lua_tostring(s_L, -1));
            lua_pop(s_L, 1);
            s_status = SCRIPTING_STATUS_ERROR;
            return;
        }
    } else {
        lua_pop(s_L, 1);
    }

    /* Set up coroutine for TIER_LOOP */
    if (s_tier == TIER_LOOP) {
        lua_State *co = lua_newthread(s_L);  /* pushes thread onto s_L stack */
        lua_getglobal(s_L, "on_loop");       /* pushes on_loop onto s_L stack */
        lua_xmove(s_L, co, 1);              /* moves on_loop to co's stack */
        /* store co in registry */
        lua_pushthread(co);
        lua_xmove(co, s_L, 1);
        lua_setfield(s_L, LUA_REGISTRYINDEX, "loop_coro");
        /* pop the thread value that lua_newthread left on s_L */
        lua_pop(s_L, 1);
    }

    s_status = SCRIPTING_STATUS_RUNNING;
}

void scripting_engine_tick(light_frame_t *frame, uint32_t now_ms, uint32_t dt_ms)
{
    s_current_tick_ms = now_ms;
    s_frame = frame;

    if (s_status == SCRIPTING_STATUS_ERROR || s_status == SCRIPTING_STATUS_IDLE) {
        s_frame = NULL;
        return;
    }

    if (s_tier == TIER_FRAME) {
        lua_getglobal(s_L, "on_frame");
        lua_pushnumber(s_L, (double)(now_ms - s_start_ms));
        lua_pushnumber(s_L, (double)dt_ms);
        if (lua_pcall(s_L, 2, 0, 0) != LUA_OK) {
            ESP_LOGE(TAG, "on_frame error: %s", lua_tostring(s_L, -1));
            lua_pop(s_L, 1);
            s_status = SCRIPTING_STATUS_ERROR;
        }
    } else if (s_tier == TIER_LOOP) {
        if (s_status == SCRIPTING_STATUS_SLEEPING) {
            if (now_ms < s_sleep_until_ms) {
                s_frame = NULL;
                return;
            }
            s_status = SCRIPTING_STATUS_RUNNING;
        }

        /* Retrieve coroutine from registry */
        lua_getfield(s_L, LUA_REGISTRYINDEX, "loop_coro");
        lua_State *co = lua_tothread(s_L, -1);
        lua_pop(s_L, 1);

        int nres   = 0;
        int result = lua_resume(co, s_L, 0, &nres);

        if (result == LUA_YIELD) {
            s_status = SCRIPTING_STATUS_SLEEPING;
            lua_pop(co, nres);
        } else if (result == LUA_OK) {
            /* on_loop returned — restart it */
            lua_pop(co, nres);
            lua_State *co2 = lua_newthread(s_L);
            lua_getglobal(s_L, "on_loop");
            lua_xmove(s_L, co2, 1);
            lua_pushthread(co2);
            lua_xmove(co2, s_L, 1);
            lua_setfield(s_L, LUA_REGISTRYINDEX, "loop_coro");
            lua_pop(s_L, 1);
            s_sleep_until_ms = 0;
        } else {
            ESP_LOGE(TAG, "on_loop error: %s", lua_tostring(co, -1));
            lua_pop(co, nres + 1);
            s_status = SCRIPTING_STATUS_ERROR;
        }
    }

    s_frame = NULL;
}

void scripting_engine_stop(void)
{
    s_status         = SCRIPTING_STATUS_IDLE;
    s_tier           = TIER_NONE;
    s_sleep_until_ms = 0;
    s_frame          = NULL;
    if (s_L) {
        lua_close(s_L);
        s_L = NULL;
    }
}

scripting_engine_status_t scripting_engine_status(void)
{
    return s_status;
}
