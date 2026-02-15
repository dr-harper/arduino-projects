# Arduino Projects

A collection of ESP32 and ESP8266 projects built around addressable LED strips, grids, and panels. Each sub-project is a self-contained Arduino sketch with a web-based control interface.

## Projects

### Night Light (`night_light/`)

A children's night light that changes colour based on time of day. Features a configurable schedule with smooth transitions between time slots.

**Hardware:** ESP32-C3, 55x SK6812 RGBW LEDs

**Features:**
- 8 configurable schedule slots with RGBW colour, brightness, and effects
- 10 LED effects (Solid, Breathing, Soft Glow, Starry Twinkle, Rainbow, Candle Flicker, Sunrise, Christmas, Halloween, Birthday)
- Special date triggers (e.g. birthdays, holidays) with auto-activation
- Mobile-first dark-theme web UI with live status dashboard
- Optional MQTT integration with Home Assistant auto-discovery
- OTA firmware updates via the web UI
- Thermal protection using the ESP32-C3's internal temperature sensor
- WiFiManager captive portal for initial WiFi setup with standalone light controls in AP mode

**Access:** `http://nightlight.local` (default password: `nightlight`)

---

### LED Grid (`led_grid/`)

A 16x16 WS2812B LED matrix with visual effects and playable games.

**Hardware:** ESP32-C3 Super Mini, 16x16 WS2812B matrix (256 LEDs)

**Features:**
- 17 visual effects including Clock, Rainbow Wave, Fire, Aurora, Matrix, Fireworks, Game of Life, Plasma, and more
- Playable Tetris and Snake via WebSocket phone controls (with AI autoplay mode)
- Configurable game speed, AI skill level, and clock display options
- MQTT integration for Home Assistant
- OTA firmware updates
- Gzip-compressed web UI served from flash

**Access:** `http://tetris.local` (default password: `tetris`)

---

### LED Panel (`led_panel/`)

An 8x32 WS2812B panel — a fork of the LED Grid adapted for a wider, shorter form factor.

**Hardware:** ESP32-C3 Super Mini, 8x32 WS2812B panel (256 LEDs)

**Features:**
- Same effect library as LED Grid, tuned for the 32x8 aspect ratio
- Clock effect with 4x6 pixel font for improved legibility
- Column-major serpentine LED mapping with 180-degree rotation (panel mounted upside-down)
- Playable Tetris and Snake via phone controls
- OTA firmware updates

**Access:** `http://ledpanel.local` (default password: `ledpanel`)

---

### LED Tie (`led_tie/`)

A wearable LED tie with scrolling text, 14 animation modes, and an OLED display. Built for a work event — it has a "flirt mode" and an "avoidant mode" triggered by holding the button.

**Hardware:** ESP8266 (Wemos D1 Mini), 70x WS2812B LEDs, SSD1306 OLED display, button

**Features:**
- 14 LED animation modes with auto-cycle
- Customisable scrolling phrases (stored in EEPROM)
- Physical button: single press cycles modes, hold activates special modes
- WiFi AP with web UI for live control and phrase editing

---

### AI Camera (`ai_camera/`)

An ESP32-S3 camera that captures images and runs on-device MobileNet classification via TensorFlow Lite Micro.

**Hardware:** Seeed Studio XIAO ESP32S3 Sense, OV2640 camera, 8MB PSRAM

**Features:**
- On-device image classification (MobileNet V1 via TFLite Micro)
- PSRAM ring buffer for storing up to 10 captured images
- Live camera stream and still capture via web UI
- Manual tagging and classification correction
- Button-triggered capture with LED feedback

**Setup:** Before compiling, generate the ML model:
```bash
cd ai_camera/tools && python3 generate_model.py
```

**Access:** `http://aicamera.local` (default password: `aicamera`)

---

### Carbon Intensity LEDs (`carbon_intensity/`)

An early experiment — an ESP8266 with 10 LEDs intended to visualise UK grid carbon intensity. Currently just a skeleton with WiFiManager setup.

**Hardware:** ESP8266, 10x WS2812B LEDs

---

## Getting Started

### Prerequisites

- [Arduino CLI](https://arduino.github.io/arduino-cli/) or the Arduino IDE
- ESP32 board support package (`esp32` by Espressif)
- ESP8266 board support package (for `led_tie` and `carbon_intensity` only)

### Common Libraries

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

### Flashing

**First flash (new chip via USB):**
```bash
arduino-cli upload \
  --fqbn esp32:esp32:nologo_esp32c3_super_mini \
  --port /dev/cu.usbmodem101 \
  --input-dir build/esp32.esp32.nologo_esp32c3_super_mini/
```

**Subsequent flashes (OTA via WiFi):**
```bash
# Set your device password (required for OTA upload)
export LED_GRID_PASS="your-device-password"
export LED_PANEL_PASS="your-device-password"

# Optionally override the default hostname/IP
export LED_GRID_IP="192.168.1.25"

# From the project directory (led_grid/ or led_panel/):
./build.sh --upload              # Uses mDNS hostname
./build.sh --upload 192.168.1.x  # Or specify an IP
```

The build script compresses HTML pages, compiles the firmware, and uploads via the device's web UI.

### Initial Setup

1. Power on the device — it creates a WiFi access point (e.g. "NightLight-Setup")
2. Connect to the AP from your phone
3. The captive portal opens automatically — select your home WiFi and enter the password
4. The device reboots and connects to your network
5. Access the web UI via `http://<hostname>.local`

### Default Passwords

All devices ship with simple default passwords (shown above). These can be changed via the Settings page in each device's web UI.

## Licence

This project is licensed under the MIT Licence — see [LICENSE](LICENSE) for details.
