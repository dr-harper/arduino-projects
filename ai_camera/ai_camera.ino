/*
 * AI Camera — ESP32-S3 Image Classification & Tagging
 *
 * Captures images via the OV2640 camera on the XIAO ESP32S3 Sense,
 * runs on-device MobileNet classification via TFLite Micro, stores
 * images in a PSRAM ring buffer, and serves a web UI for viewing,
 * classifying, and tagging images.
 *
 * Hardware:
 *   - Seeed Studio XIAO ESP32S3 Sense
 *   - OV2640 camera module (built-in)
 *   - 8MB PSRAM, 8MB Flash
 *
 * Arduino IDE Settings:
 *   - Board: XIAO_ESP32S3
 *   - PSRAM: OPI PSRAM (must be enabled)
 *   - Flash Size: 8MB
 *   - Partition: 8MB with spiffs (3MB APP / 1.5MB SPIFFS) or Huge APP
 *
 * Libraries required (install via Library Manager):
 *   - WiFiManager (tzapu) >= 2.0.17
 *   - TensorFlowLite_ESP32 (for ML inference)
 *
 * Before compiling, run the model generator script:
 *   cd ai_camera/tools && python3 generate_model.py
 *   This downloads a pre-trained MobileNet V1 and generates
 *   model_data.h and classifier_labels.h
 */

#include <WiFi.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "camera_manager.h"
#include "wifi_manager_setup.h"
#include "web_server.h"
#include "button_handler.h"
#include "image_store.h"
#include "ml_inference.h"
#include "led_feedback.h"

// ─── Global state ───────────────────────────────────────────────────────────
AiCameraConfig config;

bool     cameraReady      = false;
bool     mlReady          = false;
bool     captureRequested = false;
uint32_t lastCaptureMs    = 0;

// 64-bit uptime (survives millis() overflow after ~49 days)
uint64_t uptimeMs = 0;

// ─── Setup ──────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("\n=== AI Camera starting ==="));

    // Check PSRAM
    if (psramFound()) {
        Serial.printf("PSRAM: %u bytes total, %u bytes free\n",
                      (unsigned)ESP.getPsramSize(),
                      (unsigned)ESP.getFreePsram());
    } else {
        Serial.println(F("WARNING: No PSRAM detected — reduced functionality"));
    }

    // Hardware watchdog — reboots if loop() stalls for 10 seconds
    const esp_task_wdt_config_t wdtCfg = {
        .timeout_ms = 10000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdtCfg);
    esp_task_wdt_add(NULL);

    // Onboard LED feedback
    initLedFeedback();
    setLedPattern(LedPattern::BLINK_FAST);  // Fast blink during init

    // Initialise camera
    cameraReady = initCamera();
    if (cameraReady) {
        Serial.println(F("Camera ready"));
    } else {
        Serial.println(F("Camera init failed — retrying..."));
        delay(1000);
        cameraReady = initCamera();
        if (!cameraReady) {
            Serial.println(F("Camera init failed permanently"));
        }
    }

    // Initialise image store (PSRAM ring buffer)
    initImageStore();

    // Initialise ML inference engine
    mlReady = initInference();
    if (mlReady) {
        Serial.println(F("ML inference engine ready"));
    } else {
        Serial.println(F("ML inference init failed — classification disabled"));
    }

    // Initialise button
    initButton();

    // Connect to WiFi (slow blink during connection)
    setLedPattern(LedPattern::BLINK_SLOW);
    setupWiFiManager();

    // Sync time via NTP
    setupNTP(config.timezone);

    // Start web server (also loads config from NVS)
    setupWebServer(config);

    // Ready — gentle pulse
    setLedPattern(LedPattern::PULSE);

    Serial.println(F("=== AI Camera ready ==="));
    Serial.printf("Free heap: %u, Free PSRAM: %u\n",
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned)ESP.getFreePsram());
}

// ─── Loop ───────────────────────────────────────────────────────────────────
void loop() {
    esp_task_wdt_reset();

    // 0. Track 64-bit uptime
    {
        static uint32_t lastMillis = 0;
        static uint32_t overflowCount = 0;
        uint32_t nowMs = millis();
        if (nowMs < lastMillis) overflowCount++;
        lastMillis = nowMs;
        uptimeMs = ((uint64_t)overflowCount << 32) | nowMs;
    }

    // 1. Handle web requests
    handleWebClient();

    // 2. WiFi reconnection check
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

    // 3. Check button for capture trigger
    if (buttonPressed()) {
        captureRequested = true;
        Serial.println(F("Button pressed — capture requested"));
    }

    // 4. Process capture request (capture + auto-classify)
    if (captureRequested && cameraReady) {
        captureRequested = false;
        setLedPattern(LedPattern::BLINK_FAST);

        camera_fb_t *fb = captureFrame();
        if (fb) {
            int idx = storeImage(fb->buf, fb->len, fb->width, fb->height);
            releaseFrame(fb);  // Data copied to PSRAM by storeImage; safe to release
            fb = nullptr;

            // Auto-classify if ML is ready and config enables it
            if (idx >= 0 && mlReady && config.autoClassify) {
                CapturedImage *img = getImageMutable(idx);
                if (img) {
                    img->numResults = classifyImage(
                        img->jpegData, img->jpegLength,
                        img->topResults, MAX_TOP_RESULTS);
                }
            }

            if (idx >= 0) {
                Serial.printf("Image captured: slot %d\n", idx);
                lastCaptureMs = now;
            } else {
                Serial.println(F("Image capture OK but storage failed"));
            }
        } else {
            Serial.println(F("Image capture failed"));
        }

        setLedPattern(LedPattern::FLASH_ONCE);
    }

    // 5. Update LED pattern state
    updateLed();

    // Small yield for WiFi stack
    delay(1);
}
