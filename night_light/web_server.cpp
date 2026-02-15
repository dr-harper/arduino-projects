#include "web_server.h"
#include "globals.h"
#include "html_pages.h"
#include "schedule.h"
#include "wifi_manager_setup.h"
#include "mqtt_manager.h"
#include <WebServer.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Update.h>
#include <esp_task_wdt.h>

// ─── Module state ───────────────────────────────────────────────────────────
static WebServer server(80);
static NightLightConfig *cfgPtr = nullptr;

// Session token (persisted in NVS so login survives reboots)
static char sessionToken[SESSION_TOKEN_LENGTH + 1] = "";

// Login rate limiting
static uint8_t       loginFailures = 0;
static unsigned long  lockoutUntil = 0;

// ─── Helpers ────────────────────────────────────────────────────────────────
static void loadSessionToken() {
    Preferences p;
    p.begin("nightlight", true);
    String tok = p.getString("session", "");
    p.end();
    if (tok.length() == SESSION_TOKEN_LENGTH) {
        strncpy(sessionToken, tok.c_str(), SESSION_TOKEN_LENGTH);
        sessionToken[SESSION_TOKEN_LENGTH] = '\0';
    }
}

static void generateToken() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < SESSION_TOKEN_LENGTH; i++) {
        sessionToken[i] = charset[random(0, sizeof(charset) - 1)];
    }
    sessionToken[SESSION_TOKEN_LENGTH] = '\0';

    // Persist to NVS so token survives reboots
    Preferences p;
    p.begin("nightlight", false);
    p.putString("session", sessionToken);
    p.end();
}

static bool isAuthenticated() {
    if (sessionToken[0] == '\0') return false;
    if (!server.hasHeader("Cookie")) return false;
    String cookie = server.header("Cookie");
    String expected = String("session=") + sessionToken;
    return cookie.indexOf(expected) >= 0;
}

static void redirectToLogin() {
    server.sendHeader("Location", "/login");
    server.send(302, "text/plain", "Redirecting...");
}

// Replace %CSS% placeholder with shared styles
static String injectCSS(const char *pageTemplate) {
    String page = String(FPSTR(pageTemplate));
    page.replace("%CSS%", FPSTR(SHARED_CSS));
    return page;
}

static String slotColourHex(const ScheduleSlot &s) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", s.red, s.green, s.blue);
    return String(buf);
}

static String pad2(uint8_t v) {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02u", v);
    return String(buf);
}

// ─── Route Handlers ─────────────────────────────────────────────────────────

static void handleLoginPage() {
    String page = injectCSS(LOGIN_PAGE);
    // Show error message if redirected after failed attempt
    if (server.hasArg("error")) {
        String errParam = server.arg("error");
        if (errParam == "locked") {
            page.replace("%ERROR%", "Too many attempts — try again shortly");
        } else {
            page.replace("%ERROR%", "Incorrect password");
        }
        page.replace("display:none", "display:block");
    } else {
        page.replace("%ERROR%", "");
    }
    server.send(200, "text/html", page);
}

static void handleLoginPost() {
    // Rate limiting — block login attempts during lockout
    if (millis() < lockoutUntil) {
        server.sendHeader("Location", "/login?error=locked");
        server.send(302);
        return;
    }

    if (!server.hasArg("password")) {
        server.sendHeader("Location", "/login?error=1");
        server.send(302);
        return;
    }
    String pw = server.arg("password");
    if (pw == String(cfgPtr->authPassword)) {
        loginFailures = 0;
        generateToken();
        String cookie = String("session=") + sessionToken + "; Path=/; HttpOnly; Max-Age=604800";
        server.sendHeader("Set-Cookie", cookie);
        server.sendHeader("Location", "/");
        server.send(302, "text/plain", "OK");
        Serial.println(F("Web UI: login successful"));
    } else {
        loginFailures++;
        Serial.printf("Web UI: login failed (%d/5)\n", loginFailures);
        if (loginFailures >= 5) {
            lockoutUntil = millis() + 30000;
            loginFailures = 0;
            Serial.println(F("Web UI: login locked out for 30s"));
            server.sendHeader("Location", "/login?error=locked");
        } else {
            server.sendHeader("Location", "/login?error=1");
        }
        server.send(302);
    }
}

