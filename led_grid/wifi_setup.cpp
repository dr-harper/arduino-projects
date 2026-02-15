#include "wifi_setup.h"
#include "led_effects.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <time.h>

// Diagonal rainbow chase while waiting for WiFi config
static void apAnimation(Adafruit_NeoPixel &strip) {
    static uint8_t frame = 0;
    frame++;
    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            uint8_t diag = x + y;
            uint8_t hue = frame * 3 + diag * 8;
            // Simple HSV-ish colour wheel
            uint8_t r, g, b;
            uint8_t h = hue;
            if (h < 85) {
                r = (255 - h * 3); g = 0; b = (h * 3);
            } else if (h < 170) {
                h -= 85;
                r = 0; g = (h * 3); b = (255 - h * 3);
            } else {
                h -= 170;
                r = (h * 3); g = (255 - h * 3); b = 0;
            }
            // Dim it down (we're in setup mode, be gentle)
            r >>= 2; g >>= 2; b >>= 2;
            strip.setPixelColor(xyToIndex(x, y),
                Adafruit_NeoPixel::Color(r, g, b));
        }
    }
    strip.show();
}

void setupWiFi(Adafruit_NeoPixel &strip) {
    WiFi.setHostname(MDNS_HOSTNAME);

    WiFiManager wm;
    wm.setConfigPortalTimeout(0);   // No timeout — wait forever
    wm.setDarkMode(true);
    wm.setConfigPortalBlocking(false);

    bool connected = wm.autoConnect(AP_NAME, AP_PASS);

    if (!connected) {
        Serial.println(F("WiFi: Entering AP config mode..."));
        Serial.printf("Connect to '%s' (pass: '%s') to configure\n",
                      AP_NAME, AP_PASS);

        // Animate while waiting for config
        while (WiFi.status() != WL_CONNECTED) {
            wm.process();
            apAnimation(strip);
            delay(30);
        }
    }

    Serial.printf("WiFi: Connected! IP: %s\n",
                  WiFi.localIP().toString().c_str());

    // NTP time sync (UK timezone: GMT+0 with BST daylight saving)
    configTzTime("GMT0BST,M3.5.0/1,M10.5.0", "pool.ntp.org", "time.google.com");
    Serial.println(F("NTP: Time sync configured"));

    // mDNS
    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("mDNS: http://%s.local\n", MDNS_HOSTNAME);
    } else {
        Serial.println(F("mDNS: Failed to start"));
    }
}
