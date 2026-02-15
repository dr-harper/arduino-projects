#include "web_server.h"
#include "globals.h"
#include "html_pages.h"
#include "wifi_manager_setup.h"
#include "camera_manager.h"
#include "image_store.h"
#include "ml_inference.h"
#include <WebServer.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Update.h>
#include <esp_task_wdt.h>

// ─── Module state ───────────────────────────────────────────────────────────
static WebServer server(80);
static AiCameraConfig *cfgPtr = nullptr;

// Session token (persisted in NVS)
static char sessionToken[SESSION_TOKEN_LENGTH + 1] = "";

// Login rate limiting
static uint8_t       loginFailures = 0;
static unsigned long  lockoutStartMs = 0;
static const unsigned long LOCKOUT_DURATION_MS = 30000;

// MJPEG stream tracking
static bool streamActive = false;

// ─── JSON Escape Helper ─────────────────────────────────────────────────────
// Escapes \ and " in a string for safe JSON embedding.
static void jsonEscape(char *dst, size_t dstLen, const char *src) {
    size_t w = 0;
    for (const char *p = src; *p && w < dstLen - 1; p++) {
        if ((*p == '"' || *p == '\\') && w + 2 < dstLen) {
            dst[w++] = '\\';
        }
        dst[w++] = *p;
    }
    dst[w] = '\0';
}

// ─── NVS Helpers ────────────────────────────────────────────────────────────
static void loadSessionToken() {
    Preferences p;
    p.begin("aicamera", true);
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
        sessionToken[i] = charset[esp_random() % (sizeof(charset) - 1)];
    }
    sessionToken[SESSION_TOKEN_LENGTH] = '\0';

    Preferences p;
    p.begin("aicamera", false);
    p.putString("session", sessionToken);
    p.end();
}

void loadConfig(AiCameraConfig &cfg) {
    Preferences p;
    p.begin("aicamera", true);
    String tz = p.getString("timezone", DEFAULT_TIMEZONE);
    strncpy(cfg.timezone, tz.c_str(), sizeof(cfg.timezone) - 1);
    cfg.timezone[sizeof(cfg.timezone) - 1] = '\0';

    String pw = p.getString("authPass", DEFAULT_AUTH_PASSWORD);
    strncpy(cfg.authPassword, pw.c_str(), sizeof(cfg.authPassword) - 1);
    cfg.authPassword[sizeof(cfg.authPassword) - 1] = '\0';

    cfg.jpegQuality = p.getUChar("jpegQuality", CAM_JPEG_QUALITY);
    cfg.autoClassify = p.getBool("autoClassify", false);
    p.end();
}

void saveConfig(const AiCameraConfig &cfg) {
    Preferences p;
    p.begin("aicamera", false);
    p.putString("timezone", cfg.timezone);
    p.putString("authPass", cfg.authPassword);
    p.putUChar("jpegQuality", cfg.jpegQuality);
    p.putBool("autoClassify", cfg.autoClassify);
    p.end();
}

// ─── Auth Helpers ───────────────────────────────────────────────────────────
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

static String injectCSS(const char *pageTemplate) {
    String page = String(FPSTR(pageTemplate));
    page.replace("%CSS%", FPSTR(SHARED_CSS));
    return page;
}

// ─── Route: Login ───────────────────────────────────────────────────────────
static void handleLoginPage() {
    String page = injectCSS(LOGIN_PAGE);
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
    if (lockoutStartMs > 0 && (millis() - lockoutStartMs) < LOCKOUT_DURATION_MS) {
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
        String cookie = String("session=") + sessionToken + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=604800";
        server.sendHeader("Set-Cookie", cookie);
        server.sendHeader("Location", "/");
        server.send(302, "text/plain", "OK");
        Serial.println(F("Web UI: login successful"));
    } else {
        loginFailures++;
        Serial.printf("Web UI: login failed (%d/5)\n", loginFailures);
        if (loginFailures >= 5) {
            lockoutStartMs = millis();
            loginFailures = 0;
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
    p.begin("aicamera", false);
    p.remove("session");
    p.end();
    server.sendHeader("Set-Cookie", "session=; Path=/; Max-Age=0");
    server.sendHeader("Location", "/login");
    server.send(302);
}

// ─── Route: Dashboard ───────────────────────────────────────────────────────
static void handleDashboard() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    String page = injectCSS(DASHBOARD_PAGE);
    server.send(200, "text/html", page);
}

// ─── Route: Settings ────────────────────────────────────────────────────────
static void handleSettingsPage() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    String page = injectCSS(SETTINGS_PAGE);
    server.send(200, "text/html", page);
}

