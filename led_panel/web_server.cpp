#include "web_server.h"
#include "html_pages_gz.h"
#include "persistence.h"
#include "tetris_effect.h"
#include "snake_game.h"
#include "led_effects.h"
#include "websocket_handler.h"
#include <WebServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Update.h>
#include <Adafruit_NeoPixel.h>

static WebServer server(80);
static GridConfig *cfgPtr = nullptr;

// ─── Session Auth ──────────────────────────────────────────────────────────

#define SESSION_TOKEN_LEN 16
static char sessionToken[SESSION_TOKEN_LEN + 1] = {0};
static uint8_t loginFailures = 0;
static unsigned long lockoutStartMs = 0;
#define MAX_LOGIN_ATTEMPTS 5
#define LOCKOUT_MS 30000

static void generateToken() {
    const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < SESSION_TOKEN_LEN; i++) {
        sessionToken[i] = chars[esp_random() % (sizeof(chars) - 1)];
    }
    sessionToken[SESSION_TOKEN_LEN] = '\0';
    Preferences p;
    p.begin("ledgrid", false);
    p.putString("session", sessionToken);
    p.end();
    setWebSocketAuthToken(sessionToken);
}

static void loadSession() {
    Preferences p;
    p.begin("ledgrid", true);
    String tok = p.getString("session", "");
    p.end();
    if (tok.length() == SESSION_TOKEN_LEN) {
        strncpy(sessionToken, tok.c_str(), SESSION_TOKEN_LEN);
        sessionToken[SESSION_TOKEN_LEN] = '\0';
        setWebSocketAuthToken(sessionToken);
    } else {
        generateToken();
    }
}

static bool isAuthenticated() {
    if (!server.hasHeader("Cookie")) return false;
    String cookie = server.header("Cookie");
    String expected = String("session=") + sessionToken;
    int idx = cookie.indexOf(expected);
    if (idx < 0) return false;
    // Ensure exact match (bounded by ; or end of string)
    int endIdx = idx + expected.length();
    if (endIdx < (int)cookie.length() && cookie[endIdx] != ';' && cookie[endIdx] != ' ') {
        return false;
    }
    return true;
}

static void redirectLogin() {
    server.sendHeader("Location", "/login");
    server.send(302, "text/plain", "Redirecting...");
}

// ─── Gzip Page Helper ─────────────────────────────────────────────────────

static void sendGzPage(const uint8_t *data, size_t len) {
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "text/html", (const char *)data, len);
}

// ─── Route Handlers ────────────────────────────────────────────────────────

static void handleLoginPage() {
    sendGzPage(LOGIN_PAGE_GZ, LOGIN_PAGE_GZ_LEN);
}

static void handleLoginPost() {
    // Rate limiting (overflow-safe)
    if (loginFailures >= MAX_LOGIN_ATTEMPTS) {
        unsigned long elapsed = millis() - lockoutStartMs;
        if (elapsed < LOCKOUT_MS) {
            server.send(429, "text/html",
                "<html><body><h2>Locked out. Try again in 30 seconds.</h2></body></html>");
            return;
        }
        loginFailures = 0;
    }

    String pw = server.arg("password");
    if (pw == cfgPtr->authPassword) {
        loginFailures = 0;
        generateToken();
        String cookie = String("session=") + sessionToken +
            "; Path=/; HttpOnly; SameSite=Strict; Max-Age=604800";
        server.sendHeader("Set-Cookie", cookie);
        server.sendHeader("Location", "/");
        server.send(302, "text/plain", "OK");
    } else {
        loginFailures++;
        if (loginFailures >= MAX_LOGIN_ATTEMPTS) {
            lockoutStartMs = millis();
        }
        server.sendHeader("Location", "/login?error=1");
        server.send(302, "text/plain", "Bad password");
    }
}

static void handleLogout() {
    server.sendHeader("Set-Cookie",
        "session=; Path=/; HttpOnly; Max-Age=0");
    server.sendHeader("Location", "/login");
    server.send(302, "text/plain", "Logged out");
}