static void handleLogout() {
    sessionToken[0] = '\0';
    Preferences p;
    p.begin("nightlight", false);
    p.remove("session");
    p.end();
    server.sendHeader("Set-Cookie", "session=; Path=/; Max-Age=0");
    server.sendHeader("Location", "/login");
    server.send(302);
}

static void handleDashboard() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    String page = injectCSS(DASHBOARD_PAGE);
    server.send(200, "text/html", page);
}

static void handleSchedulePage() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    String page = injectCSS(SCHEDULE_PAGE);
    server.send(200, "text/html", page);
}

static void handleSettingsPage() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    String page = injectCSS(SETTINGS_PAGE);
    server.send(200, "text/html", page);
}

static void handleApiStatus() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Unauthorised"); return; }

    int hour = 0, minute = 0;
    bool synced = getCurrentTime(hour, minute);

    // Uptime calculation using 64-bit counter
    uint64_t uptimeSec = uptimeMs / 1000;
    unsigned long days  = (unsigned long)(uptimeSec / 86400);
    unsigned long hours = (unsigned long)((uptimeSec % 86400) / 3600);
    unsigned long mins  = (unsigned long)((uptimeSec % 3600) / 60);

    String timeStr = getFormattedTime();
    String ipStr   = WiFi.localIP().toString();

    // Fixed-buffer JSON to reduce heap fragmentation
    // Needs ~3.5KB worst case: 8 slots × (summary + detail) + 4 special dates + metadata
    char buf[4096];
    int pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "{\"version\":\"%s\","
        "\"uptime\":\"%lud %luh %lum\","
        "\"uptimeMs\":%llu,"
        "\"freeHeap\":%lu,"
        "\"chipTemp\":%.1f,"
        "\"thermalThrottle\":%s,"
        "\"thermalDimPct\":%d,"
        "\"time\":\"%s\","
        "\"synced\":%s,"
        "\"ledsOn\":%s,"
        "\"wifiConnected\":%s,"
        "\"ip\":\"%s\","
        "\"timezone\":\"%s\","
        "\"transition\":%d,",
        FW_VERSION,
        days, hours, mins,
        (unsigned long long)uptimeMs,
        (unsigned long)ESP.getFreeHeap(),
        chipTempC,
        (thermalShutdown || thermalDimFactor < 1.0f) ? "true" : "false",
        (int)(thermalDimFactor * 100),
        timeStr.c_str(),
        synced ? "true" : "false",
        cfgPtr->ledsEnabled ? "true" : "false",
        (WiFi.status() == WL_CONNECTED) ? "true" : "false",
        ipStr.c_str(),
        cfgPtr->timezone,
        cfgPtr->transitionMinutes);

    // MQTT status
    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "\"mqttEnabled\":%s,"
        "\"mqttConnected\":%s,"
        "\"mqttHost\":\"%s\","
        "\"mqttPort\":%u,"
        "\"mqttUsername\":\"%s\",",
        cfgPtr->mqtt.enabled ? "true" : "false",
        isMqttConnected() ? "true" : "false",
        cfgPtr->mqtt.host,
        cfgPtr->mqtt.port,
        cfgPtr->mqtt.username);

    // Current state (override-aware)
    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "\"manualOverride\":%s,"
        "\"userBrightnessActive\":%s,",
        manualOverride ? "true" : "false",
        userBrightnessActive ? "true" : "false");

    if (manualOverride) {
        uint8_t effectiveBri = userBrightnessActive ? userBrightness : overrideBrightness;
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "\"slotLabel\":\"Pretty\","
            "\"slotColour\":\"#%02x%02x%02x\","
            "\"brightness\":%u,"
            "\"activeWhite\":%u,"
            "\"activeEffect\":%u,",
            overrideR, overrideG, overrideB,
            effectiveBri, overrideW, overrideEffect);
    } else if (currentSlotIndex >= 0 && currentSlotIndex < cfgPtr->numSlots) {
        const ScheduleSlot &slot = cfgPtr->slots[currentSlotIndex];
        uint8_t effectiveBri = userBrightnessActive ? userBrightness : slot.brightness;
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "\"slotLabel\":\"%s\","
            "\"slotColour\":\"#%02x%02x%02x\","
            "\"brightness\":%u,"
            "\"activeWhite\":%u,"
            "\"activeEffect\":%u,",
            slot.label, slot.red, slot.green, slot.blue,
            effectiveBri, slot.white, slot.effect);
    } else {
        uint8_t effectiveBri = userBrightnessActive ? userBrightness : DEFAULT_BRIGHTNESS;
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "\"slotLabel\":\"Default\","
            "\"slotColour\":\"#%02x%02x%02x\","
            "\"brightness\":%u,"
            "\"activeWhite\":%u,"
            "\"activeEffect\":0,",
            DEFAULT_R, DEFAULT_G, DEFAULT_B,
            effectiveBri, DEFAULT_W);
    }

    // Slot summary for dashboard
    pos += snprintf(buf + pos, sizeof(buf) - pos, "\"slots\":[");
    for (uint8_t i = 0; i < cfgPtr->numSlots; i++) {
        const ScheduleSlot &s = cfgPtr->slots[i];
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s{\"label\":\"%s\",\"start\":\"%02u:%02u\",\"end\":\"%02u:%02u\","
            "\"r\":%u,\"g\":%u,\"b\":%u,\"active\":%s}",
            i > 0 ? "," : "",
            s.label, s.startHour, s.startMinute, s.endHour, s.endMinute,
            s.red, s.green, s.blue,
            (i == currentSlotIndex) ? "true" : "false");
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "],");

    // Full slot detail for schedule editor
    pos += snprintf(buf + pos, sizeof(buf) - pos, "\"slotsDetail\":[");
    for (uint8_t i = 0; i < cfgPtr->numSlots; i++) {
        const ScheduleSlot &s = cfgPtr->slots[i];
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s{\"enabled\":%s,\"label\":\"%s\","
            "\"startHour\":%u,\"startMinute\":%u,"
            "\"endHour\":%u,\"endMinute\":%u,"
            "\"red\":%u,\"green\":%u,\"blue\":%u,\"white\":%u,"
            "\"brightness\":%u,\"effect\":%u}",
            i > 0 ? "," : "",
            s.enabled ? "true" : "false", s.label,
            s.startHour, s.startMinute,
            s.endHour, s.endMinute,
            s.red, s.green, s.blue, s.white,
            s.brightness, s.effect);
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "],");

    // Special dates
    pos += snprintf(buf + pos, sizeof(buf) - pos, "\"specialDates\":[");
    for (uint8_t i = 0; i < cfgPtr->numSpecialDates && i < MAX_SPECIAL_DATES; i++) {
        const SpecialDate &sd = cfgPtr->specialDates[i];
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s{\"enabled\":%s,\"month\":%u,\"day\":%u,"
            "\"effect\":%u,\"brightness\":%u,"
            "\"red\":%u,\"green\":%u,\"blue\":%u,\"white\":%u,"
            "\"label\":\"%s\"}",
            i > 0 ? "," : "",
            sd.enabled ? "true" : "false", sd.month, sd.day,
            sd.effect, sd.brightness,
            sd.red, sd.green, sd.blue, sd.white,
            sd.label);
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "]}");

    server.send(200, "application/json", buf);
}