// ─── Route: Image Detail ────────────────────────────────────────────────────
static void handleImagePage() {
    if (!isAuthenticated()) { redirectToLogin(); return; }
    String page = injectCSS(IMAGE_DETAIL_PAGE);
    server.send(200, "text/html", page);
}

// ─── API: MJPEG Stream ─────────────────────────────────────────────────────
static void handleStream() {
    if (!isAuthenticated()) { server.send(401); return; }
    if (!cameraReady) { server.send(503, "text/plain", "Camera not ready"); return; }

    streamActive = true;

    // Switch to stream resolution
    setCameraFrameSize(STREAM_FRAME_SIZE);
    setCameraJpegQuality(STREAM_JPEG_QUALITY);

    WiFiClient client = server.client();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("Cache-Control: no-cache");
    client.println("Connection: keep-alive");
    client.println();

    while (client.connected()) {
        esp_task_wdt_reset();

        camera_fb_t *fb = captureFrame();
        if (!fb) {
            delay(50);
            continue;
        }

        client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                       (unsigned)fb->len);
        client.write(fb->buf, fb->len);
        client.println();

        releaseFrame(fb);

        // ~15 fps target
        delay(66);
    }

    // Restore capture resolution
    setCameraFrameSize(CAM_FRAME_SIZE);
    setCameraJpegQuality(cfgPtr->jpegQuality);
    streamActive = false;
}

// ─── API: Capture ───────────────────────────────────────────────────────────
static void handleApiCapture() {
    if (!isAuthenticated()) { server.send(401); return; }
    if (!cameraReady) { server.send(503, "text/plain", "Camera not ready"); return; }

    // Ensure capture resolution
    if (streamActive) {
        server.send(409, "text/plain", "Stream active — stop stream first");
        return;
    }

    setCameraFrameSize(CAM_FRAME_SIZE);
    setCameraJpegQuality(cfgPtr->jpegQuality);

    camera_fb_t *fb = captureFrame();
    if (!fb) {
        server.send(500, "text/plain", "Capture failed");
        return;
    }

    int idx = storeImage(fb->buf, fb->len, fb->width, fb->height);
    releaseFrame(fb);

    if (idx < 0) {
        server.send(500, "text/plain", "Storage failed");
        return;
    }

    // Auto-classify if ML is ready and enabled
    if (isInferenceReady() && cfgPtr->autoClassify) {
        CapturedImage *imgMut = getImageMutable(idx);
        if (imgMut) {
            imgMut->numResults = classifyImage(
                imgMut->jpegData, imgMut->jpegLength,
                imgMut->topResults, MAX_TOP_RESULTS);
        }
    }

    const CapturedImage *img = getImage(idx);
    char buf[384];
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "{\"id\":%d,\"time\":\"%s\",\"width\":%u,\"height\":%u,\"size\":%u",
        idx, img->isoTime, img->width, img->height, (unsigned)img->jpegLength);
    if (img->numResults > 0) {
        char safeLabel[128];
        jsonEscape(safeLabel, sizeof(safeLabel), img->topResults[0].label);
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            ",\"topLabel\":\"%s\",\"topConf\":%.2f",
            safeLabel, (double)img->topResults[0].confidence);
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "}");
    server.send(200, "application/json", buf);
}