static void handleDashboard() {
    if (!isAuthenticated()) { redirectLogin(); return; }
    sendGzPage(DASHBOARD_PAGE_GZ, DASHBOARD_PAGE_GZ_LEN);
}

static void handleSettings() {
    if (!isAuthenticated()) { redirectLogin(); return; }
    sendGzPage(SETTINGS_PAGE_GZ, SETTINGS_PAGE_GZ_LEN);
}

static void handleTetrisPage() {
    if (!isAuthenticated()) { redirectLogin(); return; }
    sendGzPage(TETRIS_PAGE_GZ, TETRIS_PAGE_GZ_LEN);
}

// ─── API Handlers ──────────────────────────────────────────────────────────

static void handleApiStatus() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Auth required"); return; }

    unsigned long up = millis() / 1000;
    unsigned long days = up / 86400;
    unsigned long hours = (up % 86400) / 3600;
    unsigned long mins = (up % 3600) / 60;

    char buf[512];
    int written = snprintf(buf, sizeof(buf),
        "{\"version\":\"%s\","
        "\"uptime\":\"%lud %luh %lum\","
        "\"freeHeap\":%lu,"
        "\"effect\":%d,"
        "\"brightness\":%d,"
        "\"manualMode\":%s,"
        "\"bgR\":%d,\"bgG\":%d,\"bgB\":%d,"
        "\"dropStartMs\":%d,"
        "\"dropMinMs\":%d,"
        "\"moveIntervalMs\":%d,"
        "\"rotIntervalMs\":%d,"
        "\"aiSkillPct\":%d,"
        "\"jitterPct\":%d,"
        "\"ssid\":\"%s\","
        "\"ip\":\"%s\"}",
        FW_VERSION,
        days, hours, mins,
        (unsigned long)ESP.getFreeHeap(),
        (int)cfgPtr->currentEffect,
        cfgPtr->brightness,
        cfgPtr->manualMode ? "true" : "false",
        cfgPtr->bgR, cfgPtr->bgG, cfgPtr->bgB,
        cfgPtr->dropStartMs,
        cfgPtr->dropMinMs,
        cfgPtr->moveIntervalMs,
        cfgPtr->rotIntervalMs,
        cfgPtr->aiSkillPct,
        cfgPtr->jitterPct,
        WiFi.SSID().c_str(),
        WiFi.localIP().toString().c_str());

    if (written >= (int)sizeof(buf)) {
        server.send(500, "text/plain", "Response too large");
        return;
    }

    server.send(200, "application/json", buf);
}

static void handleApiBrightness() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Auth required"); return; }
    int val = server.arg("value").toInt();
    cfgPtr->brightness = constrain(val, 0, 255);
    // Brightness is applied in the main loop via strip.setBrightness()
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleApiEffect() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Auth required"); return; }
    int val = server.arg("value").toInt();
    if (val >= 0 && val < EFFECT_COUNT) {
        cfgPtr->currentEffect = (Effect)val;
        setWsActiveEffect(cfgPtr->currentEffect);
        if (cfgPtr->currentEffect == EFFECT_TETRIS) {
            resetTetris();
        } else if (cfgPtr->currentEffect == EFFECT_SNAKE) {
            resetSnake();
        }
    }
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleApiTuning() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Auth required"); return; }

    if (server.hasArg("dropStartMs"))
        cfgPtr->dropStartMs = constrain(server.arg("dropStartMs").toInt(), 100, 600);
    if (server.hasArg("dropMinMs"))
        cfgPtr->dropMinMs = constrain(server.arg("dropMinMs").toInt(), 30, 200);
    if (server.hasArg("moveIntervalMs"))
        cfgPtr->moveIntervalMs = constrain(server.arg("moveIntervalMs").toInt(), 20, 200);
    if (server.hasArg("rotIntervalMs"))
        cfgPtr->rotIntervalMs = constrain(server.arg("rotIntervalMs").toInt(), 50, 300);
    if (server.hasArg("aiSkillPct"))
        cfgPtr->aiSkillPct = constrain(server.arg("aiSkillPct").toInt(), 0, 100);
    if (server.hasArg("jitterPct"))
        cfgPtr->jitterPct = constrain(server.arg("jitterPct").toInt(), 0, 50);

    setTetrisConfig(*cfgPtr);
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleApiBackground() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Auth required"); return; }

    if (server.hasArg("r")) cfgPtr->bgR = constrain(server.arg("r").toInt(), 0, 40);
    if (server.hasArg("g")) cfgPtr->bgG = constrain(server.arg("g").toInt(), 0, 40);
    if (server.hasArg("b")) cfgPtr->bgB = constrain(server.arg("b").toInt(), 0, 40);

    setTetrisConfig(*cfgPtr);
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleApiSave() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Auth required"); return; }

    if (server.hasArg("authPassword")) {
        String pw = server.arg("authPassword");
        if (pw.length() > 0 && pw.length() < sizeof(cfgPtr->authPassword)) {
            strncpy(cfgPtr->authPassword, pw.c_str(), sizeof(cfgPtr->authPassword) - 1);
            cfgPtr->authPassword[sizeof(cfgPtr->authPassword) - 1] = '\0';
        }
    }

    saveGridConfig(*cfgPtr);
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleApiDefaults() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Auth required"); return; }
    initDefaultConfig(*cfgPtr);
    setTetrisConfig(*cfgPtr);
    saveGridConfig(*cfgPtr);
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleApiWsToken() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Auth required"); return; }
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"token\":\"%s\"}", sessionToken);
    server.send(200, "application/json", buf);
}

