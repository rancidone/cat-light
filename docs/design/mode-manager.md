---
status: implemented
date: 2026-04-02
---

# Unit 3 Design — Mode Manager

---

## Problem

Own the active mode, the mode cycle, and the button. Provide a uniform interface over built-in modes and script modes. Manage the dev slot and dev mode state, including failsafe exit on session timeout.

---

## Responsibilities

- Build the mode list at boot: built-ins (fixed order) + saved scripts (from persistence) + dev slot (if occupied)
- Maintain the active mode and call its `mode_tick(frame, dt)` each render tick via the light engine; for script modes, `mode_manager_tick()` calls `scripting_engine_tick()` internally
- Own the button: advance the mode cycle on press, suppressed in dev mode
- Manage dev mode state explicitly (see below)
- Poll `scripting_engine_status()` each tick; on `SCRIPTING_ERROR` enter error state
- Error state: special-cased outside mode list — red eyes blinking at 1 Hz, whiskers off; not reachable via button cycle
- Clear error state and resume normal cycling on any mode switch
- If active mode is deleted: advance to next mode in cycle; wrap to first built-in if deleted mode was last

---

## Mode List

| Slot | Source | Notes |
|---|---|---|
| Built-ins | Compiled in | Fixed order, always present |
| Saved scripts | Loaded from persistence at boot | Added after built-ins |
| Dev slot | RAM only, loaded via web UI | Single slot; added to end of cycle when occupied |

On boot: always starts on first built-in. Last active mode is not restored. Saved script list loaded from persistence at boot; updated live via `mode_manager_slot_saved/deleted`.

Built-in mode list (current): Fade, Spooky, Rainbow, Night Light. Not locked at design time — mode manager treats all built-ins uniformly.

---

## Dev Mode

Dev mode is an explicit device state, distinct from merely having the dev slot occupied.

| Event | Effect |
|---|---|
| Web UI loads script into dev slot | Dev slot occupied, device enters dev mode |
| Button press | Suppressed while in dev mode |
| Web UI exits dev mode | Dev mode exited, dev slot cleared, normal cycling resumes — explicit exit is the only path that clears the slot |
| Web session disconnects | 30-minute timeout starts; dev mode remains active until expiry |
| Reconnect within 30 minutes | Timeout cancelled, dev mode resumes |
| 30-minute timeout expires | Dev mode exited, cycling resumes; dev slot script remains in RAM |
| New script loaded into dev slot | Dev slot updated in place; if dev mode active, scripting engine restarts with new script |

---

## Boundaries

- Does not implement lighting behaviors — delegates entirely to `mode_tick` callbacks
- Does not own the Lua VM or scripting primitives — passes script buffer to scripting engine
- Does not own persistence — queries it at boot for saved script list, and re-reads individual slots via `slot_saved/deleted` callbacks; does not write
- Does not own WiFi or web server — receives commands from them via API

---

## Interface

```c
void mode_manager_init(void);
void mode_manager_start(void);

// Called each render tick by light engine
void mode_manager_tick(light_frame_t *frame, uint32_t dt_ms);

// Web UI / button
void mode_manager_next(void);
void mode_manager_slot_saved(uint8_t slot);
void mode_manager_slot_deleted(uint8_t slot);
void mode_manager_load_dev(const char *src, size_t len);
void mode_manager_exit_dev(void);

void mode_manager_on_session_connect(void);
void mode_manager_on_session_disconnect(void);
mode_manager_state_t mode_manager_get_state(void);
esp_err_t mode_manager_list_modes(mode_manager_mode_info_t *out, uint8_t max_count,
                                  uint8_t *count_out, uint8_t *active_index_out);
esp_err_t mode_manager_set_active(uint8_t index);
```

---

## Concurrency

- `mode_manager_tick()` runs on the render task
- Button handler and web UI commands arrive from other contexts — must go through a command queue or be protected by the frame mutex
- Dev mode timeout timer runs in a FreeRTOS timer callback; posts an exit command rather than mutating state directly

---

## Tradeoffs & Decisions

- **Uniform mode interface** — built-ins and script modes both provide a `mode_tick` callback; mode manager has no special cases per type
- **Dev mode is explicit state** — not inferred from slot occupancy; keeps button suppression and timeout logic unambiguous
- **Timeout on disconnect, not on inactivity** — active sessions never time out; timeout only starts on disconnect
- **Dev slot script held in RAM after timeout** — cycling resumes but script is still available when user reconnects; no need to re-paste
- **Boot always starts on first built-in** — no persistence dependency at boot for mode manager
- **Mode list is live-updated** — web server calls `mode_manager_slot_saved/deleted` after persistence writes; no reboot required
