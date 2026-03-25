---
status: ready
date: 2026-03-24
---

# Unit 4 Design — Persistence & Storage

---

## Problem

Store user scripts, web UI assets, and device config durably across reboots. Size the LittleFS partition correctly at flash time — this is painful to change later.

---

## Responsibilities

- Mount and own the LittleFS partition
- Store and retrieve scripts by slot (1–10)
- Store and retrieve config key/values via NVS
- Enforce the 10-slot script limit

---

## Storage Split

| Data | Storage | Rationale |
|---|---|---|
| Scripts (up to 10) | LittleFS | File-sized blobs, need names/metadata |
| Web UI assets | LittleFS | Static files served over HTTP |
| WiFi credentials | NVS | Small key/value, survives LittleFS wipe |
| Brightness | NVS | Small key/value |
| Hostname | NVS | Small key/value |
| Active mode | — | Not persisted — always boots to first built-in |

---

## Script Slots

- 10 fixed slots
- Each slot stores: script source, display name (metadata only), slot index
- Slot is the identity — name is a display label, editable independently
- Writing to an occupied slot overwrites silently
- Writing when no slot is available returns an error — web UI is responsible for communicating this
- Mode manager queries the full slot list at boot

---

## Partition Layout

| Partition | Size | Notes |
|---|---|---|
| NVS | 20 KB | Key/value config |
| App | ~1.5 MB | Firmware binary |
| LittleFS | ~1.5 MB | Scripts + web UI assets |

Total: 4 MB flash. No OTA partition — out of scope for v2.

**LittleFS budget (informal):**
- Web UI assets: ~1 MB (uncompressed)
- Scripts: ~50 KB (10 × 5 KB generous estimate)
- Overhead/headroom: ~450 KB

> Risk: uncompressed web UI assets may exceed budget if the UI grows substantially. Levers available: gzip at build time, minification, asset splitting. Treat as a build-time concern, not a runtime one.

---

## Boundaries

- Does not own WiFi, HTTP serving, or mode logic
- Web server reads asset files directly from LittleFS mount point — persistence does not proxy file serving
- NVS access is direct via ESP-IDF NVS API — no abstraction layer needed at this scale

---

## Interface

```c
void persistence_init(void);

// Scripts
esp_err_t persistence_script_write(uint8_t slot, const char *name, const char *src, size_t len);
esp_err_t persistence_script_read(uint8_t slot, char *name_out, char *src_out, size_t src_max, size_t *len_out);
esp_err_t persistence_script_delete(uint8_t slot);
esp_err_t persistence_script_list(persistence_script_meta_t *out, uint8_t *count_out);

// Config
esp_err_t persistence_config_get(const char *key, char *value_out, size_t len);
esp_err_t persistence_config_set(const char *key, const char *value);
```

---

## Concurrency

LittleFS operations are not thread-safe by default — calls from web server task and mode manager task must be serialized. A simple mutex around persistence API calls is sufficient.

---

## Tradeoffs & Decisions

- **LittleFS for scripts and assets, NVS for config** — clean split; NVS survives a LittleFS wipe, appropriate for credentials
- **Slot-based script identity** — prevents unbounded script accumulation; name is metadata, not a key
- **No OTA partition** — frees ~1 MB for app + LittleFS; v2 constraint, revisit for v3
- **Web server reads assets directly from LittleFS** — no need for persistence to proxy static files
- **Asset compression deferred** — premature optimization; address if 1 MB budget proves insufficient
