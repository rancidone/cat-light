---
status: implemented
date: 2026-04-02
discovery: ../discovery/v2-firmware-discovery.md
---

# System Design — Cat Light v2 Firmware

---

## Units

| # | Unit | Responsibility |
|---|------|----------------|
| 1 | Light Engine & Render Loop | LED driving, frame timing, global brightness |
| 2 | Scripting Engine | Lua VM, coroutine model, primitive library, two-tier hooks |
| 3 | Mode Manager | Built-in modes, script modes, mode lifecycle, button |
| 4 | Persistence & Storage | LittleFS layout, script storage, config, RAM-vs-flash |
| 5 | WiFi & Web Server | Provisioning, STA/SoftAP, HTTP server, web UI serving |

---

## Dependencies

```
Light Engine
    └── Scripting Engine (calls light engine primitives)
            └── Mode Manager (hosts script modes + built-in modes)
                    ├── Persistence (loads/saves modes & config)
                    └── WiFi & Web Server (reads/writes mode & config via HTTP)
```

---

## Cross-cutting Concerns

- **Memory budget** — Lua VM + WiFi stack + web server + render loop must coexist in ESP32 heap. Needs early validation before scripting engine design is locked.
- **Concurrency** — render loop, Lua coroutines, WiFi stack, and HTTP handler run in a multi-task FreeRTOS environment. Each unit design must name its task context and synchronization boundaries.
- **Global brightness** — owned by Light Engine, applied post-frame. All other units write normalized values.
- **LittleFS partition** — owned by Persistence, sized once at flash time. All other units that touch flash go through Persistence APIs.

---

## Design Status

| Unit | Status | Document |
|------|--------|----------|
| Light Engine & Render Loop | implemented | `light-engine.md` |
| Scripting Engine | implemented | `scripting-engine.md` |
| Mode Manager | implemented | `mode-manager.md` |
| Persistence & Storage | implemented | `persistence.md` |
| WiFi & Web Server | implemented | `wifi-web.md` |

---

## Memory Budget Spike

**Status:** Complete — constraint resolved.

**Result:** 203 KB free after WiFi (STA init) + HTTP server + Lua VM + stdlib. Well above 50 KB threshold. No memory-driven constraints on scripting engine design. See `../summaries/handoff_2026-03-24T17:00_heap-spike.md` for full numbers.
