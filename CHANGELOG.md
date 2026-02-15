# Changelog

## 2026-01-30 — v1.4.0

### Features

- **Boot animation** — Rainbow colour wave plays on power-up with brightness ramping from 0→100% over 2.5 seconds, replacing the previous dark startup.
- **Fade to black** — Smooth 1-second fade when toggling LEDs on/off via the dashboard, instead of an abrupt snap. Uses a `masterFadeFactor` multiplied into effect brightness alongside the existing thermal dimming.
- **Visual timeline** — 24-hour horizontal bar on the schedule editor page showing coloured blocks for each slot. Overnight slots wrap at midnight as two segments. Updates live as slots are edited.
- **Non-blocking NTP** — Reduced startup blocking from ~10+ seconds to a 1.5-second quick check. NTP continues syncing in the background; the main loop shows a warm fallback colour until time is available.

### Fixes

- **Web UI broken with custom schedules** — The `/api/status` JSON buffer was 1,536 bytes, which overflowed with more than 3 schedule slots, producing truncated JSON that broke the dashboard. Increased to 4,096 bytes.
- **OTA updates timing out** — The 10-second hardware watchdog was not disabled during firmware uploads, causing a panic reboot mid-flash. Watchdog is now disabled on OTA start (both ArduinoOTA and web upload) and re-enabled on failure.

### Files changed
- **`config.h`** — Version 1.4.0; added `FADE_DURATION_MS`
- **`globals.h`** — Added `masterFadeFactor` extern
- **`night_light.ino`** — Boot animation call, master fade state machine replacing abrupt kill switch, watchdog disable for ArduinoOTA
- **`led_effects.cpp`** — `wheel()` made non-static, `playBootAnimation()` function, `masterFadeFactor` applied in `applyEffect()`
- **`led_effects.h`** — Declared `wheel()` and `playBootAnimation()`
- **`wifi_manager_setup.cpp`** — Non-blocking NTP sync (1.5s quick check instead of 20-retry loop)
- **`web_server.cpp`** — JSON buffer 1536→4096, watchdog disable for web OTA upload
- **`html_pages.h`** — Visual timeline on schedule editor (CSS, HTML, JS)

---

## 2026-01-29 — v1.3.0

### Improved — Reliability, UX & code quality (`night_light/`)

#### Reliability

- **Thermal hysteresis** — Added 5°C hysteresis band (`THERMAL_WARN_CLEAR_C` at 60°C) to prevent LED flickering at threshold temperatures. Once throttling/shutdown activates, it stays active until temperature drops below the clear threshold. Applied in both main loop and AP mode loop.
- **Hardware watchdog** — ESP32 task watchdog (`esp_task_wdt`) with 10-second timeout. Automatically reboots if `loop()` stalls.
- **WiFi reconnection** — Periodic WiFi status check every 30 seconds. On disconnect, calls `WiFi.reconnect()`. After 10 consecutive failures (~5 minutes), reboots the device.
- **millis() overflow fix** — 64-bit `uptimeMs` counter tracks uptime across `millis()` wraparound (every ~49 days). Used in `/api/status` and MQTT uptime sensor.
- **MQTT exponential backoff** — Reconnect interval doubles on each failure (5s → 10s → 20s → 40s → 60s cap). Resets to 5s on successful connection or settings change.

#### UX

- **Pretty mode banner** — Dashboard shows a purple "Manual override active" banner with a "Return to schedule" button when in manual override mode.
- **Schedule overlap warning** — Schedule editor checks for overlapping time slots on save and shows a warning toast (does not block the save).
- **Slot reordering** — Schedule editor slots have ▲/▼ buttons to reorder without deleting and re-creating.
- **AP controls brightness %** — AP mode controls page shows brightness as a percentage alongside the slider.
- **WiFi/MQTT indicators** — Dashboard footer shows colour-coded WiFi and MQTT connection status (green = connected, red = disconnected).

#### Security

- **Login rate limiting** — After 5 failed login attempts, locks out for 30 seconds. Shows "Too many attempts" message.

#### Code Quality

- **Centralised externs** — New `globals.h` file replaces per-file `extern` blocks in `web_server.cpp`, `mqtt_manager.cpp`, and `wifi_manager_setup.cpp`.
- **Fixed-buffer JSON** — `/api/status` endpoint uses `snprintf` into a stack buffer instead of `String +=` concatenation, reducing heap fragmentation.

