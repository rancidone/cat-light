# Session Handoff: V2 Firmware Design Complete
**Date:** 2026-03-24
**Session Duration:** ~3 hours
**Session Focus:** Complete all five v2 firmware unit designs, resolve all consistency issues, and promote to ready.
**Context Usage at Handoff:** ~35%

---

## What Was Accomplished

1. Heap spike run and results captured → `./docs/summaries/handoff-2026-03-24T17:00-heap-spike.md`
2. Design frontmatter contract established → `./docs/design/TEMPLATE.md`
3. Light Engine design finalized → `./docs/design/light-engine.md`
4. Scripting Engine design completed → `./docs/design/scripting-engine.md`
5. Mode Manager design completed → `./docs/design/mode-manager.md`
6. Persistence & Storage design completed → `./docs/design/persistence.md`
7. WiFi & Web Server design completed → `./docs/design/wifi-web.md`
8. System design updated with all unit statuses and spike result → `./docs/design/system.md`
9. Full consistency/completeness pass — 16 issues found and resolved
10. All five units promoted to `status: ready`
11. Pre-implementation TODO added → `./TODO.md`

---

## Exact State of Work in Progress

- All design units: complete and promoted to `ready` — no in-progress work
- `./main/main_heap_spike.c`: still present; `CMakeLists.txt` still references it instead of `main.c` — needs restoration before any firmware work

---

## Decisions Made This Session

- **Two-tier scripting model**: `on_loop` (high-level, ~500ms) vs `on_frame(t, dt)` (frame-level, 30ms tick). Script picks one tier by which hook it defines. `on_frame` wins if both defined. CONFIRMED
- **Lua runs on render task**: no dedicated Lua task. Frame latency from slow scripts == dropped frame (PWM holds), no user-visible difference. CONFIRMED
- **`sleep()` in `on_frame` is a script error**: triggers error state. CONFIRMED
- **Error state**: scripting engine sets `SCRIPTING_ERROR` status; mode manager owns rendering (red eyes 1 Hz, whiskers off); special-cased outside mode list. CONFIRMED
- **Dev mode explicit state**: button suppressed in dev mode; exits only via web UI or 30-minute post-disconnect timeout. CONFIRMED
- **Dev slot on timeout**: dev mode exits, cycling resumes, but script stays in RAM — user can reconnect and resume within session. CONFIRMED
- **Explicit exit clears dev slot; timeout does not**: intentional asymmetry, documented. CONFIRMED
- **Mode list is live-updated**: `mode_manager_slot_saved(slot)` / `mode_manager_slot_deleted(slot)` called by web server after persistence writes. CONFIRMED
- **Active mode deleted**: advance to next in cycle; wrap to first built-in if last. CONFIRMED
- **Boot button ownership**: `app_main` reads button before unit init; sets `boot_flags_t` (defined in `boot.h`); WiFi reads `boot_flags.force_softap`. Mode manager takes over button after init. CONFIRMED
- **LittleFS partition**: 1.5 MB. NVS: 20 KB. App: ~1.5 MB. No OTA partition. CONFIRMED
- **Scripts**: slot-based identity (1–10), name is metadata only. CONFIRMED
- **Config in NVS**: WiFi credentials, brightness, hostname. Active mode not persisted. CONFIRMED
- **`on_start` never sleeps** regardless of tier. CONFIRMED
- **Whiskers on one channel**: all 4 LEDs driven together, no individual control. CONFIRMED
- **`eye.hue()` removed**: redundant with `hsv()` helper. CONFIRMED

---

## Key Numbers

- **Boot heap**: 270 KB free
- **After NVS init**: 269 KB free
- **After WiFi STA init**: 225 KB free (WiFi stack costs ~44 KB)
- **After HTTP server**: 218 KB free
- **After Lua VM (newstate)**: 214 KB free
- **After Lua stdlib (openlibs)**: 204 KB free
- **After trivial script execution**: 203 KB free
- **Total cost boot→ready**: ~67 KB
- **LittleFS budget**: 1.5 MB total; ~1 MB web UI assets, ~50 KB scripts, ~450 KB headroom
- **Render tick**: 30ms (fixed compile-time constant)
- **Dev mode timeout**: 30 minutes after session disconnect
- **STA connection timeout**: 15 seconds

