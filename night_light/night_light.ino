/*
 * Night Light — ESP32-C3 Kids' Night Light
 *
 * A time-aware night light that connects to home WiFi, syncs the
 * clock via NTP, and changes LED colour/brightness on a configurable
 * schedule.  Parents control everything through a mobile-friendly
 * web interface protected by a simple password.
 *
 * Hardware:
 *   - ESP32-C3 dev board
 *   - 30x WS2812B addressable LEDs on GPIO2
 *
 * Libraries required (install via Library Manager):
 *   - Adafruit NeoPixel  >= 1.12.0
 *   - WiFiManager (tzapu) >= 2.0.17
 *   - PubSubClient        >= 2.8 (for MQTT / Home Assistant)
 */

#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "wifi_manager_setup.h"
#include "schedule.h"
#include "led_effects.h"
#include "web_server.h"
#include "mqtt_manager.h"

// ─── Global objects ─────────────────────────────────────────────────────────
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, LED_TYPE);
NightLightConfig  config;

// ─── State ──────────────────────────────────────────────────────────────────
int8_t   currentSlotIndex    = -1;   // Currently active schedule slot (-1 = none)
int8_t   previousSlotIndex   = -1;   // For transition tracking
bool     manualOverride      = false; // "Pretty mode" — manual colour/effect override
uint8_t  overrideBrightness  = 0;
uint8_t  overrideEffect      = EFFECT_SOLID;
uint8_t  overrideR = 0, overrideG = 0, overrideB = 0, overrideW = 0;

// Independent brightness — persists across mode and slot changes
bool     userBrightnessActive = false;
uint8_t  userBrightness       = 128;

// Special date state
int8_t  currentSpecialDateIndex  = -1;
bool    specialDateOverrideActive = false;
bool    specialDateDismissed      = false;
uint8_t lastSpecialDateDay       = 0;

// Transition state
bool           inTransition       = false;
unsigned long  transitionStartMs  = 0;
unsigned long  transitionDurationMs = 0;

// Thermal monitoring
float    chipTempC          = 0.0f;   // Latest chip temperature reading
float    thermalDimFactor   = 1.0f;   // 1.0 = no throttle, 0.0 = fully throttled
bool     thermalShutdown    = false;  // True when above critical threshold
unsigned long lastTempRead  = 0;

// Master fade (smooth on/off toggle)
float         masterFadeFactor = 1.0f;  // 1.0 = fully on, 0.0 = fully off
bool          fadeActive       = false;
unsigned long fadeStartMs      = 0;

// 64-bit uptime (survives millis() overflow after ~49 days)
uint64_t uptimeMs = 0;

// Timing
unsigned long lastLedUpdate = 0;

// ─── Setup ──────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("\n=== Night Light starting ==="));

    // Hardware watchdog — reboots if loop() stalls for 10 seconds
    const esp_task_wdt_config_t wdtCfg = {
        .timeout_ms = 10000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdtCfg);
    esp_task_wdt_add(NULL);

    // Initialise LED strip
    strip.begin();
    strip.setBrightness(50);
    strip.show();
    Serial.println(F("LEDs initialised"));

    // Load configuration from NVS (or defaults)
    loadSchedule(config);
    Serial.print(F("Schedule loaded: "));
    Serial.print(config.numSlots);
    Serial.println(F(" slots"));

    // Rainbow wave so the LEDs light up immediately on power-on,
    // then settle to a warm fallback colour through WiFi + NTP setup.
    playBootAnimation(strip, 2500);

    // Hold warm fallback colour while WiFi/NTP connect
    strip.setBrightness(255);
    for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(FALLBACK_R, FALLBACK_G, FALLBACK_B, FALLBACK_W));
    }
    strip.show();
    Serial.println(F("Boot animation complete"));

    // Connect to WiFi via WiFiManager captive portal (pulses LEDs while waiting)
    setupWiFiManager(strip);

    // Sync time via NTP
    setupNTP(config.timezone);

    // Start the web server
    setupWebServer(config, strip);

    // Start MQTT (connects if enabled and configured)
    setupMqtt(config);

    // ArduinoOTA — allows firmware upload from the Arduino IDE over WiFi
    ArduinoOTA.setHostname(MDNS_HOSTNAME);
    ArduinoOTA.setPassword(config.authPassword);
    ArduinoOTA.onStart([]() {
        // Disable watchdog during OTA — flash writes stall the loop
        // and would otherwise trigger a panic reboot mid-update
        esp_task_wdt_delete(NULL);
        strip.clear();
        strip.show();
        Serial.println(F("OTA update starting..."));
    });
    ArduinoOTA.onEnd([]() {
        Serial.println(F("\nOTA update complete — rebooting"));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.print(F("OTA error: "));
        Serial.println(error);
        // Re-enable watchdog after failed OTA so normal operation is still protected
        esp_task_wdt_add(NULL);
    });
    ArduinoOTA.begin();
    Serial.println(F("ArduinoOTA ready"));

    Serial.println(F("=== Night Light ready ==="));
}

