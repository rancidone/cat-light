---
status: draft
date: 2026-04-02
---

# Web UI Design

## Problem

Provide a browser UI that makes device setup, mode control, and script authoring understandable for non-technical users, while remaining small and robust enough to be served directly from device flash over a connection that may disappear during WiFi changes.

## Responsibilities

- Present the device's current mode state and allow explicit mode switching
- Expose device config controls for brightness and hostname
- Provide WiFi credential entry and explain the reconnect/IP-change behavior clearly
- Let users inspect, edit, run, save, rename, and delete scripts using the existing REST API
- Make dev mode state visible, including the difference between running from RAM and saving to flash
- Communicate backend errors and disconnected/reconnecting states without implying stronger guarantees than the firmware provides

## Boundaries

- Does not invent new firmware capabilities beyond the current HTTP API
- Does not assume live push updates; current API is request/response only
- Does not own script execution semantics; it reflects the scripting and mode-manager models already defined
- Does not rely on external CDNs or runtime dependencies; assets must be device-hosted

## Known Backend Model

- Provisioning and normal use share the same server and endpoints
- Saving WiFi credentials causes the current browser connection to break; the user may need to reconnect on a new IP
- Device modes are a mixed list: built-ins, saved script slots, and an optional dev entry
- Dev mode is explicit state, not just "unsaved script exists"
- Saved scripts are fixed slots `1..10`; slot identity is stable and names are only labels
- Active mode is not restored across reboot; boot always starts on the first built-in mode

## Solution Structure

The Web UI is split into two distinct browser experiences:

1. Setup flow
2. Main app

Setup exists to get the device onto the user's WiFi and explain what happens next. The main app exists for ongoing control, configuration, and scripting after network attachment is complete.

This split is intentional, not cosmetic. Provisioning has different success criteria, different failure modes, and a different mental model than normal use. Folding setup into the main app would force the ongoing UI to carry provisioning-specific state and edge cases everywhere.

## Setup Flow

- Primary job: collect WiFi credentials and explain reconnect behavior
- Runs when the device is in SoftAP mode or otherwise not yet attached to the home network
- Uses the existing `POST /api/wifi/credentials` endpoint
- Treats reconnect as a terminal handoff, not an in-app transition the firmware can guarantee
- Uses a minimal handoff after submission: instruct the user to rejoin their normal WiFi and then visit `http://cat-light.local`

The setup flow should assume that after credentials are submitted:

- the current page may stop responding immediately
- the device may appear at a different IP or hostname
- the browser may remain attached to the device AP until the user manually rejoins their normal network

So the correct UX is not "spinner until success." The correct UX is "submit, then show a short handoff message before connectivity disappears."

## Main App

- Primary jobs: mode control, script editing, script slot management, brightness/hostname config
- Assumes the user has already reached the device on the local network
- Treats WiFi reconfiguration as an exceptional trip back through the setup-style reconnect handoff, not a common inline settings action
- Lands on mode control first; scripting is a secondary workspace, not the default home view
- Uses a separate scripting screen rather than embedding script authoring into the mode-control surface

The main app uses a small persistent top-level navigation with three destinations:

1. Control
2. Scripts
3. Device

This is intentionally narrow. The UI has three distinct user jobs after setup:

- control what the light is doing right now
- author or manage scripts
- change device-level settings that affect future sessions

Anything more granular would add navigation cost without matching the backend model. Anything flatter would mix active control, long-form editing, and disruptive settings into one overloaded surface.

WiFi reconfiguration is not a fourth primary destination. It remains an action inside Device settings because it is infrequent and because its completion breaks continuity rather than returning cleanly to the app.

## Main App Navigation

### Control

Control is the default landing screen and the everyday home surface.

It owns:

- current active mode visibility
- explicit mode switching across built-ins, saved script modes, and dev mode when present
- lightweight status messaging for mode changes and mode-related errors
- a compact summary of device state that matters while controlling the light

Control does not own script authoring, full slot management, or disruptive configuration.

The screen should optimize for immediate comprehension and low-friction switching:

- the current mode is visually prominent
- available modes are presented as a direct selection list, not hidden behind nested settings
- dev mode, when active, is labeled explicitly so the user understands they are running unsaved RAM-backed content rather than a saved slot

### Scripts

Scripts is a dedicated workspace for authoring and managing user scripts.

It owns:

- slot list for slots `1..10`
- occupied/empty state per slot
- script source editing
- rename, save, overwrite, and delete actions for slots
- loading a script into dev mode for immediate test execution
- explicit visibility into whether the current work is running in dev mode or stored durably in a slot

Scripts should be optimized for one focused workflow: choose a slot or start from current text, edit, test in dev mode, then decide whether to save.

This screen should make the RAM-vs-flash distinction impossible to miss:

- run/test actions map to dev mode and do not imply persistence
- save actions map to a numbered slot and do imply persistence
- exiting dev mode is explicit, not inferred from leaving the screen

Scripts is separate from Control so that the default app experience stays simple for users who mostly switch modes and rarely author scripts.

### Device

Device owns durable device-level configuration and rare administrative actions.

It owns:

- brightness configuration
- hostname configuration
- WiFi credential update entry point
- explanatory status text for actions that may interrupt connectivity

Device should not try to behave like a live provisioning dashboard. For WiFi changes it reuses the same design principle as setup:

- warn before submission that the current connection may disappear immediately
- submit credentials
- show a short reconnect handoff directing the user back to `http://cat-light.local` after rejoining normal WiFi

This keeps the disruptive network transition contained in one place instead of leaking provisioning assumptions across the whole app.

## Cross-Screen Behavior

- Top-level navigation is persistent and uses stable labels: `Control`, `Scripts`, `Device`
- The active screen owns its own fetch and submit cycle; no push-driven shared live model is assumed
- Screen changes do not implicitly save edits, exit dev mode, or apply config changes
- Success and error feedback should be local to the action that caused it, because request/response APIs provide only point-in-time truth
- Global device state that matters across screens should be summarized sparingly rather than mirrored in full on every screen

## Interface

The design-level interface for the browser app is:

- setup route or setup-only surface when the device is in provisioning context
- main app shell with three primary destinations for normal use
- explicit action boundaries between temporary execution (`/api/dev/load`, `/api/dev/exit`), durable script storage (`/api/scripts/*`), live mode switching (`/api/modes*`), and durable config (`/api/config`)

The UI should reflect the API's request/response nature directly:

- fetch screen data on entry and after mutating actions
- do not imply that background state remains synchronized without a refresh
- treat connectivity loss during WiFi actions as expected handoff behavior, not an exceptional unknown failure

## Tradeoffs & Decisions

- Separate setup from the main app
- Setup ends with a handoff, not guaranteed continuity
- Minimal reconnect instructions
- Main app optimized for repeat use
- Mode control is the default surface
- Scripting is a separate screen from control/config, not an embedded panel inside the main mode view
- Main app navigation is limited to three destinations: `Control`, `Scripts`, and `Device`
- WiFi reconfiguration stays inside `Device` instead of becoming a first-class top-level destination

## Readiness

High. The main app structure is now concrete: three primary destinations, explicit screen ownership, and contained WiFi handoff behavior aligned with the current API and firmware model.
