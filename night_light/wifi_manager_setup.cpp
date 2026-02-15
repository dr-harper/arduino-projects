#include "wifi_manager_setup.h"
#include "config.h"
#include "globals.h"
#include "html_pages.h"
#include "led_effects.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <time.h>

static bool ntpSynced = false;

// ─── AP mode scanner animation ──────────────────────────────────────────────
// Dim warm amber base with a red scanner LED travelling along the strip.
static void scannerStrip(Adafruit_NeoPixel &strip, unsigned long now) {
    int numPx = strip.numPixels();

    // Scanner travels the full strip length in ~2.5 seconds
    const unsigned long cycleMs = 2500;
    float pos = (float)(now % cycleMs) / (float)cycleMs * (float)numPx;

    strip.setBrightness(255);  // brightness handled per-pixel

    for (int i = 0; i < numPx; i++) {
        // Distance from the scanner head
        float dist = pos - (float)i;
        if (dist < 0.0f) dist += (float)numPx;  // wrap around

        if (dist < 1.0f) {
            // Scanner head — bright red
            strip.setPixelColor(i, strip.Color(255, 0, 0, 0));
        } else if (dist < 6.0f) {
            // Fade trail behind the scanner (5 LEDs)
            float fade = 1.0f - (dist - 1.0f) / 5.0f;
            uint8_t r = (uint8_t)(255.0f * fade * fade);
            uint8_t g = (uint8_t)(20.0f * fade);
            strip.setPixelColor(i, strip.Color(r, g, 0, 0));
        } else {
            // Base: dim warm amber
            strip.setPixelColor(i, strip.Color(18, 10, 2, 0));
        }
    }
    strip.show();
}

// ─── AP controls server helpers ─────────────────────────────────────────────

// Replace %CSS% placeholder with shared styles (local copy for AP context)
static String injectCssAp(const char *pageTemplate) {
    String page = String(FPSTR(pageTemplate));
    page.replace("%CSS%", FPSTR(SHARED_CSS));
    return page;
}