static void initOverrideFromSlot() {
    if (currentSlotIndex >= 0) {
        overrideR = cfgPtr->slots[currentSlotIndex].red;
        overrideG = cfgPtr->slots[currentSlotIndex].green;
        overrideB = cfgPtr->slots[currentSlotIndex].blue;
        overrideW = cfgPtr->slots[currentSlotIndex].white;
        overrideBrightness = cfgPtr->slots[currentSlotIndex].brightness;
        overrideEffect = cfgPtr->slots[currentSlotIndex].effect;
    } else {
        overrideR = FALLBACK_R;
        overrideG = FALLBACK_G;
        overrideB = FALLBACK_B;
        overrideW = FALLBACK_W;
        overrideBrightness = FALLBACK_BRIGHTNESS;
        overrideEffect = EFFECT_SOLID;
    }
}

static void handleApiBrightness() {
    if (!isAuthenticated()) { server.send(401); return; }

    if (server.hasArg("brightness")) {
        // Brightness is independent — does not change the active mode
        userBrightness = constrain(server.arg("brightness").toInt(), 0, 255);
        userBrightnessActive = true;
        mqttPublishLightState();
    }
    server.send(200, "text/plain", "OK");
}

static void handleApiColour() {
    if (!isAuthenticated()) { server.send(401); return; }

    // Seed from slot only when first entering override mode
    if (!manualOverride) initOverrideFromSlot();

    bool changed = false;
    if (server.hasArg("r")) { overrideR = constrain(server.arg("r").toInt(), 0, 255); changed = true; }
    if (server.hasArg("g")) { overrideG = constrain(server.arg("g").toInt(), 0, 255); changed = true; }
    if (server.hasArg("b")) { overrideB = constrain(server.arg("b").toInt(), 0, 255); changed = true; }
    if (server.hasArg("w")) { overrideW = constrain(server.arg("w").toInt(), 0, 255); changed = true; }

    if (changed) {
        manualOverride = true;
        mqttPublishLightState();
    }
    server.send(200, "text/plain", "OK");
}