// ─── API: List Images ───────────────────────────────────────────────────────
static void handleApiImages() {
    if (!isAuthenticated()) { server.send(401); return; }

    char buf[4096];
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "{\"count\":%d,\"capacity\":%d,\"images\":[",
        getImageCount(), getImageCapacity());

    bool first = true;
    for (int i = 0; i < getImageCapacity(); i++) {
        const CapturedImage *img = getImage(i);
        if (!img) continue;

        // Safety: stop before we could overflow (each entry ~250 bytes max)
        if (pos >= (int)sizeof(buf) - 400) {
            Serial.println(F("Warning: /api/images response truncated"));
            break;
        }

        char safeLabel[128] = "";
        if (img->numResults > 0) {
            jsonEscape(safeLabel, sizeof(safeLabel), img->topResults[0].label);
        }
        float topConf = (img->numResults > 0) ? img->topResults[0].confidence : 0.0f;
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s{\"id\":%d,\"time\":\"%s\",\"width\":%u,\"height\":%u,"
            "\"size\":%u,\"numResults\":%u,\"numTags\":%u,"
            "\"topLabel\":\"%s\",\"topConf\":%.2f}",
            first ? "" : ",",
            i, img->isoTime, img->width, img->height,
            (unsigned)img->jpegLength, img->numResults, img->numUserTags,
            safeLabel, (double)topConf);
        first = false;
    }

    pos += snprintf(buf + pos, sizeof(buf) - pos, "]}");
    server.send(200, "application/json", buf);
}

// ─── Helper: Parse numeric ID from URI path ────────────────────────────────
static int parseImageIdFromUri(const String &uri, int afterSlashPos) {
    String idStr = uri.substring(afterSlashPos);
    if (idStr.length() == 0) return -1;
    for (unsigned int i = 0; i < idStr.length(); i++) {
        if (!isDigit(idStr.charAt(i))) return -1;
    }
    return idStr.toInt();
}

// ─── API: Serve Image JPEG ──────────────────────────────────────────────────
static void handleApiImage() {
    if (!isAuthenticated()) { server.send(401); return; }

    // Parse image ID from URI: /api/image/0
    String uri = server.uri();
    int lastSlash = uri.lastIndexOf('/');
    if (lastSlash < 0) { server.send(400, "text/plain", "Bad request"); return; }

    int id = parseImageIdFromUri(uri, lastSlash + 1);
    if (id < 0) { server.send(400, "text/plain", "Invalid image ID"); return; }

    const CapturedImage *img = getImage(id);
    if (!img) {
        server.send(404, "text/plain", "Image not found");
        return;
    }

    server.sendHeader("Cache-Control", "max-age=3600");
    server.send_P(200, "image/jpeg", (const char *)img->jpegData, img->jpegLength);
}

// ─── Helper: Parse image ID from /api/image/<id>/tags URI ───────────────────
static int parseTagsImageId(const String &uri) {
    // URI format: /api/image/<id>/tags
    int tagSlash = uri.lastIndexOf('/');
    if (tagSlash < 0) return -1;
    String before = uri.substring(0, tagSlash);
    int idSlash = before.lastIndexOf('/');
    if (idSlash < 0) return -1;
    return parseImageIdFromUri(before, idSlash + 1);
}

// ─── API: GET Tags ──────────────────────────────────────────────────────────
static void handleApiImageTagsGet() {
    if (!isAuthenticated()) { server.send(401); return; }

    int id = parseTagsImageId(server.uri());
    if (id < 0) { server.send(400, "text/plain", "Invalid image ID"); return; }

    const CapturedImage *img = getImage(id);
    if (!img) { server.send(404, "text/plain", "Image not found"); return; }

    char buf[512];
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos, "{\"tags\":[");
    for (uint8_t i = 0; i < img->numUserTags; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s\"%s\"", i > 0 ? "," : "", img->userTags[i]);
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "],\"corrected\":%s}",
                    img->tagsCorrected ? "true" : "false");
    server.send(200, "application/json", buf);
}

