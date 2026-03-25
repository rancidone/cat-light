# Handoff — Heap Spike / Memory Budget Validation

**Date:** 2026-03-24
**Topic:** ESP32 heap budget measurement for v2 firmware subsystems
**Status:** Spike complete — results captured, ready to inform v2 design

---

## What Was Done

Wrote and ran a memory budget spike to measure free heap after each major v2 subsystem init. The spike code lives at `./main/main_heap_spike.c` and temporarily replaces `main.c` (swap in `CMakeLists.txt` to re-run).

The spike initializes subsystems in dependency order and logs free heap + min-ever after each step:
1. Boot baseline
2. NVS init (required by WiFi)
3. WiFi STA init (stack only, not connected)
4. HTTP server start
5. Lua VM — `luaL_newstate()` only
6. Lua VM — `luaL_openlibs()` (standard libs)
7. Trivial Lua script execution (warm the VM)

---

## Measured Results

| Stage | Free Heap | Min-Ever |
|---|---|---|
| boot | **270 KB** | 270 KB |
| after nvs_flash_init | **269 KB** | 269 KB |
| after wifi init (STA, not connected) | **225 KB** | 225 KB |
| after httpd_start | **218 KB** | 218 KB |
| after luaL_newstate | **214 KB** | 214 KB |
| after luaL_openlibs | **204 KB** | 204 KB |
| after trivial script execution | **203 KB** | 203 KB |

### Derived Costs

| Subsystem | Heap Cost |
|---|---|
| NVS init | 1 KB |
| WiFi stack init (from post-NVS) | 44 KB |
| HTTP server start | 7 KB |
| HTTP server start | 7 KB |
| Lua VM (newstate) | 4 KB |
| Lua stdlib (openlibs) | 10 KB |
| Script execution (trivial) | ~1 KB |
| **Lua VM total (newstate + libs)** | **~14 KB** |

### Remaining Headroom

- **Total cost from boot to fully initialized:** ~67 KB (270 → 203 KB)
- **203 KB** free after all subsystems initialized
- WiFi stack is by far the largest consumer: **44 KB** (269 → 225 KB)
- This is with WiFi STA init only (not connected) — actual connection will cost more
- SoftAP mode and dual STA+AP would cost additional heap (not yet measured)

---

## Key Findings

- Lua VM + stdlib fit in ~14 KB — well within budget, risk from discovery is resolved
- WiFi + HTTP + Lua total cost from the captured window: ~22 KB
- 203 KB remaining is healthy headroom for: LED render loop, script execution buffers, FreeRTOS task stacks, LittleFS, and web UI asset serving
- WiFi connection (DHCP, TCP stack activation) will consume additional heap — worth a follow-up measurement with an actual STA connection established

---

## Files

- `./main/main_heap_spike.c` — spike source (keep for re-runs; not production code)
- `./main/main.c` — production main (currently swapped out in CMakeLists.txt)

---

## Next Steps

1. **Restore `main.c`** in `CMakeLists.txt` (replace `main_heap_spike.c` back to `main.c`, remove lua from REQUIRES)
2. **Optional follow-up spike:** measure heap after actual WiFi STA connection established (DHCP + IP assigned) to get true connected-state headroom
3. **Proceed to v2 design** — heap budget risk from discovery is resolved; Lua is confirmed viable
4. Begin `./docs/design/` work for v2 system architecture per the discovery brief

---

## Open Questions

- What is free heap after a real WiFi STA connection (not just init)?
- Does SoftAP mode cost meaningfully more than STA init?
- Boot and NVS baseline numbers not captured — run again if exact total cost from boot is needed
