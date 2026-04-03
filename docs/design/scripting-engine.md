---
status: implemented
date: 2026-04-02
---

# Unit 2 Design — Scripting Engine

---

## Problem

Execute user-authored Lua scripts that define lighting behaviors. Scripts must not block the render loop, must fail safely, and must expose a controlled primitive library without access to frame internals.

---

## Responsibilities

- Own the Lua VM (`lua_State`)
- Load and execute scripts as coroutines
- Implement `sleep(ms)` as a coroutine yield; track elapsed ticks and resume when elapsed
- Call `on_loop()` at ~500ms cadence, or `on_frame(t, dt)` every render tick — script picks one tier by which hook it defines
- Expose the primitive library to Lua (eye color/pulse/flicker/rainbow/fade, whisker brightness/pulse/fade, timing helpers, HSV conversion)
- Catch Lua errors and signal the error state
- Report script status (running, sleeping, errored) to the mode manager

---

## Boundaries

- Runs on the render task — called from `mode_tick()` each tick
- Does not write `light_frame_t` directly — primitives call `light_engine_set_mode_tick` callback writes; eye primitives map to `eye_r/g/b`, whisker primitives map to `whisker` in `light_frame_t`
- Does not expose `light_frame_t`, brightness scalar, or raw LED values to Lua
- Does not own mode selection or persistence — receives a loaded script buffer, does not read flash

---

## Hooks

```lua
function on_start()       -- called once on script activation, both tiers
function on_loop()        -- high-level tier; use sleep(ms) to pace
function on_frame(t, dt)  -- frame tier; t = ms since start, dt = ms since last frame
```

- `on_start` always runs first, regardless of tier
- Presence of `on_frame` opts script into frame tier; `on_loop` is ignored if both defined
- `sleep(ms)` in `on_loop` yields coroutine; engine resumes after elapsed ticks
- `sleep(ms)` in `on_frame` is a script error — triggers error state

---

## Primitive Library

**Eyes** (both RGB eye LEDs):
```lua
eye.set(r, g, b)          -- immediate color (0–255 each)
eye.fade(r, g, b, ms)     -- transition from current color over ms
eye.pulse(r, g, b, ms)    -- fade in then out
eye.flicker(r, g, b)      -- random brightness jitter at current color
eye.rainbow(ms)           -- cycle full hue range over ms
```

**Whiskers** (all 4 LEDs on one channel):
```lua
whisker.set(brightness)        -- 0.0–1.0
whisker.fade(brightness, ms)   -- fade to brightness over ms
whisker.pulse(brightness, ms)  -- fade in then out
```

**Helpers**:
```lua
sleep(ms)       -- yield coroutine (on_loop only; error elsewhere)
time()          -- ms elapsed since script start (on_loop only; use t in on_frame)
hsv(h, s, v)    -- pure conversion, returns r, g, b; h: 0–360, s/v: 0.0–1.0
```

### Availability by hook

| Primitive | `on_start` | `on_loop` | `on_frame` |
|---|---|---|---|
| `eye.set` | ✓ | ✓ | ✓ |
| `eye.fade` | ✓ | ✓ | — |
| `eye.pulse` | ✓ | ✓ | — |
| `eye.flicker` | ✓ | ✓ | — |
| `eye.rainbow` | ✓ | ✓ | — |
| `whisker.set` | ✓ | ✓ | ✓ |
| `whisker.fade` | ✓ | ✓ | — |
| `whisker.pulse` | ✓ | ✓ | — |
| `sleep(ms)` | — | ✓ | — |
| `time()` | — | ✓ | — |
| `hsv()` | ✓ | ✓ | ✓ |

---

## Error Handling

- Lua errors caught via `lua_pcall` / `coroutine.resume` status
- On error: engine sets status to `SCRIPTING_ERROR`, stops calling script hooks
- Mode manager detects error status and enters error state (special-cased outside mode list)
- Error state output (owned by mode manager): red eyes blinking at 1 Hz, whiskers off
- Error state cleared by any mode switch — mode manager calls `scripting_engine_stop()`

---

## Interface

```c
void scripting_engine_init(void);
void scripting_engine_load(const char *src, size_t len);
void scripting_engine_tick(uint32_t now_ms, uint32_t dt_ms);
void scripting_engine_stop(void);
scripting_engine_status_t scripting_engine_status(void);
```

`scripting_engine_tick()` is called by the mode manager's `mode_tick` implementation each render tick.

---

## Concurrency

Runs entirely on the render task — no additional synchronization required beyond what the light engine already provides.

---

## Tradeoffs & Decisions

- **Lua on render task** — simpler than a dedicated task; frame latency from slow scripts is the same failure mode as a dropped frame (PWM holds last value), so no user-visible difference justifies the added complexity
- **One coroutine per script** — one tier per script; mixing `on_loop` and `on_frame` adds coroutine scheduling complexity with no clear use case
- **`on_frame` wins if both hooks defined** — silent, predictable, documented behavior
- **Primitives only, no raw frame access** — keeps Lua scripts within a safe abstraction; prevents brightness bypass
- **Engine does not own script loading** — decouples VM lifecycle from persistence; mode manager passes a buffer