static void handleApiRestart() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Auth required"); return; }
    server.send(200, "application/json", "{\"ok\":true}");
    delay(500);
    ESP.restart();
}

// ─── OTA Update ────────────────────────────────────────────────────────────

static void handleUpdatePage() {
    if (!isAuthenticated()) { redirectLogin(); return; }
    sendGzPage(UPDATE_PAGE_GZ, UPDATE_PAGE_GZ_LEN);
}

static void handleUpdateResult() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Auth required"); return; }
    server.send(200, "text/html",
        Update.hasError()
            ? "<html><body><h2>Update failed!</h2><a href='/'>Back</a></body></html>"
            : "<html><body><h2>Update OK! Rebooting...</h2></body></html>");
    delay(1000);
    if (!Update.hasError()) ESP.restart();
}

static void handleUpdateUpload() {
    if (!isAuthenticated()) return;
    HTTPUpload &upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("OTA: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.println(F("OTA: Success"));
        } else {
            Update.printError(Serial);
        }
    }
}

// ─── Setup & Loop ──────────────────────────────────────────────────────────

void setupWebServer(GridConfig &cfg) {
    cfgPtr = &cfg;
    loadSession();

    const char *headerKeys[] = {"Cookie"};
    server.collectHeaders(headerKeys, 1);

    // Pages
    server.on("/",         HTTP_GET,  handleDashboard);
    server.on("/login",    HTTP_GET,  handleLoginPage);
    server.on("/login",    HTTP_POST, handleLoginPost);
    server.on("/logout",   HTTP_GET,  handleLogout);
    server.on("/settings", HTTP_GET,  handleSettings);
    server.on("/tetris",   HTTP_GET,  handleTetrisPage);
    server.on("/update",   HTTP_GET,  handleUpdatePage);
    server.on("/update",   HTTP_POST, handleUpdateResult, handleUpdateUpload);

    // API
    server.on("/api/status",     HTTP_GET,  handleApiStatus);
    server.on("/api/brightness", HTTP_POST, handleApiBrightness);
    server.on("/api/effect",     HTTP_POST, handleApiEffect);
    server.on("/api/tuning",     HTTP_POST, handleApiTuning);
    server.on("/api/background", HTTP_POST, handleApiBackground);
    server.on("/api/save",       HTTP_POST, handleApiSave);
    server.on("/api/defaults",   HTTP_POST, handleApiDefaults);
    server.on("/api/ws-token",   HTTP_GET,  handleApiWsToken);
    server.on("/api/restart",    HTTP_POST, handleApiRestart);

    server.begin();
    Serial.println(F("Web server started on port 80"));
}

void loopWebServer() {
    server.handleClient();
}