static void handleApiEffect() {
    if (!isAuthenticated()) { server.send(401); return; }

    if (server.hasArg("effect")) {
        int eff = server.arg("effect").toInt();
        if (eff < 0) {
            // Return to schedule-driven mode
            manualOverride = false;
        } else {
            // Seed from slot only when first entering override mode
            if (!manualOverride) initOverrideFromSlot();

            overrideEffect = constrain(eff, 0, EFFECT_COUNT - 1);
            manualOverride = true;
        }
        mqttPublishLightState();
    }
    server.send(200, "text/plain", "OK");
}

static void handleApiSettings() {
    if (!isAuthenticated()) { server.send(401); return; }

    bool changed = false;
    bool tzChanged = false;

    if (server.hasArg("timezone")) {
        String tz = server.arg("timezone");
        if (tz.length() > 0 && tz.length() < sizeof(cfgPtr->timezone)) {
            strncpy(cfgPtr->timezone, tz.c_str(), sizeof(cfgPtr->timezone) - 1);
            cfgPtr->timezone[sizeof(cfgPtr->timezone) - 1] = '\0';
            changed = true;
            tzChanged = true;
        }
    }
    if (server.hasArg("transition")) {
        cfgPtr->transitionMinutes = constrain(server.arg("transition").toInt(), 0, 30);
        changed = true;
    }
    if (server.hasArg("password")) {
        String pw = server.arg("password");
        if (pw.length() > 0 && pw.length() < sizeof(cfgPtr->authPassword)) {
            strncpy(cfgPtr->authPassword, pw.c_str(), sizeof(cfgPtr->authPassword) - 1);
            cfgPtr->authPassword[sizeof(cfgPtr->authPassword) - 1] = '\0';
            changed = true;
        }
    }
    if (server.hasArg("ledsOn")) {
        cfgPtr->ledsEnabled = (server.arg("ledsOn") == "1");
        changed = true;
    }

    if (changed) {
        saveSchedule(*cfgPtr);
        if (tzChanged) {
            reconfigureNTP(cfgPtr->timezone);
        }
        mqttPublishLightState();
    }

    server.send(200, "text/plain", "OK");
}

// ─── MQTT Settings Endpoint ────────────────────────────────────────────────

