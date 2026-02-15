<h1 align="center">Arduino Projects</h1>

<p align="center">
  <strong>A collection of ESP32 & ESP8266 projects — LED grids, smart night lights, an AI camera, and a wearable LED tie.</strong>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/licence-MIT-blue.svg" alt="MIT Licence"></a>
  <img src="https://img.shields.io/badge/platform-ESP32%20%7C%20ESP8266-orange.svg" alt="Platform">
  <img src="https://img.shields.io/badge/framework-Arduino-teal.svg" alt="Arduino">
  <img src="https://img.shields.io/badge/LEDs-WS2812B%20%7C%20SK6812-ff69b4.svg" alt="LEDs">
</p>

<p align="center">
  Every project runs a built-in web server with a mobile-friendly control panel — no app required, just connect and go.
</p>

<!-- TODO: Add a hero image/collage of the projects here -->
<!-- <p align="center"><img src="docs/images/hero.jpg" width="700"></p> -->

---

## Projects at a Glance

| Project | Hardware | LEDs | Highlights |
|---------|----------|------|------------|
| [Night Light](#night-light) | ESP32-C3 | 55x SK6812 RGBW | Time-based schedule, MQTT/Home Assistant, 10 effects |
| [LED Grid](#led-grid) | ESP32-C3 Super Mini | 16x16 WS2812B (256) | Tetris, Snake, 17 effects, phone controls |
| [LED Panel](#led-panel) | ESP32-C3 Super Mini | 8x32 WS2812B (256) | Clock display, games, 17 effects |
| [LED Tie](#led-tie) | ESP8266 (Wemos D1 Mini) | 70x WS2812B + OLED | Wearable, scrolling text, 14 modes |
| [AI Camera](#ai-camera) | XIAO ESP32S3 Sense | - | On-device ML image classification |
| [Carbon Intensity](#carbon-intensity-leds) | ESP8266 | 10x WS2812B | Early experiment (skeleton) |

---

## Night Light

> A children's night light that changes colour on a schedule — warm amber at bedtime, dim red for sleep, soft green for "OK to wake".

<!-- TODO: Add photo -->
<!-- <p align="center"><img src="docs/images/night_light.jpg" width="500"></p> -->

**Hardware:** ESP32-C3 + 55x SK6812 RGBW LEDs

<details>
<summary><strong>Features</strong></summary>

- **Scheduled colours** — Up to 8 time slots, each with RGBW colour, brightness, and effect. Smooth crossfade transitions between slots.
- **10 LED effects** — Solid, Breathing, Soft Glow, Starry Twinkle, Rainbow, Candle Flicker, Sunrise, Christmas, Halloween, Birthday
- **Special dates** — Configure up to 4 dates (birthdays, holidays) that automatically override the schedule with a themed effect
- **Home Assistant** — Optional MQTT with auto-discovery. Full bidirectional sync: colour, brightness, effects, schedule slots, diagnostics
- **Thermal protection** — Reads the ESP32-C3's internal temperature sensor; linearly dims LEDs above 65 C and cuts off at 80 C
- **Web UI** — Mobile-first dark theme with dashboard, schedule editor, settings, and OTA firmware updates
- **AP mode controls** — When no WiFi is configured, a standalone colour picker and effect selector runs alongside the captive portal
- **Reliability** — Hardware watchdog, WiFi auto-reconnect, 64-bit uptime counter, login rate limiting

</details>

<details>
<summary><strong>Home Assistant Entities</strong></summary>

| Entity | Type | Details |
|--------|------|---------|
| Light | `light` | RGBW colour, brightness, 10 effects |
| Active Slot | `sensor` | Current schedule slot name |
| Slot Override | `select` | "Auto" + all slot labels |
| NTP Status | `binary_sensor` | Time sync connectivity |
| WiFi RSSI | `sensor` | Signal strength (diagnostic) |
| Uptime | `sensor` | Duration in seconds (diagnostic) |
| Free Memory | `sensor` | Heap bytes (diagnostic) |
| Chip Temperature | `sensor` | Internal temp in C (diagnostic) |
| Special Date | `sensor` | Active date label |

</details>

**Access:** `http://nightlight.local` | Default password: `nightlight`

**[Full changelog](CHANGELOG.md)**

---

## LED Grid

> A 16x16 pixel display that plays Tetris by itself, lets you play Snake from your phone, and doubles as a clock.

<!-- TODO: Add photo/GIF -->
<!-- <p align="center"><img src="docs/images/led_grid.gif" width="500"></p> -->

**Hardware:** ESP32-C3 Super Mini + 16x16 WS2812B matrix (256 LEDs)

<details>
<summary><strong>17 Visual Effects</strong></summary>

| Effect | Description |
|--------|-------------|
| Tetris | AI-driven pieces fall and stack (configurable skill/speed) |
| Rainbow Wave | Rainbow ripple across the grid |
| Colour Wash | Entire grid fades through hues |
| Diagonal Rainbow | Rainbow bands on the diagonal |
| Rain | Coloured drops falling down columns |
| Clock | NTP-synced digital clock (12/24h, crossfade transitions) |
| Fire | Heat-based fire simulation |
| Aurora | Flowing green/purple northern lights |
| Lava | Slow drifting metaball blobs |
| Candle | Warm flickering candlelight |
| Twinkle | Random twinkling starfield |
| Matrix | Green falling code streams |
| Fireworks | Particle launch and burst |
| Game of Life | Conway's Life with colour |
| Plasma | Sine-wave interference patterns |
| Spiral | Rotating colour pinwheel |
| Valentine's | Pulsing heart with sparkles |
| Snake | AI or manual phone control |

</details>

<details>
<summary><strong>Games</strong></summary>

- **Tetris** — AI plays automatically with tuneable skill level and speed. Switch to manual mode and play on your phone with touch controls or keyboard arrows.
- **Snake** — AI autopilot or manual control via WebSocket, played from the browser.

Both games use a live WebSocket connection for responsive phone controls.

</details>

<details>
<summary><strong>Clock Features</strong></summary>

- 12 or 24-hour format
- Crossfade digit transitions
- 6 colour presets (warm white, cool white, amber, green, blue, red)
- Orbiting second trail on the border
- Minute position marker

</details>

**Access:** `http://tetris.local` | Default password: `tetris`

**[Detailed README](led_grid/README.md)**

---

## LED Panel

> The LED Grid's wider sibling — an 8x32 panel that makes a great desk clock with scrolling effects.

<!-- TODO: Add photo -->
<!-- <p align="center"><img src="docs/images/led_panel.jpg" width="500"></p> -->

**Hardware:** ESP32-C3 Super Mini + 8x32 WS2812B panel (256 LEDs)

<details>
<summary><strong>Features</strong></summary>

- Same 17-effect library as the LED Grid, tuned for the 32x8 aspect ratio
- Clock effect with 4x6 pixel font for improved legibility
- Playable Tetris and Snake via phone controls
- Column-major serpentine LED mapping with 180-degree rotation (panel mounted upside-down in its case)
- OTA firmware updates via the web UI

</details>

**Access:** `http://ledpanel.local` | Default password: `ledpanel`

---

## LED Tie

> A wearable LED tie built for a work event. Features a "flirt mode" and an "avoidant mode" activated by holding the button.

<!-- TODO: Add photo -->
<!-- <p align="center"><img src="docs/images/led_tie.jpg" width="500"></p> -->

**Hardware:** ESP8266 (Wemos D1 Mini) + 70x WS2812B LEDs + SSD1306 OLED display + button

<details>
<summary><strong>Features</strong></summary>

- 14 LED animation modes with auto-cycle
- Customisable scrolling phrases stored in EEPROM
- OLED display for mode names and status
- Physical button: single press cycles modes, 2s hold activates flirt mode, 3s hold activates avoidant mode
- WiFi AP with web UI for live control and phrase editing
- Configurable brightness levels via double-tap

</details>

---

## AI Camera

> An ESP32-S3 camera that captures images and classifies them on-device using MobileNet — no cloud required.

<!-- TODO: Add photo -->
<!-- <p align="center"><img src="docs/images/ai_camera.jpg" width="500"></p> -->

**Hardware:** Seeed Studio XIAO ESP32S3 Sense + OV2640 camera + 8MB PSRAM

<details>
<summary><strong>Features</strong></summary>

- On-device image classification (MobileNet V1 via TensorFlow Lite Micro)
- PSRAM ring buffer storing up to 10 captured images
- Live camera stream and still capture via web UI
- Manual tagging and classification correction
- Button-triggered capture with LED feedback

</details>

**Setup:** Before compiling, generate the ML model:
```bash
cd ai_camera/tools && python3 generate_model.py
```

**Access:** `http://aicamera.local` | Default password: `aicamera`

---

## Carbon Intensity LEDs

An early experiment — an ESP8266 with 10 LEDs intended to visualise UK grid carbon intensity. Currently just a skeleton with WiFiManager setup.

---

## Getting Started

### Prerequisites

- [Arduino CLI](https://arduino.github.io/arduino-cli/) or the Arduino IDE
- ESP32 board support package (`esp32` by Espressif)
- ESP8266 board support package (for `led_tie` and `carbon_intensity` only)

### Libraries

Install via the Arduino Library Manager or `arduino-cli lib install`:

| Library | Used by |
|---------|---------|
| [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) | All projects |
| [WiFiManager](https://github.com/tzapu/WiFiManager) (tzapu) | All projects |
| [PubSubClient](https://github.com/knolleary/pubsubclient) | night_light, led_grid |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | led_grid, led_panel |
| [WebSockets](https://github.com/Links2004/arduinoWebSockets) | led_grid, led_panel |
| [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) | led_tie |
| [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) | led_tie |

### Initial Setup

Every project follows the same pattern:

1. **Flash the firmware** via USB (first time) or OTA (subsequent updates)
2. **Connect to the setup AP** — the device creates a WiFi network (e.g. "NightLight-Setup")
3. **Configure WiFi** — the captive portal opens automatically on your phone
4. **Open the web UI** — access `http://<hostname>.local` from any device on your network

All default passwords can be changed via the Settings page in each device's web UI.

### Flashing

<details>
<summary><strong>First flash (USB)</strong></summary>

Connect the ESP32 via a data-capable USB cable. If the port isn't detected, hold the BOOT button while plugging in.

```bash
arduino-cli upload \
  --fqbn esp32:esp32:nologo_esp32c3_super_mini \
  --port /dev/cu.usbmodem101 \
  --input-dir build/esp32.esp32.nologo_esp32c3_super_mini/
```

</details>

<details>
<summary><strong>OTA updates (WiFi)</strong></summary>

Once the device is on your network, use the build script for wireless updates:

```bash
# Set your device password (required for OTA upload)
export LED_GRID_PASS="your-device-password"
export LED_PANEL_PASS="your-device-password"

# Optionally override the default hostname/IP
export LED_GRID_IP="192.168.1.25"

# From the project directory:
./build.sh                       # Compile only
./build.sh --upload              # Compile + OTA upload (mDNS)
./build.sh --upload 192.168.1.x  # Compile + OTA upload (IP)
```

The build script compresses HTML pages, compiles the firmware, and uploads via the device's web UI.

</details>

## Licence

This project is licensed under the MIT Licence — see [LICENSE](LICENSE) for details.