// ─── API: POST Tags (Update) ───────────────────────────────────────────────
static void handleApiImageTagsPost() {
    if (!isAuthenticated()) { server.send(401); return; }

    int id = parseTagsImageId(server.uri());
    if (id < 0) { server.send(400, "text/plain", "Invalid image ID"); return; }

    CapturedImage *img = getImageMutable(id);
    if (!img) {
        server.send(404, "text/plain", "Image not found");
        return;
    }

    String body = server.arg("plain");
    if (body.length() == 0) {
        server.send(400, "text/plain", "No data");
        return;
    }

    // Parse tags array from JSON body: {"tags":["tag1","tag2"]}
    img->numUserTags = 0;
    int searchFrom = 0;
    while (img->numUserTags < MAX_USER_TAGS) {
        int start = body.indexOf('"', searchFrom);
        if (start < 0) break;
        int end = body.indexOf('"', start + 1);
        if (end < 0) break;

        String tag = body.substring(start + 1, end);
        // Validate: only allow alphanumeric, spaces, hyphens, underscores
        bool validTag = (tag != "tags" && tag.length() > 0 && tag.length() < MAX_TAG_LENGTH);
        for (unsigned int ci = 0; validTag && ci < tag.length(); ci++) {
            char ch = tag.charAt(ci);
            if (!isAlphaNumeric(ch) && ch != ' ' && ch != '-' && ch != '_') {
                validTag = false;
            }
        }
        if (validTag) {
            strncpy(img->userTags[img->numUserTags], tag.c_str(), MAX_TAG_LENGTH - 1);
            img->userTags[img->numUserTags][MAX_TAG_LENGTH - 1] = '\0';
            img->numUserTags++;
        }
        searchFrom = end + 1;
    }
    img->tagsCorrected = true;

    server.send(200, "text/plain", "OK");
}

// ─── API: Status ────────────────────────────────────────────────────────────
static void handleApiStatus() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Unauthorised"); return; }

    uint64_t uptimeSec = uptimeMs / 1000;
    unsigned long days  = (unsigned long)(uptimeSec / 86400);
    unsigned long hours = (unsigned long)((uptimeSec % 86400) / 3600);
    unsigned long mins  = (unsigned long)((uptimeSec % 3600) / 60);

    String timeStr = getFormattedTime();
    String ipStr   = WiFi.localIP().toString();

    char buf[1024];
    int pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "{\"version\":\"%s\","
        "\"uptime\":\"%lud %luh %lum\","
        "\"freeHeap\":%lu,"
        "\"freePsram\":%lu,"
        "\"time\":\"%s\","
        "\"synced\":%s,"
        "\"cameraReady\":%s,"
        "\"streamActive\":%s,"
        "\"wifiConnected\":%s,"
        "\"ip\":\"%s\","
        "\"timezone\":\"%s\","
        "\"imageCount\":%d,"
        "\"imageCapacity\":%d,"
        "\"jpegQuality\":%u,"
        "\"autoClassify\":%s,"
        "\"mlReady\":%s,"
        "\"lastInferenceMs\":%lu}",
        FW_VERSION,
        days, hours, mins,
        (unsigned long)ESP.getFreeHeap(),
        (unsigned long)ESP.getFreePsram(),
        timeStr.c_str(),
        isTimeSynced() ? "true" : "false",
        cameraReady ? "true" : "false",
        streamActive ? "true" : "false",
        (WiFi.status() == WL_CONNECTED) ? "true" : "false",
        ipStr.c_str(),
        cfgPtr->timezone,
        getImageCount(),
        getImageCapacity(),
        cfgPtr->jpegQuality,
        cfgPtr->autoClassify ? "true" : "false",
        isInferenceReady() ? "true" : "false",
        (unsigned long)getLastInferenceMs());

    server.send(200, "application/json", buf);
}

// ─── API: Settings ──────────────────────────────────────────────────────────
static void handleApiSettings() {
    if (!isAuthenticated()) { server.send(401); return; }

    bool changed = false;

    if (server.hasArg("timezone")) {
        String tz = server.arg("timezone");
        if (tz.length() > 0 && tz.length() < sizeof(cfgPtr->timezone)) {
            strncpy(cfgPtr->timezone, tz.c_str(), sizeof(cfgPtr->timezone) - 1);
            cfgPtr->timezone[sizeof(cfgPtr->timezone) - 1] = '\0';
            reconfigureNTP(cfgPtr->timezone);
            changed = true;
        }
    }
    if (server.hasArg("password")) {
        String pw = server.arg("password");
        if (pw.length() > 0 && pw.length() < sizeof(cfgPtr->authPassword)) {
            strncpy(cfgPtr->authPassword, pw.c_str(), sizeof(cfgPtr->authPassword) - 1);
            cfgPtr->authPassword[sizeof(cfgPtr->authPassword) - 1] = '\0';
            changed = true;
        }
    }
    if (server.hasArg("jpegQuality")) {
        cfgPtr->jpegQuality = constrain(server.arg("jpegQuality").toInt(), 5, 63);
        setCameraJpegQuality(cfgPtr->jpegQuality);
        changed = true;
    }
    if (server.hasArg("autoClassify")) {
        cfgPtr->autoClassify = (server.arg("autoClassify") == "1" ||
                                server.arg("autoClassify") == "true");
        changed = true;
    }

    if (changed) {
        saveConfig(*cfgPtr);
    }

    server.send(200, "text/plain", "OK");
}