static void handleApiMqtt() {
    if (!isAuthenticated()) { server.send(401); return; }

    String body = server.arg("plain");
    if (body.length() == 0) {
        server.send(400, "text/plain", "No data");
        return;
    }

    // Parse MQTT settings from JSON body
    // Expected: {"enabled":true,"host":"192.168.1.x","port":1883,"username":"","password":""}
    cfgPtr->mqtt.enabled = (body.indexOf("\"enabled\":true") >= 0);

    // Extract host
    int hostPos = body.indexOf("\"host\":\"");
    if (hostPos >= 0) {
        hostPos += 8;
        int hostEnd = body.indexOf('"', hostPos);
        if (hostEnd > hostPos) {
            String h = body.substring(hostPos, hostEnd);
            strncpy(cfgPtr->mqtt.host, h.c_str(), sizeof(cfgPtr->mqtt.host) - 1);
            cfgPtr->mqtt.host[sizeof(cfgPtr->mqtt.host) - 1] = '\0';
        }
    }

    // Extract port
    int portPos = body.indexOf("\"port\":");
    if (portPos >= 0) {
        portPos += 7;
        cfgPtr->mqtt.port = (uint16_t)body.substring(portPos).toInt();
        if (cfgPtr->mqtt.port == 0) cfgPtr->mqtt.port = MQTT_DEFAULT_PORT;
    }

    // Extract username
    int userPos = body.indexOf("\"username\":\"");
    if (userPos >= 0) {
        userPos += 12;
        int userEnd = body.indexOf('"', userPos);
        if (userEnd >= userPos) {
            String u = body.substring(userPos, userEnd);
            strncpy(cfgPtr->mqtt.username, u.c_str(), sizeof(cfgPtr->mqtt.username) - 1);
            cfgPtr->mqtt.username[sizeof(cfgPtr->mqtt.username) - 1] = '\0';
        }
    }

    // Extract password
    int passPos = body.indexOf("\"password\":\"");
    if (passPos >= 0) {
        passPos += 12;
        int passEnd = body.indexOf('"', passPos);
        if (passEnd >= passPos) {
            String p = body.substring(passPos, passEnd);
            strncpy(cfgPtr->mqtt.password, p.c_str(), sizeof(cfgPtr->mqtt.password) - 1);
            cfgPtr->mqtt.password[sizeof(cfgPtr->mqtt.password) - 1] = '\0';
        }
    }

    saveSchedule(*cfgPtr);

    // Disconnect and reconnect with new settings
    mqttDisconnect();
    // loopMqtt() will handle reconnection on next iteration

    Serial.println(F("MQTT settings saved via web UI"));
    server.send(200, "text/plain", "OK");
}

static void handleApiSpecialDates() {
    if (!isAuthenticated()) { server.send(401); return; }

    String body = server.arg("plain");
    if (body.length() == 0) {
        server.send(400, "text/plain", "No data");
        return;
    }

    // Parse array of special date objects
    // Format: {"dates":[{...},{...},...]}
    int numParsed = 0;

    int arrStart = body.indexOf('[');
    if (arrStart < 0) {
        server.send(400, "text/plain", "Invalid format");
        return;
    }

    int searchFrom = arrStart + 1;
    while (numParsed < MAX_SPECIAL_DATES) {
        int objStart = body.indexOf('{', searchFrom);
        if (objStart < 0) break;
        int objEnd = body.indexOf('}', objStart);
        if (objEnd < 0) break;

        String obj = body.substring(objStart, objEnd + 1);
        SpecialDate &sd = cfgPtr->specialDates[numParsed];
        memset(&sd, 0, sizeof(SpecialDate));

        sd.enabled = (obj.indexOf("\"enabled\":true") >= 0);

        auto extractInt = [&](const char *key) -> int {
            int pos = obj.indexOf(key);
            if (pos < 0) return 0;
            pos = obj.indexOf(':', pos) + 1;
            return obj.substring(pos).toInt();
        };

        sd.month      = constrain(extractInt("\"month\""), 1, 12);
        sd.day        = constrain(extractInt("\"day\""), 1, 31);
        sd.effect     = constrain(extractInt("\"effect\""), 0, EFFECT_COUNT - 1);
        sd.brightness = constrain(extractInt("\"brightness\""), 0, 255);
        sd.red        = constrain(extractInt("\"red\""), 0, 255);
        sd.green      = constrain(extractInt("\"green\""), 0, 255);
        sd.blue       = constrain(extractInt("\"blue\""), 0, 255);
        sd.white      = constrain(extractInt("\"white\""), 0, 255);

        int lblPos = obj.indexOf("\"label\":\"");
        if (lblPos >= 0) {
            lblPos += 9;
            int lblEnd = obj.indexOf('"', lblPos);
            if (lblEnd > lblPos) {
                String lbl = obj.substring(lblPos, lblEnd);
                strncpy(sd.label, lbl.c_str(), sizeof(sd.label) - 1);
                sd.label[sizeof(sd.label) - 1] = '\0';
            }
        }

        numParsed++;
        searchFrom = objEnd + 1;
    }

    cfgPtr->numSpecialDates = numParsed;
    saveSchedule(*cfgPtr);

    Serial.print(F("Special dates saved via web: "));
    Serial.print(numParsed);
    Serial.println(F(" dates"));

    server.send(200, "text/plain", "OK");
}