#### Files changed
- **`globals.h`** — New file with centralised extern declarations
- **`config.h`** — Version 1.3.0; added `THERMAL_WARN_CLEAR_C`, `WIFI_CHECK_INTERVAL_MS`, `WIFI_MAX_FAILURES`, `MQTT_BACKOFF_MAX`
- **`night_light.ino`** — Watchdog init, 64-bit uptimeMs, WiFi reconnect loop, thermal hysteresis
- **`wifi_manager_setup.cpp`** — Thermal hysteresis in AP loop, uses `globals.h`
- **`mqtt_manager.cpp`** — Exponential backoff, 64-bit uptime in sensor publish, uses `globals.h`
- **`web_server.cpp`** — Login rate limiting, fixed-buffer JSON, `wifiConnected` field, uses `globals.h`
- **`html_pages.h`** — Pretty mode banner, schedule overlap warning, slot reorder buttons, AP brightness %, WiFi/MQTT indicators

---

## 2026-01-29 — v1.2.0

### Added — Settings page and AP mode light controls (`night_light/`)

#### Settings Page
- **`html_pages.h`** — New `SETTINGS_PAGE` template with MQTT and general settings (timezone, transition time, password, factory reset). Removed MQTT and Settings cards from `DASHBOARD_PAGE` to declutter the main view. Dashboard footer now includes a Settings link.
- **`web_server.cpp`** — New `GET /settings` route (`handleSettingsPage`) serving the settings page with auth check and CSS injection.

#### Captive Portal Light Controls
- **`wifi_manager_setup.cpp`** — Major rework of AP mode behaviour:
  - AP timeout set to 0 (stays in AP mode indefinitely, no reboot on timeout)
  - Custom menu link in WiFiManager captive portal pointing to light controls on port 8080
  - Controls `WebServer` on port 8080 with unauthenticated routes: `GET /`, `POST /api/brightness`, `POST /api/colour`, `POST /api/effect`, `GET /api/status`
  - Both WiFiManager and controls server processed concurrently in the AP loop
  - LED rendering in AP loop: manual override uses `applyEffect()`, otherwise shows scanner animation
  - Controls server stops cleanly when WiFi connects
- **`html_pages.h`** — New `AP_CONTROLS_PAGE` template with colour picker, warm white slider, brightness slider, effect dropdown (all 10 effects), and status polling. No authentication required. Links back to WiFi setup at 192.168.4.1.

#### AP Mode LED Animation
- **`wifi_manager_setup.cpp`** — Replaced `pulseStrip()` (pulsing warm white) with `scannerStrip()`: dim warm amber base across all LEDs with a bright red scanner LED travelling along the strip with a 5-LED fade trail. Cycle time ~2.5 seconds.

- **`config.h`** — Version bumped to 1.2.0

---

## 2026-01-29

### Fixed — Independent brightness and schedule/pretty mode separation (`night_light/`)

- **`night_light.ino`** — Added `userBrightnessActive` and `userBrightness` globals. Restructured `loop()` to always evaluate the schedule (tracking `currentSlotIndex`) regardless of mode. Removed early return that blocked schedule evaluation during manual override. Render logic now applies effective brightness (user-set or slot default) independently of colour/effect mode.
- **`web_server.cpp`** — `handleApiBrightness()` now sets `userBrightness` independently without triggering pretty mode. Status JSON reports effective brightness and `userBrightnessActive` flag. Factory reset clears user brightness.
- **`mqtt_manager.cpp`** — `handleLightCommand()` separates brightness from mode changes: brightness-only commands don't enter pretty mode, colour/effect commands do. State publishing uses effective brightness.
- **`led_effects.h/.cpp`** — `applyTransition()` accepts optional `brightnessOverride` parameter so transitions respect user-set brightness instead of lerping between slot values.

**Behaviour change:** Two distinct modes — *Schedule* (colours follow the schedule) and *Pretty* (manual colour/effect, persists until "Use schedule" is selected). Brightness is fully independent: adjusting it never changes the mode, and it persists across slot transitions.

---

## 2026-01-28 — v1.1.0