// ─── API: Clear Images ──────────────────────────────────────────────────────
static void handleApiClearImages() {
    if (!isAuthenticated()) { server.send(401); return; }
    clearImages();
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
            Serial.printf("OTA complete: %u bytes\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
        // Restore watchdog regardless of success/failure
        esp_task_wdt_add(NULL);
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.abort();
        esp_task_wdt_add(NULL);
        Serial.println(F("OTA upload aborted"));
    }
}

static void handleUpdateResult() {
    if (!isAuthenticated()) { server.send(401); return; }

    if (Update.hasError()) {
        server.send(500, "text/plain", "Update failed");
    } else {
        server.send(200, "text/plain", "OK");
        Serial.println(F("Rebooting after OTA update..."));
        delay(500);
        ESP.restart();
    }
}

// ─── API: Delete Single Image ──────────────────────────────────────────────
static void handleApiImageDelete() {
    if (!isAuthenticated()) { server.send(401); return; }

    String uri = server.uri();
    int lastSlash = uri.lastIndexOf('/');
    if (lastSlash < 0) { server.send(400, "text/plain", "Bad request"); return; }

    int id = parseImageIdFromUri(uri, lastSlash + 1);
    if (id < 0) { server.send(400, "text/plain", "Invalid image ID"); return; }

    if (deleteImage(id)) {
        server.send(200, "text/plain", "OK");
    } else {
        server.send(404, "text/plain", "Image not found");
    }
}

// ─── API: Download Image ───────────────────────────────────────────────────
static void handleApiImageDownload() {
    if (!isAuthenticated()) { server.send(401); return; }

    int id = parseTagsImageId(server.uri());
    if (id < 0) { server.send(400, "text/plain", "Invalid image ID"); return; }

    const CapturedImage *img = getImage(id);
    if (!img) { server.send(404, "text/plain", "Image not found"); return; }

    char disposition[64];
    snprintf(disposition, sizeof(disposition),
        "attachment; filename=\"aicamera_%d.jpg\"", id);
    server.sendHeader("Content-Disposition", disposition);
    server.send_P(200, "image/jpeg", (const char *)img->jpegData, img->jpegLength);
}

// ─── API: Classify Image ───────────────────────────────────────────────────
static void handleApiClassify() {
    if (!isAuthenticated()) { server.send(401); return; }

    // Reuse suffix-style URI parsing: /api/image/<id>/classify
    int id = parseTagsImageId(server.uri());
    if (id < 0) { server.send(400, "text/plain", "Invalid image ID"); return; }

    if (!isInferenceReady()) {
        server.send(503, "text/plain", "ML inference not ready");
        return;
    }

    CapturedImage *img = getImageMutable(id);
    if (!img) { server.send(404, "text/plain", "Image not found"); return; }

    img->numResults = classifyImage(
        img->jpegData, img->jpegLength,
        img->topResults, MAX_TOP_RESULTS);

    char buf[1024];
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "{\"id\":%d,\"inferenceMs\":%lu,\"results\":[",
        id, (unsigned long)getLastInferenceMs());
    for (uint8_t i = 0; i < img->numResults; i++) {
        char safeLabel[128];
        jsonEscape(safeLabel, sizeof(safeLabel), img->topResults[i].label);
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s{\"label\":\"%s\",\"confidence\":%.4f,\"classIndex\":%u}",
            i > 0 ? "," : "",
            safeLabel,
            (double)img->topResults[i].confidence,
            img->topResults[i].classIndex);
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "]}");
    server.send(200, "application/json", buf);
}