// ─── Loop ───────────────────────────────────────────────────────────────────
void loop() {
    esp_task_wdt_reset();

    // 0. Track 64-bit uptime (handles millis() overflow after ~49 days)
    {
        static uint32_t lastMillis = 0;
        static uint32_t overflowCount = 0;
        uint32_t nowMs = millis();
        if (nowMs < lastMillis) overflowCount++;
        lastMillis = nowMs;
        uptimeMs = ((uint64_t)overflowCount << 32) | nowMs;
    }

    // 1. Handle web requests, MQTT, and OTA
    ArduinoOTA.handle();
    handleWebClient();
    loopMqtt();

    // 1a. Periodic WiFi reconnection check
    unsigned long now = millis();
    {
        static unsigned long lastWifiCheck = 0;
        static uint8_t wifiFailCount = 0;
        if (now - lastWifiCheck >= WIFI_CHECK_INTERVAL_MS) {
            lastWifiCheck = now;
            if (WiFi.status() != WL_CONNECTED) {
                wifiFailCount++;
                WiFi.reconnect();
                Serial.printf("WiFi disconnected — reconnect attempt %d\n", wifiFailCount);
                if (wifiFailCount >= WIFI_MAX_FAILURES) {
                    Serial.println(F("WiFi reconnect failed — rebooting"));
                    ESP.restart();
                }
            } else {
                wifiFailCount = 0;
            }
        }
    }

    // 2. Periodic chip temperature reading + thermal protection (with hysteresis)
    if (now - lastTempRead >= TEMP_READ_INTERVAL_MS) {
        lastTempRead = now;
        chipTempC = temperatureRead();

        if (chipTempC >= THERMAL_CRIT_C) {
            thermalDimFactor = 0.0f;
            thermalShutdown  = true;
        } else if (thermalShutdown && chipTempC >= THERMAL_WARN_CLEAR_C) {
            // Stay in shutdown until cooled below clear threshold
            thermalDimFactor = 0.0f;
        } else if (chipTempC >= THERMAL_WARN_C) {
            // Linear ramp-down from 1.0 at WARN to 0.0 at CRIT
            thermalDimFactor = 1.0f - (chipTempC - THERMAL_WARN_C) / (THERMAL_CRIT_C - THERMAL_WARN_C);
            thermalShutdown  = false;
        } else {
            thermalDimFactor = 1.0f;
            thermalShutdown  = false;
        }
    }

    // 2a. Rate-limit LED updates
    if (now - lastLedUpdate < LED_UPDATE_INTERVAL) {
        return;
    }
    lastLedUpdate = now;

    // 3. Master fade — smooth transition when toggling LEDs on/off
    {
        static bool prevLedsEnabled = true;
        if (config.ledsEnabled != prevLedsEnabled) {
            fadeActive  = true;
            fadeStartMs = now;
            prevLedsEnabled = config.ledsEnabled;
        }

        if (fadeActive) {
            float progress = (float)(now - fadeStartMs) / (float)FADE_DURATION_MS;
            if (progress >= 1.0f) {
                progress   = 1.0f;
                fadeActive  = false;
            }
            masterFadeFactor = config.ledsEnabled ? progress : (1.0f - progress);
        } else {
            masterFadeFactor = config.ledsEnabled ? 1.0f : 0.0f;
        }

        if (!config.ledsEnabled && masterFadeFactor <= 0.0f) {
            strip.clear();
            strip.show();
            return;
        }
    }

    // 3a. Thermal shutdown — force LEDs off if chip is critically hot
    if (thermalShutdown) {
        strip.clear();
        strip.show();
        return;
    }

    // 4. Get current time — needed for schedule evaluation regardless of mode
    int hour, minute;
    bool timeValid = getCurrentTime(hour, minute);

    if (!timeValid) {
        // NTP not synced — show fallback warm white with user brightness if set
        uint8_t fallbackBri = userBrightnessActive ? userBrightness : FALLBACK_BRIGHTNESS;
        applyEffect(strip, EFFECT_SOLID,
                    FALLBACK_R, FALLBACK_G, FALLBACK_B, FALLBACK_W, fallbackBri, now);
        return;
    }

    // 5. Evaluate special dates (birthdays, holidays)
    {
        int year, month, day;
        if (getCurrentDate(year, month, day)) {
            // Reset dismissed flag when date changes
            if ((uint8_t)day != lastSpecialDateDay) {
                lastSpecialDateDay = (uint8_t)day;
                specialDateDismissed = false;
            }

            int8_t sdIdx = evaluateSpecialDates(config, (uint8_t)month, (uint8_t)day);

            if (sdIdx >= 0 && !specialDateDismissed) {
                if (!manualOverride && !specialDateOverrideActive) {
                    // Activate special date override (enters pretty mode)
                    const SpecialDate &sd = config.specialDates[sdIdx];
                    overrideEffect     = sd.effect;
                    overrideBrightness = sd.brightness;
                    overrideR = sd.red;
                    overrideG = sd.green;
                    overrideB = sd.blue;
                    overrideW = sd.white;
                    manualOverride = true;
                    specialDateOverrideActive = true;
                    currentSpecialDateIndex = sdIdx;
                    mqttPublishLightState();
                } else if (specialDateOverrideActive && !manualOverride) {
                    // User cleared the override during the special date — dismiss it
                    specialDateDismissed = true;
                    specialDateOverrideActive = false;
                    currentSpecialDateIndex = -1;
                }
            } else if (sdIdx < 0 && specialDateOverrideActive) {
                // Date no longer matches — release override
                manualOverride = false;
                specialDateOverrideActive = false;
                currentSpecialDateIndex = -1;
                mqttPublishLightState();
            }
        }
    }

    // 6. Evaluate schedule — ALWAYS runs so currentSlotIndex stays accurate
    int8_t slotIdx = evaluateSchedule(config, hour, minute);

    if (slotIdx != currentSlotIndex) {
        previousSlotIndex = currentSlotIndex;
        currentSlotIndex  = slotIdx;

        // Only set up visual transition when in schedule mode
        if (!manualOverride && config.transitionMinutes > 0
            && previousSlotIndex >= 0 && currentSlotIndex >= 0) {
            inTransition        = true;
            transitionStartMs   = now;
            transitionDurationMs = (unsigned long)config.transitionMinutes * 60UL * 1000UL;
        } else {
            inTransition = false;
        }

        // Notify MQTT of slot change
        mqttPublishLightState();
    }

    // 7. Render LEDs — pretty mode uses override colour/effect,
    //    schedule mode uses the active slot.  Brightness is always independent.
    if (manualOverride) {
        uint8_t bri = userBrightnessActive ? userBrightness : overrideBrightness;
        applyEffect(strip, (EffectType)overrideEffect,
                    overrideR, overrideG, overrideB, overrideW, bri, now);
    } else if (inTransition) {
        float progress = (float)(now - transitionStartMs) / (float)transitionDurationMs;
        if (progress >= 1.0f) {
            inTransition = false;
            progress = 1.0f;
        }
        if (previousSlotIndex >= 0 && currentSlotIndex >= 0) {
            const ScheduleSlot &from = config.slots[previousSlotIndex];
            const ScheduleSlot &to   = config.slots[currentSlotIndex];
            int16_t briOverride = userBrightnessActive ? (int16_t)userBrightness : -1;
            applyTransition(strip, from, to, progress, now, briOverride);
        }
    } else if (currentSlotIndex >= 0) {
        const ScheduleSlot &slot = config.slots[currentSlotIndex];
        uint8_t bri = userBrightnessActive ? userBrightness : slot.brightness;
        applyEffect(strip, (EffectType)slot.effect,
                    slot.red, slot.green, slot.blue, slot.white, bri, now);
    } else {
        // Outside all schedule slots — warm amber default
        uint8_t bri = userBrightnessActive ? userBrightness : DEFAULT_BRIGHTNESS;
        applyEffect(strip, EFFECT_SOLID,
                    DEFAULT_R, DEFAULT_G, DEFAULT_B, DEFAULT_W, bri, now);
    }
}