### Added — Firmware version, diagnostics, RGBW, special dates, colour controls, thermal protection (`night_light/`)

#### Feature 5: Chip Temperature Monitoring + Thermal Protection
- **`config.h`** — Added `THERMAL_WARN_C` (65°C), `THERMAL_CRIT_C` (80°C), `TEMP_READ_INTERVAL_MS` (5s) constants
- **`night_light.ino`** — Reads ESP32-C3 internal temperature sensor every 5 seconds via `temperatureRead()`. Exposes `chipTempC`, `thermalDimFactor` (1.0–0.0), `thermalShutdown` globals. Above warning threshold, linearly dims LEDs; above critical threshold, forces LEDs off.
- **`led_effects.cpp`** — `applyEffect()` applies `thermalDimFactor` to brightness before rendering any effect, covering all code paths including transitions
- **`web_server.cpp`** — `chipTemp`, `thermalThrottle`, `thermalDimPct` fields in `/api/status`
- **`html_pages.h`** — Dashboard footer shows chip temperature with colour coding (green < 55°C, amber 55–65°C, red when throttling)
- **`mqtt_manager.cpp`** — New HA diagnostic sensor: Chip Temperature (`device_class: temperature`, unit: °C, `mdi:thermometer`). Published alongside uptime/heap in periodic sensor updates.


#### Feature 1: Firmware Version Display
- **`config.h`** — Added `FW_VERSION "1.1.0"` define
- **`web_server.cpp`** — `version` field in `/api/status` JSON response
- **`mqtt_manager.cpp`** — Device block `sw` field now uses `FW_VERSION` instead of hardcoded string
- **`html_pages.h`** — Dashboard footer shows firmware version; firmware update page shows current version via fetch

#### Feature 2: Uptime + Free Memory Diagnostics
- **`web_server.cpp`** — Added `uptime`, `uptimeMs`, `freeHeap` fields to `/api/status`
- **`html_pages.h`** — Dashboard footer shows uptime and free memory
- **`mqtt_manager.cpp`** — New HA diagnostic sensors: Uptime (duration, seconds), Free Memory (bytes, `mdi:memory`). Published alongside RSSI and NTP in periodic sensor updates.

#### Feature 3: RGBW White Channel Support
- **`config.h`** — Added `FALLBACK_W` constant; added `uint8_t white` field to `ScheduleSlot`; struct size change triggers one-time NVS reset to defaults
- **`night_light.ino`** — Added `overrideW` global; all `applyEffect()` calls pass white channel
- **`led_effects.h/.cpp`** — All effect signatures updated with `uint8_t w` parameter. `effectSolid/Breathing/SoftGlow/StarryTwinkle` use RGBW `strip.Color()`. `effectCandleFlicker` adds warm white glow. `effectSunrise` ramps white across 3 phases. `wheel()` helper outputs 4-channel colour. Added `wheelRGB()` helper for raw RGB extraction.
- **`web_server.cpp`** — `extern overrideW`; white field in slotsDetail JSON; white copied in brightness/effect override handlers; `white` field parsed in schedule save
- **`mqtt_manager.cpp`** — RGBW colour mode in light discovery; `w` in colour state payload; `w` parsed from incoming commands; white tracked in deduplication
- **`html_pages.h`** — Warm White range slider in schedule editor; white=0 in default new slot
- **`schedule.cpp`** — Default slot white values: bedtime=60, sleep=0, wake=20

#### Feature 4: Holiday/Birthday Theme Effects
- **`config.h`** — Extended `EffectType` enum: `EFFECT_CHRISTMAS=7`, `EFFECT_HALLOWEEN=8`, `EFFECT_BIRTHDAY=9`, `EFFECT_COUNT=10`. Added `SpecialDate` struct and `MAX_SPECIAL_DATES=4`. Added special dates array to `NightLightConfig`.
- **`led_effects.cpp`** — 3 new effects: Christmas (red/green alternating with breathing), Halloween (orange/purple with candle flicker), Birthday (rainbow base with sparkle twinkles using `wheelRGB()`)
- **`schedule.h/.cpp`** — Added `evaluateSpecialDates()` function. NVS persistence for special dates (`numSpecDates`, `specDate0`–`specDate3`). Initialised to 0 in defaults.
- **`wifi_manager_setup.h/.cpp`** — Added `getCurrentDate()` function using `getLocalTime()`
- **`night_light.ino`** — Special date evaluation in loop: auto-activates matching date's effect, tracks `specialDateOverrideActive` and `specialDateDismissed` flags, auto-releases on day change
- **`web_server.cpp`** — `specialDates` array in `/api/status`; new `POST /api/special-dates` endpoint for saving dates
- **`html_pages.h`** — Special Dates card in dashboard with enable toggle, label, month/day pickers, effect selector, brightness slider. Christmas/Halloween/Birthday options in all effect selects.
- **`mqtt_manager.cpp`** — Special Date sensor (`mdi:cake-variant`) publishing active date label. Effect list auto-expands to 10 effects.