// ─── API: Get Classification Results ────────────────────────────────────────
static void handleApiImageResults() {
    if (!isAuthenticated()) { server.send(401); return; }

    int id = parseTagsImageId(server.uri());
    if (id < 0) { server.send(400, "text/plain", "Invalid image ID"); return; }

    const CapturedImage *img = getImage(id);
    if (!img) { server.send(404, "text/plain", "Image not found"); return; }

    char buf[1024];
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "{\"id\":%d,\"mlReady\":%s,\"results\":[",
        id, isInferenceReady() ? "true" : "false");
    for (uint8_t i = 0; i < img->numResults; i++) {
        char safeLabel[128];
        jsonEscape(safeLabel, sizeof(safeLabel), img->topResults[i].label);
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s{\"label\":\"%s\",\"confidence\":%.4f,\"classIndex\":%u}",
            i > 0 ? "," : "",
            safeLabel,
            (double)img->topResults[i].confidence,
            img->topResults[i].classIndex);
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "]}");
    server.send(200, "application/json", buf);
}

static void handleNotFound() {
    server.sendHeader("Location", "/");
    server.send(302);
}

// ─── Public API ─────────────────────────────────────────────────────────────

void setupWebServer(AiCameraConfig &cfg) {
    cfgPtr = &cfg;

    // Load config from NVS
    loadConfig(cfg);

    // Restore session token
    loadSessionToken();

    // Collect cookies for authentication
    const char *headerKeys[] = {"Cookie"};
    server.collectHeaders(headerKeys, 1);

    // Pages
    server.on("/login",    HTTP_GET,  handleLoginPage);
    server.on("/login",    HTTP_POST, handleLoginPost);
    server.on("/logout",   HTTP_GET,  handleLogout);
    server.on("/",         HTTP_GET,  handleDashboard);
    server.on("/settings", HTTP_GET,  handleSettingsPage);
    server.on("/image",    HTTP_GET,  handleImagePage);
    server.on("/update",   HTTP_GET,  handleUpdatePage);
    server.on("/update",   HTTP_POST, handleUpdateResult, handleUpdateUpload);

    // Stream
    server.on("/stream",   HTTP_GET,  handleStream);

    // API
    server.on("/api/status",       HTTP_GET,  handleApiStatus);
    server.on("/api/capture",      HTTP_POST, handleApiCapture);
    server.on("/api/images",       HTTP_GET,  handleApiImages);
    server.on("/api/settings",     HTTP_POST, handleApiSettings);
    server.on("/api/clear-images", HTTP_POST, handleApiClearImages);

    // Dynamic image routes — use onNotFound fallback for /api/image/<id>
    // WebServer doesn't support path params, so we handle in onNotFound
    server.onNotFound([]() {
        String uri = server.uri();
        if (uri.startsWith("/api/image/")) {
            if (uri.endsWith("/tags")) {
                if (server.method() == HTTP_POST) {
                    handleApiImageTagsPost();
                } else if (server.method() == HTTP_GET) {
                    handleApiImageTagsGet();
                } else {
                    server.send(405, "text/plain", "Method not allowed");
                }
            } else if (uri.endsWith("/classify")) {
                if (server.method() == HTTP_POST) {
                    handleApiClassify();
                } else {
                    server.send(405, "text/plain", "Method not allowed");
                }
            } else if (uri.endsWith("/results")) {
                if (server.method() == HTTP_GET) {
                    handleApiImageResults();
                } else {
                    server.send(405, "text/plain", "Method not allowed");
                }
            } else if (uri.endsWith("/download")) {
                if (server.method() == HTTP_GET) {
                    handleApiImageDownload();
                } else {
                    server.send(405, "text/plain", "Method not allowed");
                }
            } else if (server.method() == HTTP_GET) {
                handleApiImage();
            } else if (server.method() == HTTP_DELETE) {
                handleApiImageDelete();
            } else {
                server.send(405, "text/plain", "Method not allowed");
            }
        } else {
            handleNotFound();
        }
    });

    server.begin();
    Serial.print(F("Web server started at http://"));
    Serial.println(WiFi.localIP());
}

void handleWebClient() {
    server.handleClient();
}