static void handleScheduleSave() {
    if (!isAuthenticated()) { server.send(401); return; }

    String body = server.arg("plain");
    if (body.length() == 0) {
        server.send(400, "text/plain", "No data");
        return;
    }

    // Minimal JSON parsing for the schedule slots array.
    // Format: {"slots":[{...},{...},...]}
    // We parse field by field to avoid pulling in a full JSON library.
    int numParsed = 0;

    int slotsStart = body.indexOf("\"slots\"");
    if (slotsStart < 0) {
        server.send(400, "text/plain", "Missing slots");
        return;
    }

    int arrStart = body.indexOf('[', slotsStart);
    if (arrStart < 0) {
        server.send(400, "text/plain", "Invalid format");
        return;
    }

    int searchFrom = arrStart + 1;
    while (numParsed < MAX_SCHEDULE_SLOTS) {
        int objStart = body.indexOf('{', searchFrom);
        if (objStart < 0) break;
        int objEnd = body.indexOf('}', objStart);
        if (objEnd < 0) break;

        String obj = body.substring(objStart, objEnd + 1);
        ScheduleSlot &slot = cfgPtr->slots[numParsed];
        memset(&slot, 0, sizeof(ScheduleSlot));

        // Parse each field
        slot.enabled = (obj.indexOf("\"enabled\":true") >= 0);

        // Extract numeric fields
        auto extractInt = [&](const char *key) -> int {
            int pos = obj.indexOf(key);
            if (pos < 0) return 0;
            pos = obj.indexOf(':', pos) + 1;
            return obj.substring(pos).toInt();
        };

        slot.startHour   = constrain(extractInt("\"startHour\""), 0, 23);
        slot.startMinute = constrain(extractInt("\"startMinute\""), 0, 59);
        slot.endHour     = constrain(extractInt("\"endHour\""), 0, 23);
        slot.endMinute   = constrain(extractInt("\"endMinute\""), 0, 59);
        slot.red         = constrain(extractInt("\"red\""), 0, 255);
        slot.green       = constrain(extractInt("\"green\""), 0, 255);
        slot.blue        = constrain(extractInt("\"blue\""), 0, 255);
        slot.white       = constrain(extractInt("\"white\""), 0, 255);
        slot.brightness  = constrain(extractInt("\"brightness\""), 0, 255);
        slot.effect      = constrain(extractInt("\"effect\""), 0, EFFECT_COUNT - 1);

        // Extract label string
        int lblPos = obj.indexOf("\"label\":\"");
        if (lblPos >= 0) {
            lblPos += 9;  // skip past "label":"
            int lblEnd = obj.indexOf('"', lblPos);
            if (lblEnd > lblPos) {
                String lbl = obj.substring(lblPos, lblEnd);
                strncpy(slot.label, lbl.c_str(), sizeof(slot.label) - 1);
                slot.label[sizeof(slot.label) - 1] = '\0';
            }
        }

        numParsed++;
        searchFrom = objEnd + 1;
    }

    cfgPtr->numSlots = numParsed;
    saveSchedule(*cfgPtr);

    // Clear manual override so the new schedule takes effect immediately
    manualOverride = false;

    // Notify MQTT that schedule has changed (re-publish discovery + state)
    mqttPublishScheduleChanged();

    Serial.print(F("Schedule saved via web: "));
    Serial.print(numParsed);
    Serial.println(F(" slots"));

    server.send(200, "text/plain", "OK");
}

