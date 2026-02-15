# LED Grid

A 16x16 WS2812B LED grid driven by an ESP32-C3, featuring 18 visual effects, AI Tetris, playable Snake, a digital clock, and a mobile-friendly web portal. Integrates with Home Assistant via MQTT auto-discovery.

## Hardware

| Component | Detail |
|-----------|--------|
| MCU | ESP32-C3 Super Mini |
| LEDs | 16x16 WS2812B grid (256 LEDs), serpentine wiring |
| Data pin | GPIO4 |
| Button | BOOT button (GPIO9) — cycles effects |
| Power | 5V supply (up to 15A at full white) |

### Wiring

```
ESP32-C3 GPIO4 ──── DIN (WS2812B grid)
ESP32-C3 GND   ──── GND (shared with PSU)
5V PSU         ──── 5V  (WS2812B grid)
```

> Add a 300-1000 ohm resistor on the data line and a large capacitor (1000uF) across the 5V power rails to protect the LEDs.

## Features

### 18 Visual Effects

| # | Effect | Description |
|---|--------|-------------|
| 0 | Tetris | AI-driven Tetris simulation with configurable skill/speed |
| 1 | Rainbow Wave | Rainbow ripple across the grid |
| 2 | Colour Wash | Entire grid fades through hues |
| 3 | Diagonal Rainbow | Rainbow bands on the diagonal |
| 4 | Rain | Coloured drops falling down columns |
| 5 | Clock | Digital clock via NTP (12/24h, crossfade, customisable colours) |
| 6 | Fire | Heat-based fire simulation |
| 7 | Aurora | Flowing green/purple northern lights |
| 8 | Lava | Slow drifting metaball blobs |
| 9 | Candle | Warm flickering candlelight |
| 10 | Twinkle | Random twinkling starfield |
| 11 | Matrix | Green falling code streams |
| 12 | Fireworks | Particle launch and burst |
| 13 | Life | Conway's Game of Life with colour |
| 14 | Plasma | Sine-wave interference patterns |
| 15 | Spiral | Rotating colour pinwheel |
| 16 | Valentines | Pulsing heart with sparkles |
| 17 | Snake | Snake game — AI or manual phone control |

### Games

- **Tetris** — AI plays automatically with tuneable skill level and speed. Switch to manual mode via the web UI and play on your phone with touch controls or keyboard arrows.
- **Snake** — AI or manual control via WebSocket, played from the browser.

### Digital Clock

- NTP-synced time display with 12 or 24-hour format
- Crossfade digit transitions
- 6 digit colour presets (warm white, cool white, amber, green, blue, red)
- Orbiting second trail on the border
- Minute position marker
- Leading zero suppression with centred single digits

### Web Portal

Mobile-first dark-themed control panel at `http://tetris.local`:

- **Dashboard** — Brightness slider, effect selector, AI tuning, background colour picker
- **Settings** — Clock display options, Tetris AI tuning, network/MQTT configuration, system info
- **Manual Tetris** — Touch/keyboard controls with live WebSocket grid rendering
- **OTA Updates** — Upload firmware binaries from the browser

Protected by session-based authentication with rate limiting.

### Home Assistant Integration

MQTT auto-discovery creates entities automatically:

| Entity | Type | Details |
|--------|------|---------|
| Light | `light` | On/off, brightness (0-255), effect selection (all 18) |
| IP Address | `sensor` | Device IP (diagnostic) |
| Uptime | `sensor` | Formatted uptime (diagnostic) |
| Free Heap | `sensor` | Available memory in KB (diagnostic) |

**Bidirectional sync** — Changes from HA, the web UI, or the physical button all stay in sync. The brightness slider appears at the top level of the HA light card.

**On/Off semantics** — "OFF" sets brightness to 0 and remembers the previous value. "ON" restores it. No master power switch needed.

## First Boot

1. Power on the ESP32-C3
2. Connect to the **Tetris-Setup** WiFi network (password: `tetris123`)
3. Configure your home WiFi via the captive portal
4. Access **http://tetris.local** (password: `tetris`)
5. Press the BOOT button to cycle through effects

## Building

### Prerequisites

- [Arduino CLI](https://arduino.github.io/arduino-cli/)
- ESP32 board package (`esp32:esp32`)
- Python 3 (for HTML compression)

### Libraries

Install via Arduino Library Manager or `arduino-cli lib install`:

| Library | Version |
|---------|---------|
| Adafruit NeoPixel | >= 1.12.0 |
| WiFiManager (tzapu) | >= 2.0.17 |
| WebSockets (Links2004) | >= 2.4.0 |
| ArduinoJson | >= 7.0.0 |
| PubSubClient | >= 2.8 |

### Compile

```bash
cd led_grid

# Compile only
./build.sh

# Compile + OTA upload (set password first)
export LED_GRID_PASS="your-device-password"
./build.sh --upload              # Uses tetris.local by default
./build.sh --upload 192.168.1.x  # Or specify an IP
```

The build script:
1. Compresses HTML pages into gzipped C headers (`compress_html.py`)
2. Compiles with `arduino-cli`
3. Optionally uploads via OTA (HTTP, not ArduinoOTA)

### Flash Size

The ESP32-C3 Super Mini has a 4MB flash chip with a ~1.25MB app partition. The firmware currently uses ~93% of the app partition (~1.22MB), leaving ~85KB for future additions.

## MQTT Setup

1. Go to **Settings > Network > MQTT** on the web portal
2. Enter your MQTT broker host, port, and credentials
3. Tick "Enable MQTT" and save
4. The device appears in Home Assistant automatically

### Topics

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `ledgrid/<ID>/availability` | Device -> HA | Online/offline (LWT) |
| `ledgrid/<ID>/light/state` | Device -> HA | JSON state (brightness, effect, on/off) |
| `ledgrid/<ID>/light/set` | HA -> Device | JSON commands |
| `ledgrid/<ID>/diagnostics` | Device -> HA | IP, uptime, free heap |
| `homeassistant/light/ledgrid_<ID>/config` | Device -> HA | Auto-discovery |

`<ID>` is the last 6 hex characters of the device's MAC address.

## Project Structure

```
led_grid/
  led_grid.ino          Main sketch (setup/loop)
  config.h              Hardware constants, effect enum, config structs
  persistence.h/.cpp    NVS load/save for all settings
  led_effects.h/.cpp    All 18 visual effects + clock display
  tetris_effect.h/.cpp  Tetris game engine (AI + manual)
  snake_game.h/.cpp     Snake game engine (AI + manual)
  web_server.h/.cpp     HTTP routes, API endpoints, OTA updates
  websocket_handler.h/.cpp  WebSocket for live game control
  wifi_setup.h/.cpp     WiFiManager captive portal + mDNS
  mqtt_client.h/.cpp    MQTT client, HA auto-discovery, state sync
  html_pages.h          Raw HTML/CSS/JS for all web pages
  html_pages_gz.h       Gzip-compressed pages (auto-generated)
  compress_html.py      Build tool: compresses HTML into C headers
  build.sh              Build + OTA upload script
```

## Configuration

All settings persist across reboots via ESP32 NVS (non-volatile storage). Configurable from the web UI or MQTT:

- **Brightness** (0-255)
- **Active effect** (0-17)
- **Background colour** (RGB, 0-40 per channel)
- **Tetris AI** — skill level (0-100%), drop speed, move/rotate intervals, jitter
- **Clock** — 12/24h format, transition mode, fade duration, digit colour, second trail, minute marker
- **MQTT** — broker host/port, username/password, enable/disable
- **Auth password**

## Licence

MIT — see [LICENSE](../LICENSE) for details.