// ─── WiFi Manager ───────────────────────────────────────────────────────────
void setupWiFiManager(Adafruit_NeoPixel &strip) {
    // Set WiFi hostname BEFORE connecting (shows on router/DHCP)
    WiFi.setHostname(MDNS_HOSTNAME);

    WiFiManager wifiManager;

    // Reset saved credentials for testing (uncomment if needed):
    // wifiManager.resetSettings();

    // No timeout — stay in AP mode indefinitely until WiFi is configured
    wifiManager.setConfigPortalTimeout(0);

    // Dark theme for the captive portal
    wifiManager.setDarkMode(true);

    // Add link to light controls on port 8080
    wifiManager.setCustomMenuHTML(
        "<br><a href='http://192.168.4.1:8080' "
        "style='display:block;text-align:center;padding:10px;background:#7b68ee;"
        "color:#fff;border-radius:8px;margin:8px;text-decoration:none'>"
        "&#127769; Light Controls</a>"
    );

    // Non-blocking so we can animate LEDs while waiting
    wifiManager.setConfigPortalBlocking(false);

    Serial.println(F("Starting WiFiManager..."));
    Serial.print(F("AP: "));
    Serial.println(F(AP_NAME));

    // autoConnect returns immediately in non-blocking mode if credentials
    // aren't stored, or after a quick connection attempt if they are.
    if (wifiManager.autoConnect(AP_NAME, AP_PASS)) {
        Serial.println(F("WiFi connected (stored credentials)"));
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP());
        if (MDNS.begin(MDNS_HOSTNAME)) {
            MDNS.addService("http", "tcp", 80);
            Serial.printf("mDNS: http://%s.local\n", MDNS_HOSTNAME);
        }
        return;
    }

    // ── Not yet connected — run config portal with controls server ──────

    Serial.println(F("AP mode — starting controls server on :8080"));

    WebServer ctrlServer(8080);

    // Serve the controls page (no authentication)
    ctrlServer.on("/", HTTP_GET, [&]() {
        String page = injectCssAp(AP_CONTROLS_PAGE);
        ctrlServer.send(200, "text/html", page);
    });

    // Brightness
    ctrlServer.on("/api/brightness", HTTP_POST, [&]() {
        if (ctrlServer.hasArg("brightness")) {
            userBrightness = constrain(ctrlServer.arg("brightness").toInt(), 0, 255);
            userBrightnessActive = true;
        }
        ctrlServer.send(200, "text/plain", "OK");
    });

    // Colour — enters pretty mode (preserves current effect)
    ctrlServer.on("/api/colour", HTTP_POST, [&]() {
        bool changed = false;
        if (ctrlServer.hasArg("r")) { overrideR = constrain(ctrlServer.arg("r").toInt(), 0, 255); changed = true; }
        if (ctrlServer.hasArg("g")) { overrideG = constrain(ctrlServer.arg("g").toInt(), 0, 255); changed = true; }
        if (ctrlServer.hasArg("b")) { overrideB = constrain(ctrlServer.arg("b").toInt(), 0, 255); changed = true; }
        if (ctrlServer.hasArg("w")) { overrideW = constrain(ctrlServer.arg("w").toInt(), 0, 255); changed = true; }
        if (changed) {
            if (!manualOverride) {
                overrideEffect = EFFECT_SOLID;
                overrideBrightness = AP_DEFAULT_BRIGHTNESS;
            }
            manualOverride = true;
        }
        ctrlServer.send(200, "text/plain", "OK");
    });

    // Effect override
    ctrlServer.on("/api/effect", HTTP_POST, [&]() {
        if (ctrlServer.hasArg("effect")) {
            int eff = ctrlServer.arg("effect").toInt();
            overrideEffect = constrain(eff, 0, EFFECT_COUNT - 1);
            if (!manualOverride) {
                overrideBrightness = AP_DEFAULT_BRIGHTNESS;
            }
            manualOverride = true;
        }
        ctrlServer.send(200, "text/plain", "OK");
    });

    // Minimal status
    ctrlServer.on("/api/status", HTTP_GET, [&]() {
        uint8_t bri = userBrightnessActive ? userBrightness : overrideBrightness;
        char hexBuf[8];
        snprintf(hexBuf, sizeof(hexBuf), "#%02x%02x%02x", overrideR, overrideG, overrideB);
        String json = "{";
        json += "\"brightness\":" + String(bri) + ",";
        json += "\"colour\":\"" + String(hexBuf) + "\",";
        json += "\"white\":" + String(overrideW) + ",";
        json += "\"effect\":" + String(overrideEffect) + ",";
        json += "\"manualOverride\":" + String(manualOverride ? "true" : "false");
        json += "}";
        ctrlServer.send(200, "application/json", json);
    });

    ctrlServer.begin();
    Serial.println(F("AP controls server ready on port 8080"));

    // ── Main AP loop — process both servers, render LEDs ────────────────

    unsigned long lastLedMs  = 0;
    unsigned long lastTempMs = 0;

    while (WiFi.status() != WL_CONNECTED) {
        wifiManager.process();
        ctrlServer.handleClient();

        unsigned long now = millis();

        // Thermal protection — read chip temperature periodically (with hysteresis)
        if (now - lastTempMs >= TEMP_READ_INTERVAL_MS) {
            lastTempMs = now;
            chipTempC = temperatureRead();

            if (chipTempC >= THERMAL_CRIT_C) {
                thermalDimFactor = 0.0f;
                thermalShutdown  = true;
            } else if (thermalShutdown && chipTempC >= THERMAL_WARN_CLEAR_C) {
                // Stay in shutdown until cooled below clear threshold
                thermalDimFactor = 0.0f;
            } else if (chipTempC >= THERMAL_WARN_C) {
                thermalDimFactor = 1.0f - (chipTempC - THERMAL_WARN_C) / (THERMAL_CRIT_C - THERMAL_WARN_C);
                thermalShutdown  = false;
            } else {
                thermalDimFactor = 1.0f;
                thermalShutdown  = false;
            }
        }

        if (now - lastLedMs >= LED_UPDATE_INTERVAL) {
            lastLedMs = now;

            if (thermalShutdown) {
                strip.clear();
                strip.show();
            } else if (manualOverride) {
                uint8_t bri = userBrightnessActive ? userBrightness : overrideBrightness;
                applyEffect(strip, (EffectType)overrideEffect,
                            overrideR, overrideG, overrideB, overrideW, bri, now);
            } else {
                scannerStrip(strip, now);
            }
        }

        delay(2);  // yield for WiFi stack
    }

    // WiFi connected — clean up AP state
    ctrlServer.stop();
    manualOverride      = false;
    userBrightnessActive = false;

    Serial.println(F("WiFi connected — AP controls server stopped"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());

    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("mDNS: http://%s.local\n", MDNS_HOSTNAME);
    }
}

// ─── NTP Time Sync ──────────────────────────────────────────────────────────
void setupNTP(const char *tz) {
    // Use POSIX TZ string for correct automatic DST transitions
    configTzTime(tz, NTP_SERVER_1, NTP_SERVER_2);

    Serial.print(F("NTP sync (TZ: "));
    Serial.print(tz);
    Serial.println(F(")..."));

    // Quick check — don't block startup waiting for NTP.
    // The main loop already shows a warm fallback colour until time syncs,
    // and configTzTime() continues syncing in the background.
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 1500)) {  // 1.5s max wait
        ntpSynced = true;
        Serial.print(F("Time synced: "));
        Serial.println(&timeinfo, "%H:%M:%S %d/%m/%Y");
    } else {
        Serial.println(F("NTP not yet synced — will sync in background"));
    }
}

void reconfigureNTP(const char *tz) {
    configTzTime(tz, NTP_SERVER_1, NTP_SERVER_2);
    Serial.println(F("NTP reconfigured with new timezone"));
}

bool getCurrentTime(int &hour, int &minute) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }
    ntpSynced = true;
    hour   = timeinfo.tm_hour;
    minute = timeinfo.tm_min;
    return true;
}

String getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return String(F("\xe2\x80\x94"));  // em-dash
    }
    ntpSynced = true;
    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(buf);
}

bool isTimeSynced() {
    return ntpSynced;
}

bool getCurrentDate(int &year, int &month, int &day) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }
    ntpSynced = true;
    year  = timeinfo.tm_year + 1900;
    month = timeinfo.tm_mon + 1;   // tm_mon is 0-based
    day   = timeinfo.tm_mday;
    return true;
}
