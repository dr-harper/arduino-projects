/*
 * LED Grid — 16x16 Colour Effects + Web Portal
 *
 * Drives a 16x16 (256 LED) WS2812B grid with cycling colour effects,
 * AI Tetris, and manual Tetris control via web browser.
 *
 * Hardware:
 *   - ESP32-C3 Super Mini
 *   - 16x16 WS2812B LED grid (serpentine wiring), data on GPIO4
 *   - Adequate 5V power supply (256 LEDs need up to 15A at full white)
 *
 * First boot:
 *   1. Connect to "Tetris-Setup" WiFi (pass: tetris123)
 *   2. Configure your WiFi via the captive portal
 *   3. Access http://tetris.local (password: tetris)
 *
 * Libraries required (install via Library Manager):
 *   - Adafruit NeoPixel >= 1.12.0
 *   - WiFiManager (tzapu) >= 2.0.17
 *   - WebSockets (Links2004) >= 2.4.0
 *   - ArduinoJson >= 7.0.0
 */

#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "persistence.h"
#include "led_effects.h"
#include "tetris_effect.h"
#include "snake_game.h"
#include "wifi_setup.h"
#include "web_server.h"
#include "websocket_handler.h"
#include "mqtt_client.h"

// ─── Hardware ──────────────────────────────────────────────────────────────
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, LED_TYPE);

// ─── Configuration ─────────────────────────────────────────────────────────
GridConfig gridConfig;

// ─── BOOT button (GPIO9 on most ESP32-C3 boards) ──────────────────────────
#define BUTTON_PIN  9
static volatile bool buttonFlag = false;
static unsigned long lastButtonMs = 0;

void IRAM_ATTR buttonISR() {
    buttonFlag = true;
}

// ─── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println(F("\n=== LED Grid starting ==="));

    // Load configuration from NVS
    loadGridConfig(gridConfig);

    // Button for cycling effects
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

    // Flash onboard LED (GPIO8, active LOW) as a boot indicator
    pinMode(8, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(8, LOW);   // ON
        delay(200);
        digitalWrite(8, HIGH);  // OFF
        delay(200);
    }

    // Initialise the LED strip
    initLeds(strip);
    strip.setBrightness(gridConfig.brightness);

    // Apply config to game engines and clock
    setTetrisConfig(gridConfig);
    resetTetris();
    setClockUse24Hour(gridConfig.use24Hour);
    setClockTransition(gridConfig.clockTransition);
    setClockFadeMs(gridConfig.clockFadeMs);
    setClockMinMarker(gridConfig.clockMinMarker);
    setClockDigitColour(gridConfig.clockDigitColour);
    setClockTrail(gridConfig.clockTrail);
    setSnakeConfig(gridConfig);
    resetSnake();
    setWsActiveEffect(gridConfig.currentEffect);

    // WiFi setup (shows animation on grid during AP mode)
    setupWiFi(strip);

    // Web server + WebSocket + MQTT
    setupWebServer(gridConfig);
    setupWebSocket();
    setupMqtt(gridConfig);

    Serial.printf("Grid: %dx%d (%d LEDs), brightness: %d\n",
                  GRID_WIDTH, GRID_HEIGHT, NUM_LEDS, gridConfig.brightness);
    Serial.printf("Effect: %d/%d\n", gridConfig.currentEffect, EFFECT_COUNT);
    Serial.println(F("Press BOOT button to cycle effects"));
    Serial.println(F("=== LED Grid ready ==="));
}

// ─── Loop ──────────────────────────────────────────────────────────────────
void loop() {
    static unsigned long lastUpdateMs = 0;
    static uint8_t lastBrightness = 0;
    unsigned long now = millis();

    // Handle web server + WebSocket + MQTT
    loopWebServer();
    loopWebSocket();
    loopMqtt();

    // Apply brightness changes from web UI
    if (gridConfig.brightness != lastBrightness) {
        strip.setBrightness(gridConfig.brightness);
        lastBrightness = gridConfig.brightness;
    }

    // Check for button press (debounced)
    if (buttonFlag) {
        buttonFlag = false;
        if (now - lastButtonMs > 300) {
            lastButtonMs = now;
            gridConfig.currentEffect = nextEffect(gridConfig.currentEffect);
            strip.clear();
            strip.show();
            setWsActiveEffect(gridConfig.currentEffect);
            if (gridConfig.currentEffect == EFFECT_TETRIS) {
                setManualMode(false);
                resetTetris();
            } else if (gridConfig.currentEffect == EFFECT_SNAKE) {
                setSnakeManualMode(false);
                resetSnake();
            }
            mqttPublishState();
            Serial.printf("Effect: %d/%d\n", gridConfig.currentEffect, EFFECT_COUNT);
        }
    }

    // Update LEDs at target frame rate
    if (now - lastUpdateMs >= LED_UPDATE_INTERVAL_MS) {
        lastUpdateMs = now;
        updateEffect(strip, gridConfig.currentEffect);
    }
}
