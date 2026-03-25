# Discovery Brief — Cat Light v2 Firmware

**Status:** Complete — ready for Design  
**Date:** 2026-03-24

---

## Problem Statement

V1 firmware is a functional but minimal learning exercise. V2 is a complete product-quality firmware for a cat-shaped RGB night light. The primary gap is user configurability — there is no way to change behavior without reflashing. V2 closes this with a web-based configuration UI and a user-facing scripting engine, making the device accessible to non-technical users including children.

---

## Intended Outcomes

- Users can configure and control the device from a browser without reflashing
- Lighting modes are configurable via a rich primitive library, with frame-level access available for advanced users
- Children can write and run simple scripts that define custom lighting behaviors
- Scripts iterate quickly from RAM during development, then are explicitly saved to flash
- Device works reliably on a home network and degrades gracefully to SoftAP when no network is available
- A global brightness control exists independent of mode behavior
- All settings and saved scripts persist across reboots

---

## Constraints

- Hardware fixed: ESP32, RGB eye LEDs, 4 white whisker LEDs, 1 button
- ESP-IDF v5.5.3
- Scripting runtime must fit within ESP32 memory budget alongside firmware (Lua strongly implied)
- Web UI assets must be served from device flash — no CDN, no external dependencies at runtime
- Up to 10 named user scripts stored to flash via LittleFS
- Scripts running at frame rate (~20–50ms tick) set a real performance ceiling — known constraint, not a blocker
- OTA updates out of scope for v2
- Scheduling (time-based automation) out of scope for v2
- Button is system-controlled — not scriptable

---

## Assumptions

- Lua is the scripting language — small VM, coroutine support, proven on constrained hardware
- Scripts run as Lua coroutines to avoid blocking the render loop
- Two scripting tiers: high-level (`on_start`, `on_loop` at ~500ms cadence) and frame-level (`on_frame(t, dt)` at render tick). Script opts in to frame-level by defining `on_frame`
- Script primitive library covers: eye color/pulse/flicker/rainbow/fade/hue ramp, whisker brightness/pulse/fade, timing helpers, HSV conversion
- Scripts run from RAM during development; explicit user save action writes to LittleFS
- WiFi: SoftAP for provisioning, STA for normal use, SoftAP fallback when network unavailable
- Web UI is configuration-primary; live/manual control is supported as a secondary mode
- Button cycles modes at system level as in v1

---

## Risks

- Lua VM + web server + WiFi stack + LED render loop is significant memory pressure on ESP32 — heap budget needs early validation
- SoftAP↔STA transition handling adds meaningful firmware complexity (reconnect logic, fallback behavior)
- Web IDE + script execution model (RAM vs flash, save flow) needs careful UX design to avoid data loss
- Frame-level scripting at 20–50ms with a Lua coroutine has a hard performance ceiling — poorly written scripts will visibly degrade output
- LittleFS partition sizing must be planned early; wrong sizing is painful to change later

---

## Out of Scope (v2)

- OTA firmware updates
- Scheduling / time-based automation
- Home automation integration (MQTT etc.)
- Script-to-script calls or mode chaining
- Button interception from scripts

---

## Open Questions

None blocking.