static void handleApiReset() {
    if (!isAuthenticated()) { server.send(401); return; }

    resetScheduleDefaults(*cfgPtr);
    saveSchedule(*cfgPtr);
    manualOverride = false;
    userBrightnessActive = false;

    // Reconfigure NTP with default timezone
    reconfigureNTP(cfgPtr->timezone);

    // Notify MQTT that schedule has changed
    mqttPublishScheduleChanged();

    Serial.println(F("Factory reset via web UI"));
    server.send(200, "text/plain", "OK");
}

// ─── Firmware Update ────────────────────────────────────────────────────────

static void handleUpdatePage() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    String page = injectCSS(UPDATE_PAGE);
    server.send(200, "text/html", page);
}

static void handleUpdateUpload() {
    if (!isAuthenticated()) { server.send(401); return; }

    HTTPUpload &upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        // Disable watchdog during firmware upload — flash writes stall the
        // loop and would otherwise trigger a panic reboot mid-update
        esp_task_wdt_delete(NULL);
        Serial.print(F("OTA web upload: "));
        Serial.println(upload.filename);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.print(F("OTA web upload complete: "));
            Serial.print(upload.totalSize);
            Serial.println(F(" bytes"));
        } else {
            // Re-enable watchdog after failed upload
            esp_task_wdt_add(NULL);
            Update.printError(Serial);
        }
    }
}

static void handleUpdateResult() {
    if (!isAuthenticated()) { server.send(401); return; }

    if (Update.hasError()) {
        server.send(500, "text/plain", "Update failed");
    } else {
        server.send(200, "text/plain", "OK");
        Serial.println(F("Rebooting after OTA web update..."));
        delay(500);
        ESP.restart();
    }
}

static void handleNotFound() {
    server.sendHeader("Location", "/");
    server.send(302);
}

// ─── Public API ─────────────────────────────────────────────────────────────

void setupWebServer(NightLightConfig &cfg, Adafruit_NeoPixel &strip) {
    cfgPtr = &cfg;

    // Restore session token from NVS (keeps login alive across reboots)
    loadSessionToken();

    // Collect cookies for authentication
    const char *headerKeys[] = {"Cookie"};
    server.collectHeaders(headerKeys, 1);

    // Routes
    server.on("/login",   HTTP_GET,  handleLoginPage);
    server.on("/login",   HTTP_POST, handleLoginPost);
    server.on("/logout",  HTTP_GET,  handleLogout);
    server.on("/",        HTTP_GET,  handleDashboard);
    server.on("/schedule", HTTP_GET, handleSchedulePage);
    server.on("/settings", HTTP_GET, handleSettingsPage);
    server.on("/update",   HTTP_GET, handleUpdatePage);
    server.on("/update",   HTTP_POST, handleUpdateResult, handleUpdateUpload);

    // API
    server.on("/api/status",     HTTP_GET,  handleApiStatus);
    server.on("/api/brightness", HTTP_POST, handleApiBrightness);
    server.on("/api/colour",     HTTP_POST, handleApiColour);
    server.on("/api/effect",     HTTP_POST, handleApiEffect);
    server.on("/api/settings",   HTTP_POST, handleApiSettings);
    server.on("/api/reset",      HTTP_POST, handleApiReset);
    server.on("/api/mqtt",            HTTP_POST, handleApiMqtt);
    server.on("/api/special-dates",  HTTP_POST, handleApiSpecialDates);
    server.on("/schedule/save",      HTTP_POST, handleScheduleSave);

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.print(F("Web server started at http://"));
    Serial.println(WiFi.localIP());
}

void handleWebClient() {
    server.handleClient();
}
