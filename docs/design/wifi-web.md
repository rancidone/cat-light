---
status: implemented
date: 2026-04-02
---

# Unit 5 Design — WiFi & Web Server

---

## Problem

Connect the device to a home network and serve a web UI for configuration, scripting, and mode control. Degrade gracefully to SoftAP when STA is unavailable. Provide a hard reset path for credential recovery.

---

## Responsibilities

- Manage WiFi state machine (SoftAP, STA, fallback)
- Start and own the HTTP server
- Serve web UI static assets from LittleFS
- Expose REST API for scripts, config, and mode control
- Handle dev slot load/run from web UI
- Read `boot_flags_t` set by `app_main` before unit init (defined in `boot.h`)

---

## WiFi State Machine

```
Boot
 ├── Button held → clear credentials → SoftAP
 ├── No credentials → SoftAP
 └── Credentials exist → attempt STA
         ├── Connected → STA mode
         └── Failed/timeout → SoftAP fallback
```

- STA and SoftAP are mutually exclusive — never simultaneous
- STA connection timeout: 15 seconds (enough for a slow router)
- HTTP server runs in both modes on the same endpoints — interface-agnostic
- Hard reset: `app_main` reads button state before any unit init; if held, clears WiFi credentials from NVS and sets `boot_flags.force_softap`. WiFi unit reads `boot_flags_t` and starts in SoftAP. Mode manager takes over button ownership after init.

---

## REST API (outline)

| Method | Endpoint | Description |
|---|---|---|
| GET | `/api/scripts` | List all slots (index, name, occupied) |
| GET | `/api/scripts/:slot` | Read script source + name |
| PUT | `/api/scripts/:slot` | Write script to slot |
| DELETE | `/api/scripts/:slot` | Clear slot |
| PATCH | `/api/scripts/:slot/name` | Rename script (triggers `mode_manager_slot_saved`) |
| GET | `/api/modes` | Return state, active index, and available modes (built-ins + saved slots + dev) |
| POST | `/api/modes/active` | Switch active mode by JSON body `{ "index": N }` |
| POST | `/api/dev/load` | Load script into dev slot, enter dev mode |
| POST | `/api/dev/exit` | Exit dev mode |
| GET | `/api/config` | Read config (brightness, hostname) |
| PUT | `/api/config` | Write config |
| POST | `/api/wifi/credentials` | Save WiFi credentials, trigger reconnect (browser loses connection; user must navigate to new IP) |

Static assets served directly from LittleFS mount point — no API prefix.

---

## Boundaries

- Does not own LittleFS or NVS — calls persistence API for scripts and config
- Does not own mode logic — calls mode manager API for mode switching and dev slot
- Web server reads static assets directly from LittleFS (ESP-IDF supports file serving from VFS)
- Does not implement lighting behavior

---

## Concurrency

- HTTP server runs in its own FreeRTOS task (ESP-IDF default)
- API handlers call mode manager and persistence APIs from the HTTP task context
- Mode manager and persistence APIs must be safe to call from the HTTP task (see respective unit designs)

---

## Tradeoffs & Decisions

- **STA with SoftAP fallback, never simultaneous** — simpler state machine, avoids dual-interface routing complexity
- **Hard reset via button hold at boot** — physical failsafe for bad credentials; no software-only recovery path needed
- **REST API** — clean, web-standard; `esp_http_server` overhead is in the library already being used, not in REST conventions
- **Static assets served directly from LittleFS VFS** — no need to buffer assets through the app; ESP-IDF HTTP server supports this natively
- **Single server, both modes** — no mode-specific endpoint logic; simplifies server lifecycle
- **STA timeout: 15 seconds** — compile-time constant, not runtime configurable