### Home Assistant entities (new)
- **Sensor** — Uptime (diagnostic, `device_class: duration`, unit: seconds)
- **Sensor** — Free Memory (diagnostic, `mdi:memory`, unit: bytes)
- **Sensor** — Special Date (active date label, `mdi:cake-variant`)
- **Sensor** — Chip Temperature (diagnostic, `device_class: temperature`, unit: °C, `mdi:thermometer`)
- **Light** — Now RGBW colour mode (was RGB), 10 effects (was 7)

---

## 2026-01-27 (2)

### Added — MQTT + Home Assistant integration (`night_light/`)

- **`mqtt_manager.h/.cpp`** — New module: MQTT client using PubSubClient, connects to configurable broker, publishes Home Assistant auto-discovery payloads (light, select, sensors), handles incoming light/slot commands, non-blocking reconnection
- **`config.h`** — Added `MqttConfig` struct and MQTT constants (port, reconnect interval, buffer size, keepalive)
- **`schedule.cpp`** — NVS persistence for MQTT settings (host, port, username, password, enabled flag)
- **`night_light.ino`** — Wired up `setupMqtt()` and `loopMqtt()`, added MQTT state publish on slot change
- **`web_server.cpp`** — New `POST /api/mqtt` endpoint for saving MQTT settings, MQTT status fields in `/api/status`, publish calls in brightness/effect/settings/schedule handlers
- **`html_pages.h`** — MQTT settings card in dashboard: enable toggle, broker host/port, username/password, connection status indicator

### Home Assistant entities auto-discovered

- **Light** (JSON schema) — on/off, brightness 0–255, RGB colour, 7 effects
- **Select** — Slot override: "Auto" + all slot labels
- **Sensor** — Active slot name (read-only)
- **Binary sensor** — NTP sync status (`device_class: connectivity`)
- **Sensor** — WiFi RSSI (`device_class: signal_strength`, diagnostic)

### Fixed

- **`led_effects.cpp`** — `effectSoftGlow()` gradient now spans one full wave across the entire strip instead of creating multiple short waves (was `i * 0.2f`, now `i / numLeds * 2 * PI`)

---

## 2026-01-27

### Added — Night Light project (`night_light/`)

New ESP32-C3 kids' night light that changes colour based on time of day.

- **`config.h`** — Hardware constants (GPIO2, 30 LEDs), schedule data structures (`ScheduleSlot`, `NightLightConfig`), effect type enum, timing defaults
- **`wifi_manager_setup.h/.cpp`** — WiFiManager captive portal for first-boot WiFi setup, NTP time sync with configurable timezone (default UK/BST)
- **`schedule.h/.cpp`** — Up to 8 time-based schedule slots stored in ESP32 NVS Preferences, overnight slot support (e.g. 20:00–06:00), default 3-slot bedtime schedule
- **`led_effects.h/.cpp`** — 7 gentle LED effects (Solid, Breathing, Soft Glow, Starry Twinkle, Rainbow, Candle Flicker, Sunrise), smooth slot-to-slot transitions
- **`web_server.h/.cpp`** — Web server on port 80 with cookie-based session authentication, dashboard, schedule editor, brightness/effect override API endpoints
- **`html_pages.h`** — Mobile-first dark theme UI with PROGMEM HTML templates: login page, dashboard with live status updates, full schedule editor with colour picker and time inputs
- **`night_light.ino`** — Main orchestrator: initialises hardware, connects WiFi, syncs NTP, evaluates schedule in loop, drives LED effects with rate-limiting and transition support
