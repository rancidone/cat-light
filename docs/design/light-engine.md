---
status: implemented
date: 2026-04-02
---

# Unit 1 Design — Light Engine & Render Loop

---

## Problem

Drive physical LEDs at a consistent frame rate. Provide a clean frame abstraction that the active mode writes into each tick. Apply global brightness as a post-process before LED output.

---

## Responsibilities

- Own the `light_frame_t` (eye RGB + whisker values)
- Run a FreeRTOS task at a fixed 30ms tick rate (compile-time constant)
- Each tick: acquire mutex → call `mode_tick(frame, dt)` → apply brightness scalar → write LEDs → release mutex
- Expose `set_brightness(float)` API — writes under the frame mutex
- No knowledge of modes, scripts, or behaviors

---

## Boundaries

- Does not own mode selection or lifecycle
- Does not expose frame or brightness to Lua
- Brightness applied post-frame, invisible to all callers

---

## Types

```c
// Both eye LEDs share one RGB channel. All 4 whisker LEDs share one channel.
typedef struct {
    uint8_t eye_r;
    uint8_t eye_g;
    uint8_t eye_b;
    float   whisker; // 0.0–1.0
} light_frame_t;

typedef void (*mode_tick_fn_t)(light_frame_t *frame, uint32_t dt_ms, void *ctx);
```

---

## Interface

```c
void light_engine_init(void);
void light_engine_start(void);
void light_engine_set_mode_tick(mode_tick_fn_t fn, void *ctx);
void light_engine_set_brightness(float brightness);
```

---

## Concurrency

Single FreeRTOS task. Frame mutex protects `light_frame_t` and brightness scalar. External writers (web server, persistence) use `set_brightness()` which acquires the mutex.

---

## Tradeoffs & Decisions

- **Single frame + mutex** chosen over double-buffer — revisit only if contention proves real under load
- **Fixed 30ms tick rate** — compile-time constant, not runtime configurable
- **Brightness in engine struct** — not exposed to Lua API; scripts write normalized frame values only