---

## Conditional Logic Established

- IF script defines `on_frame` THEN frame tier is used, `on_loop` ignored BECAUSE mixing tiers adds coroutine complexity with no clear use case
- IF `sleep()` called in `on_frame` or `on_start` THEN script error triggered BECAUSE sleep is only valid in `on_loop`
- IF active mode is deleted THEN advance to next mode; wrap to first built-in if last BECAUSE "next" feels natural, "first" feels like a jarring reset
- IF web session disconnects while in dev mode THEN 30-minute timeout starts; if reconnect within 30 min, timeout cancelled BECAUSE hard reset is available as failsafe
- IF timeout expires THEN dev mode exits, cycling resumes, dev slot script stays in RAM BECAUSE user may want to resume
- IF button held at boot THEN `app_main` sets `boot_flags.force_softap`, clears WiFi credentials BECAUSE physical failsafe for bad credentials
- IF web UI saves/deletes a script THEN web server calls `mode_manager_slot_saved/deleted` immediately BECAUSE saved scripts must appear in mode cycle without reboot
- IF asset compression needed THEN gzip at build time or minify — deferred until 1 MB LittleFS budget is exceeded

---

## Files Created or Modified

| File Path | Action | Description |
|---|---|---|
| `./docs/design/TEMPLATE.md` | Created | Frontmatter contract + design doc template |
| `./docs/design/light-engine.md` | Modified | Added `light_frame_t` + `mode_tick_fn_t` definitions; removed `light_engine_get()`; promoted to ready |
| `./docs/design/scripting-engine.md` | Created | Full scripting engine design: hooks, primitive library, availability table, error handling |
| `./docs/design/mode-manager.md` | Created | Mode list, dev mode state machine, error state, live slot updates |
| `./docs/design/persistence.md` | Created | LittleFS/NVS split, slot-based scripts, partition layout |
| `./docs/design/wifi-web.md` | Created | WiFi state machine, REST API, boot flags, reconnect UX note |
| `./docs/design/system.md` | Modified | All units marked ready; spike section updated with results |
| `./docs/summaries/handoff-2026-03-24T17:00-heap-spike.md` | Created | Heap spike results |
| `./TODO.md` | Created | Pre-implementation: write `partitions.csv` |
| `./CLAUDE.md` | Modified | Added pointer to `TEMPLATE.md` for design frontmatter contract |
| `./.claude/commands/handoff.md` | Modified | Updated filename format to ISO 8601 (`T` separator) |

---

## What the NEXT Session Should Do

1. **First**: Restore `main.c` in `CMakeLists.txt` — replace `main_heap_spike.c`, remove lua from REQUIRES
2. **Then**: Write `partitions.csv` — locks LittleFS (1.5 MB) and app (~1.5 MB) partition sizes; this must be done before any firmware implementation
3. **Then**: Begin implementation — suggested order follows dependency graph: Light Engine → Scripting Engine → Mode Manager → Persistence → WiFi & Web Server
4. **Consider**: Running a follow-up heap spike after actual WiFi STA connection (not just init) to confirm real connected-state headroom

---

## Open Questions Requiring User Input

- None blocking. All design decisions confirmed.

---

## Assumptions That Need Validation

- ASSUMED: App binary fits in ~1.5 MB partition — validate by checking binary size after initial build
- ASSUMED: Web UI assets fit in ~1 MB uncompressed — validate during web UI development; compression available as lever if needed
- ASSUMED: Lua VM + stdlib + script execution fits comfortably in 203 KB remaining heap — spike only measured newstate + openlibs; actual script execution memory not yet profiled under load

---

## What NOT to Re-Read

- `./docs/discovery/v2-firmware-discovery.md` — fully absorbed into design; only re-read if a design decision needs revalidation against product intent
- `./docs/summaries/handoff-2026-03-24T17:00-heap-spike.md` — numbers summarized above

---

## Files to Load Next Session

- `./docs/design/system.md` — dependency graph and unit overview
- `./docs/design/light-engine.md` — first implementation target; defines `light_frame_t` and `mode_tick_fn_t`
- `./TODO.md` — pre-implementation checklist
