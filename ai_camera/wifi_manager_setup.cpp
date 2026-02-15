#include "wifi_manager_setup.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <time.h>

static bool ntpSynced = false;

// ─── WiFi Manager ───────────────────────────────────────────────────────────
void setupWiFiManager() {
    WiFi.setHostname(MDNS_HOSTNAME);

    WiFiManager wifiManager;

    // No timeout — stay in AP mode until WiFi is configured
    wifiManager.setConfigPortalTimeout(0);
    wifiManager.setDarkMode(true);

    Serial.println(F("Starting WiFiManager..."));
    Serial.print(F("AP: "));
    Serial.println(F(AP_NAME));

    // Blocking connect — waits until WiFi is configured
    if (!wifiManager.autoConnect(AP_NAME, AP_PASS)) {
        Serial.println(F("WiFi connection failed — restarting"));
        delay(1000);
        ESP.restart();
    }

    Serial.println(F("WiFi connected"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());

    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("mDNS: http://%s.local\n", MDNS_HOSTNAME);
    }
}

// ─── NTP Time Sync ──────────────────────────────────────────────────────────
void setupNTP(const char *tz) {
    configTzTime(tz, NTP_SERVER_1, NTP_SERVER_2);

    Serial.print(F("NTP sync (TZ: "));
    Serial.print(tz);
    Serial.println(F(")..."));

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 1500)) {
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
        return String(F("--:--:--"));
    }
    ntpSynced = true;
    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(buf);
}

bool getCurrentDate(int &year, int &month, int &day) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }
    ntpSynced = true;
    year  = timeinfo.tm_year + 1900;
    month = timeinfo.tm_mon + 1;
    day   = timeinfo.tm_mday;
    return true;
}

bool isTimeSynced() {
    return ntpSynced;
}

String getIsoTimestamp() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return String();
    }
    ntpSynced = true;
    char buf[24];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
    return String(buf);
}
