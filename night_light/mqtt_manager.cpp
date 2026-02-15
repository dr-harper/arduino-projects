#include "mqtt_manager.h"
#include "globals.h"
#include "schedule.h"
#include "wifi_manager_setup.h"
#include <WiFi.h>
#include <PubSubClient.h>

// ─── Module state ───────────────────────────────────────────────────────────
static WiFiClient       wifiClient;
static PubSubClient     mqtt(wifiClient);
static NightLightConfig *cfgPtr = nullptr;

static char deviceId[7]  = "";     // Last 6 hex chars of MAC
static char baseTopic[32] = "";    // "nightlight/<ID>/"

// Pre-built topic strings (filled on setup)
static char topicAvail[48];
static char topicLightState[48];
static char topicLightSet[48];
static char topicSlotState[48];
static char topicSlotSet[48];
static char topicSlotSensorState[48];
static char topicNtpState[48];
static char topicRssiState[48];
static char topicUptimeState[48];
static char topicHeapState[48];
static char topicSpecialDateState[56];
static char topicChipTempState[48];

static unsigned long lastReconnectAttempt = 0;
static unsigned long lastSensorPublish    = 0;
static unsigned long mqttBackoff          = MQTT_RECONNECT_INTERVAL;

// Track last published state to avoid duplicate publishes
static bool     lastPublishedOn   = false;
static uint8_t  lastPublishedBri  = 0;
static uint8_t  lastPublishedR    = 0;
static uint8_t  lastPublishedG    = 0;
static uint8_t  lastPublishedB    = 0;
static uint8_t  lastPublishedW    = 0;
static uint8_t  lastPublishedEff  = 0;
static bool     lastStatePublished = false;

// ─── Helpers ────────────────────────────────────────────────────────────────

static void buildDeviceId() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(deviceId, sizeof(deviceId), "%02x%02x%02x", mac[3], mac[4], mac[5]);
}

static void buildTopics() {
    snprintf(baseTopic, sizeof(baseTopic), "nightlight/%s/", deviceId);

    snprintf(topicAvail,          sizeof(topicAvail),          "nightlight/%s/availability",   deviceId);
    snprintf(topicLightState,     sizeof(topicLightState),     "nightlight/%s/light/state",    deviceId);
    snprintf(topicLightSet,       sizeof(topicLightSet),       "nightlight/%s/light/set",      deviceId);
    snprintf(topicSlotState,      sizeof(topicSlotState),      "nightlight/%s/slot/state",     deviceId);
    snprintf(topicSlotSet,        sizeof(topicSlotSet),        "nightlight/%s/slot/set",       deviceId);
    snprintf(topicSlotSensorState,sizeof(topicSlotSensorState),"nightlight/%s/slot_sensor/state", deviceId);
    snprintf(topicNtpState,       sizeof(topicNtpState),       "nightlight/%s/ntp/state",      deviceId);
    snprintf(topicRssiState,      sizeof(topicRssiState),      "nightlight/%s/rssi/state",     deviceId);
    snprintf(topicUptimeState,    sizeof(topicUptimeState),    "nightlight/%s/uptime/state",   deviceId);
    snprintf(topicHeapState,      sizeof(topicHeapState),      "nightlight/%s/heap/state",     deviceId);
    snprintf(topicSpecialDateState,sizeof(topicSpecialDateState),"nightlight/%s/special_date/state", deviceId);
    snprintf(topicChipTempState,   sizeof(topicChipTempState),   "nightlight/%s/chip_temp/state",    deviceId);
}

// Shared HA device block (appended to every discovery payload)
static void appendDeviceBlock(String &json) {
    json += ",\"dev\":{";
    json += "\"ids\":[\"nightlight_";
    json += deviceId;
    json += "\"],";
    json += "\"name\":\"Night Light\",";
    json += "\"mf\":\"DIY\",";
    json += "\"mdl\":\"ESP32-C3 Night Light\",";
    json += "\"sw\":\"";
    json += FW_VERSION;
    json += "\"";
    json += "}";
}

static const char* effectName(uint8_t e) {
    if (e < EFFECT_COUNT) return EFFECT_NAMES[e];
    return "Solid";
}

// ─── HA Discovery Payloads ──────────────────────────────────────────────────

static void publishDiscoveryLight() {
    char topic[96];
    snprintf(topic, sizeof(topic), "homeassistant/light/nightlight_%s/config", deviceId);

    String json = "{";
    json += "\"name\":\"Light\",";
    json += "\"uniq_id\":\"nightlight_";  json += deviceId;  json += "_light\",";
    json += "\"obj_id\":\"nightlight_";   json += deviceId;  json += "_light\",";
    json += "\"schema\":\"json\",";
    json += "\"stat_t\":\"";  json += topicLightState;  json += "\",";
    json += "\"cmd_t\":\"";   json += topicLightSet;    json += "\",";
    json += "\"avty_t\":\"";  json += topicAvail;       json += "\",";
    json += "\"brightness\":true,";
    json += "\"bri_scl\":255,";
    json += "\"supported_color_modes\":[\"rgbw\"],";
    json += "\"effect\":true,";
    json += "\"effect_list\":[";
    for (uint8_t i = 0; i < EFFECT_COUNT; i++) {
        if (i > 0) json += ",";
        json += "\"";
        json += EFFECT_NAMES[i];
        json += "\"";
    }
    json += "]";
    appendDeviceBlock(json);
    json += "}";

    mqtt.publish(topic, json.c_str(), true);
}

static void publishDiscoverySlotSelect() {
    char topic[96];
    snprintf(topic, sizeof(topic), "homeassistant/select/nightlight_%s_slot/config", deviceId);

    String json = "{";
    json += "\"name\":\"Slot Override\",";
    json += "\"uniq_id\":\"nightlight_";  json += deviceId;  json += "_slot\",";
    json += "\"obj_id\":\"nightlight_";   json += deviceId;  json += "_slot\",";
    json += "\"stat_t\":\"";  json += topicSlotState;  json += "\",";
    json += "\"cmd_t\":\"";   json += topicSlotSet;    json += "\",";
    json += "\"avty_t\":\"";  json += topicAvail;      json += "\",";
    json += "\"icon\":\"mdi:calendar-clock\",";
    json += "\"options\":[\"Auto\"";
    for (uint8_t i = 0; i < cfgPtr->numSlots; i++) {
        if (cfgPtr->slots[i].label[0] != '\0') {
            json += ",\"";
            json += cfgPtr->slots[i].label;
            json += "\"";
        }
    }
    json += "]";
    appendDeviceBlock(json);
    json += "}";

    mqtt.publish(topic, json.c_str(), true);
}

static void publishDiscoverySlotSensor() {
    char topic[96];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/nightlight_%s_active_slot/config", deviceId);

    String json = "{";
    json += "\"name\":\"Active Slot\",";
    json += "\"uniq_id\":\"nightlight_";  json += deviceId;  json += "_active_slot\",";
    json += "\"obj_id\":\"nightlight_";   json += deviceId;  json += "_active_slot\",";
    json += "\"stat_t\":\"";  json += topicSlotSensorState;  json += "\",";
    json += "\"avty_t\":\"";  json += topicAvail;            json += "\",";
    json += "\"icon\":\"mdi:clock-outline\"";
    appendDeviceBlock(json);
    json += "}";

    mqtt.publish(topic, json.c_str(), true);
}

static void publishDiscoveryNtpBinarySensor() {
    char topic[96];
    snprintf(topic, sizeof(topic), "homeassistant/binary_sensor/nightlight_%s_ntp/config", deviceId);

    String json = "{";
    json += "\"name\":\"NTP Sync\",";
    json += "\"uniq_id\":\"nightlight_";  json += deviceId;  json += "_ntp\",";
    json += "\"obj_id\":\"nightlight_";   json += deviceId;  json += "_ntp\",";
    json += "\"stat_t\":\"";  json += topicNtpState;  json += "\",";
    json += "\"avty_t\":\"";  json += topicAvail;     json += "\",";
    json += "\"dev_cla\":\"connectivity\",";
    json += "\"entity_category\":\"diagnostic\"";
    appendDeviceBlock(json);
    json += "}";

    mqtt.publish(topic, json.c_str(), true);
}

static void publishDiscoveryRssiSensor() {
    char topic[96];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/nightlight_%s_rssi/config", deviceId);

    String json = "{";
    json += "\"name\":\"WiFi Signal\",";
    json += "\"uniq_id\":\"nightlight_";  json += deviceId;  json += "_rssi\",";
    json += "\"obj_id\":\"nightlight_";   json += deviceId;  json += "_rssi\",";
    json += "\"stat_t\":\"";  json += topicRssiState;  json += "\",";
    json += "\"avty_t\":\"";  json += topicAvail;      json += "\",";
    json += "\"dev_cla\":\"signal_strength\",";
    json += "\"unit_of_meas\":\"dBm\",";
    json += "\"entity_category\":\"diagnostic\"";
    appendDeviceBlock(json);
    json += "}";

    mqtt.publish(topic, json.c_str(), true);
}

static void publishDiscoveryUptimeSensor() {
    char topic[96];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/nightlight_%s_uptime/config", deviceId);

    String json = "{";
    json += "\"name\":\"Uptime\",";
    json += "\"uniq_id\":\"nightlight_";  json += deviceId;  json += "_uptime\",";
    json += "\"obj_id\":\"nightlight_";   json += deviceId;  json += "_uptime\",";
    json += "\"stat_t\":\"";  json += topicUptimeState;  json += "\",";
    json += "\"avty_t\":\"";  json += topicAvail;        json += "\",";
    json += "\"dev_cla\":\"duration\",";
    json += "\"unit_of_meas\":\"s\",";
    json += "\"entity_category\":\"diagnostic\"";
    appendDeviceBlock(json);
    json += "}";

    mqtt.publish(topic, json.c_str(), true);
}

static void publishDiscoveryHeapSensor() {
    char topic[96];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/nightlight_%s_heap/config", deviceId);

    String json = "{";
    json += "\"name\":\"Free Memory\",";
    json += "\"uniq_id\":\"nightlight_";  json += deviceId;  json += "_heap\",";
    json += "\"obj_id\":\"nightlight_";   json += deviceId;  json += "_heap\",";
    json += "\"stat_t\":\"";  json += topicHeapState;  json += "\",";
    json += "\"avty_t\":\"";  json += topicAvail;      json += "\",";
    json += "\"unit_of_meas\":\"B\",";
    json += "\"icon\":\"mdi:memory\",";
    json += "\"entity_category\":\"diagnostic\"";
    appendDeviceBlock(json);
    json += "}";

    mqtt.publish(topic, json.c_str(), true);
}

static void publishDiscoverySpecialDateSensor() {
    char topic[96];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/nightlight_%s_special_date/config", deviceId);

    String json = "{";
    json += "\"name\":\"Special Date\",";
    json += "\"uniq_id\":\"nightlight_";  json += deviceId;  json += "_special_date\",";
    json += "\"obj_id\":\"nightlight_";   json += deviceId;  json += "_special_date\",";
    json += "\"stat_t\":\"";  json += topicSpecialDateState;  json += "\",";
    json += "\"avty_t\":\"";  json += topicAvail;             json += "\",";
    json += "\"icon\":\"mdi:cake-variant\"";
    appendDeviceBlock(json);
    json += "}";

    mqtt.publish(topic, json.c_str(), true);
}

static void publishDiscoveryChipTempSensor() {
    char topic[96];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/nightlight_%s_chip_temp/config", deviceId);

    String json = "{";
    json += "\"name\":\"Chip Temperature\",";
    json += "\"uniq_id\":\"nightlight_";  json += deviceId;  json += "_chip_temp\",";
    json += "\"obj_id\":\"nightlight_";   json += deviceId;  json += "_chip_temp\",";
    json += "\"stat_t\":\"";  json += topicChipTempState;  json += "\",";
    json += "\"avty_t\":\"";  json += topicAvail;          json += "\",";
    json += "\"dev_cla\":\"temperature\",";
    json += "\"unit_of_meas\":\"°C\",";
    json += "\"icon\":\"mdi:thermometer\",";
    json += "\"entity_category\":\"diagnostic\"";
    appendDeviceBlock(json);
    json += "}";

    mqtt.publish(topic, json.c_str(), true);
}

static void publishAllDiscovery() {
    publishDiscoveryLight();
    publishDiscoverySlotSelect();
    publishDiscoverySlotSensor();
    publishDiscoveryNtpBinarySensor();
    publishDiscoveryRssiSensor();
    publishDiscoveryUptimeSensor();
    publishDiscoveryHeapSensor();
    publishDiscoverySpecialDateSensor();
    publishDiscoveryChipTempSensor();
}

// ─── State Publishing ───────────────────────────────────────────────────────

static void publishLightStateInternal() {
    if (!mqtt.connected()) return;

    // Determine current effective state
    bool on = cfgPtr->ledsEnabled;
    uint8_t bri = 0, r = 0, g = 0, b = 0, w = 0, eff = EFFECT_SOLID;

    if (on && manualOverride) {
        bri = userBrightnessActive ? userBrightness : overrideBrightness;
        r   = overrideR;
        g   = overrideG;
        b   = overrideB;
        w   = overrideW;
        eff = overrideEffect;
    } else if (on && currentSlotIndex >= 0 && currentSlotIndex < cfgPtr->numSlots) {
        const ScheduleSlot &slot = cfgPtr->slots[currentSlotIndex];
        bri = userBrightnessActive ? userBrightness : slot.brightness;
        r   = slot.red;
        g   = slot.green;
        b   = slot.blue;
        w   = slot.white;
        eff = slot.effect;
    } else {
        on = false;
    }

    // Deduplicate
    if (lastStatePublished &&
        lastPublishedOn == on &&
        lastPublishedBri == bri &&
        lastPublishedR == r &&
        lastPublishedG == g &&
        lastPublishedB == b &&
        lastPublishedW == w &&
        lastPublishedEff == eff) {
        return;
    }

    String json = "{";
    json += "\"state\":\"";
    json += on ? "ON" : "OFF";
    json += "\"";
    if (on) {
        json += ",\"brightness\":";  json += String(bri);
        json += ",\"color_mode\":\"rgbw\"";
        json += ",\"color\":{\"r\":"; json += String(r);
        json += ",\"g\":"; json += String(g);
        json += ",\"b\":"; json += String(b);
        json += ",\"w\":"; json += String(w);
        json += "}";
        json += ",\"effect\":\"";
        json += effectName(eff);
        json += "\"";
    }
    json += "}";

    mqtt.publish(topicLightState, json.c_str(), true);

    lastPublishedOn  = on;
    lastPublishedBri = bri;
    lastPublishedR   = r;
    lastPublishedG   = g;
    lastPublishedB   = b;
    lastPublishedW   = w;
    lastPublishedEff = eff;
    lastStatePublished = true;
}

static void publishSlotState() {
    if (!mqtt.connected()) return;

    mqtt.publish(topicSlotState, manualOverride ? "Manual" : "Auto", true);

    // Active slot sensor
    if (currentSlotIndex >= 0 && currentSlotIndex < cfgPtr->numSlots) {
        mqtt.publish(topicSlotSensorState, cfgPtr->slots[currentSlotIndex].label, true);
    } else {
        mqtt.publish(topicSlotSensorState, "None", true);
    }
}

static void publishSensors() {
    if (!mqtt.connected()) return;

    // NTP sync status
    mqtt.publish(topicNtpState, isTimeSynced() ? "ON" : "OFF", true);

    // WiFi RSSI
    char rssi[8];
    snprintf(rssi, sizeof(rssi), "%d", WiFi.RSSI());
    mqtt.publish(topicRssiState, rssi, true);

    // Uptime in seconds (uses 64-bit counter to survive millis() overflow)
    char uptimeBuf[16];
    snprintf(uptimeBuf, sizeof(uptimeBuf), "%llu", (unsigned long long)(uptimeMs / 1000));
    mqtt.publish(topicUptimeState, uptimeBuf, true);

    // Free heap in bytes
    char heapBuf[12];
    snprintf(heapBuf, sizeof(heapBuf), "%lu", (unsigned long)ESP.getFreeHeap());
    mqtt.publish(topicHeapState, heapBuf, true);

    // Chip temperature
    char tempBuf[8];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f", chipTempC);
    mqtt.publish(topicChipTempState, tempBuf, true);

    // Active special date
    if (specialDateOverrideActive && currentSpecialDateIndex >= 0 &&
        currentSpecialDateIndex < cfgPtr->numSpecialDates) {
        mqtt.publish(topicSpecialDateState,
                     cfgPtr->specialDates[currentSpecialDateIndex].label, true);
    } else {
        mqtt.publish(topicSpecialDateState, "None", true);
    }
}

// ─── Command Handling ───────────────────────────────────────────────────────

// Find effect index by name, returns -1 if not found
static int8_t effectIndexByName(const char *name) {
    for (uint8_t i = 0; i < EFFECT_COUNT; i++) {
        if (strcasecmp(name, EFFECT_NAMES[i]) == 0) return (int8_t)i;
    }
    return -1;
}

// Simple JSON string extractor — finds "key":"value" and copies value into buf
static bool extractJsonString(const char *json, const char *key, char *buf, size_t bufLen) {
    // Build search pattern: "key":"
    char pattern[48];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);

    const char *start = strstr(json, pattern);
    if (!start) return false;

    start += strlen(pattern);
    const char *end = strchr(start, '"');
    if (!end || (size_t)(end - start) >= bufLen) return false;

    size_t len = (size_t)(end - start);
    memcpy(buf, start, len);
    buf[len] = '\0';
    return true;
}

// Simple JSON integer extractor — finds "key":123
static bool extractJsonInt(const char *json, const char *key, int &value) {
    char pattern[48];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);

    const char *start = strstr(json, pattern);
    if (!start) return false;

    start += strlen(pattern);
    // Skip whitespace
    while (*start == ' ') start++;
    value = atoi(start);
    return true;
}

static void handleLightCommand(const char *payload) {
    // Parse state
    bool turnOn = true;
    char stateStr[8];
    if (extractJsonString(payload, "state", stateStr, sizeof(stateStr))) {
        turnOn = (strcmp(stateStr, "ON") == 0);
    }

    if (!turnOn) {
        cfgPtr->ledsEnabled = false;
        saveSchedule(*cfgPtr);
        lastStatePublished = false;
        publishLightStateInternal();
        publishSlotState();
        return;
    }

    // Ensure LEDs are on
    if (!cfgPtr->ledsEnabled) {
        cfgPtr->ledsEnabled = true;
        saveSchedule(*cfgPtr);
    }

    // Parse brightness — independent of mode
    int bri;
    if (extractJsonInt(payload, "brightness", bri)) {
        userBrightness = constrain(bri, 0, 255);
        userBrightnessActive = true;
    }

    // Track whether colour or effect is being changed (triggers pretty mode)
    bool hasColour = false;
    bool hasEffect = false;

    // Parse colour — look for "color":{"r":N,"g":N,"b":N,"w":N}
    const char *colourStart = strstr(payload, "\"color\"");
    if (colourStart) {
        const char *colourObj = strchr(colourStart, '{');
        if (colourObj) {
            hasColour = true;
            // Seed from slot when first entering pretty mode
            if (!manualOverride && currentSlotIndex >= 0 && currentSlotIndex < cfgPtr->numSlots) {
                const ScheduleSlot &slot = cfgPtr->slots[currentSlotIndex];
                overrideR = slot.red;
                overrideG = slot.green;
                overrideB = slot.blue;
                overrideW = slot.white;
                overrideBrightness = slot.brightness;
                overrideEffect = slot.effect;
            }
            int r, g, b, w;
            if (extractJsonInt(colourObj, "r", r)) overrideR = constrain(r, 0, 255);
            if (extractJsonInt(colourObj, "g", g)) overrideG = constrain(g, 0, 255);
            if (extractJsonInt(colourObj, "b", b)) overrideB = constrain(b, 0, 255);
            if (extractJsonInt(colourObj, "w", w)) overrideW = constrain(w, 0, 255);
        }
    }

    // Parse effect
    char effStr[24];
    if (extractJsonString(payload, "effect", effStr, sizeof(effStr))) {
        int8_t idx = effectIndexByName(effStr);
        if (idx >= 0) {
            hasEffect = true;
            // Seed from slot when first entering pretty mode
            if (!manualOverride && !hasColour
                && currentSlotIndex >= 0 && currentSlotIndex < cfgPtr->numSlots) {
                const ScheduleSlot &slot = cfgPtr->slots[currentSlotIndex];
                overrideR = slot.red;
                overrideG = slot.green;
                overrideB = slot.blue;
                overrideW = slot.white;
                overrideBrightness = slot.brightness;
                overrideEffect = slot.effect;
            }
            overrideEffect = (uint8_t)idx;
        }
    }

    // Enter pretty mode only if colour or effect was explicitly changed
    if (hasColour || hasEffect) {
        manualOverride = true;
    }

    lastStatePublished = false;
    publishLightStateInternal();
    publishSlotState();
}

static void handleSlotCommand(const char *payload) {
    if (strcasecmp(payload, "Auto") == 0) {
        manualOverride = false;
    } else {
        // Find matching slot by label
        for (uint8_t i = 0; i < cfgPtr->numSlots; i++) {
            if (strcmp(payload, cfgPtr->slots[i].label) == 0) {
                const ScheduleSlot &slot = cfgPtr->slots[i];
                overrideR = slot.red;
                overrideG = slot.green;
                overrideB = slot.blue;
                overrideW = slot.white;
                overrideBrightness = slot.brightness;
                overrideEffect = slot.effect;
                manualOverride = true;
                break;
            }
        }
    }
    lastStatePublished = false;
    publishLightStateInternal();
    publishSlotState();
}

static void mqttCallback(char *topic, byte *payload, unsigned int length) {
    // Null-terminate the payload
    char buf[MQTT_BUFFER_SIZE];
    size_t copyLen = min((unsigned int)(sizeof(buf) - 1), length);
    memcpy(buf, payload, copyLen);
    buf[copyLen] = '\0';

    Serial.print(F("MQTT recv ["));
    Serial.print(topic);
    Serial.print(F("] "));
    Serial.println(buf);

    if (strcmp(topic, topicLightSet) == 0) {
        handleLightCommand(buf);
    } else if (strcmp(topic, topicSlotSet) == 0) {
        handleSlotCommand(buf);
    }
}

// ─── Connection Management ──────────────────────────────────────────────────

static bool connectMqtt() {
    if (!cfgPtr->mqtt.enabled || cfgPtr->mqtt.host[0] == '\0') {
        return false;
    }

    // Build client ID
    char clientId[24];
    snprintf(clientId, sizeof(clientId), "nightlight_%s", deviceId);

    Serial.print(F("MQTT connecting to "));
    Serial.print(cfgPtr->mqtt.host);
    Serial.print(F(":"));
    Serial.println(cfgPtr->mqtt.port);

    mqtt.setServer(cfgPtr->mqtt.host, cfgPtr->mqtt.port);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(MQTT_BUFFER_SIZE);
    mqtt.setKeepAlive(MQTT_KEEPALIVE);
    mqtt.setSocketTimeout(5);

    bool connected;
    if (cfgPtr->mqtt.username[0] != '\0') {
        connected = mqtt.connect(clientId,
                                 cfgPtr->mqtt.username,
                                 cfgPtr->mqtt.password,
                                 topicAvail, 1, true, "offline");
    } else {
        connected = mqtt.connect(clientId,
                                 nullptr, nullptr,
                                 topicAvail, 1, true, "offline");
    }

    if (connected) {
        Serial.println(F("MQTT connected"));

        // Publish online availability
        mqtt.publish(topicAvail, "online", true);

        // Publish all HA discovery payloads
        publishAllDiscovery();

        // Subscribe to command topics
        mqtt.subscribe(topicLightSet);
        mqtt.subscribe(topicSlotSet);

        // Publish initial state
        lastStatePublished = false;
        publishLightStateInternal();
        publishSlotState();
        publishSensors();
    } else {
        Serial.print(F("MQTT connect failed, rc="));
        Serial.println(mqtt.state());
    }

    return connected;
}

// ─── Public API ─────────────────────────────────────────────────────────────

void setupMqtt(NightLightConfig &cfg) {
    cfgPtr = &cfg;
    buildDeviceId();
    buildTopics();

    Serial.print(F("MQTT device ID: "));
    Serial.println(deviceId);

    if (cfg.mqtt.enabled && cfg.mqtt.host[0] != '\0') {
        connectMqtt();
    }
}

void loopMqtt() {
    if (!cfgPtr || !cfgPtr->mqtt.enabled || cfgPtr->mqtt.host[0] == '\0') {
        return;
    }

    if (mqtt.connected()) {
        mqtt.loop();

        // Periodic sensor updates
        unsigned long now = millis();
        if (now - lastSensorPublish >= MQTT_STATE_INTERVAL) {
            lastSensorPublish = now;
            publishSensors();
            publishLightStateInternal();
            publishSlotState();
        }
    } else {
        // Non-blocking reconnect with exponential backoff
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= mqttBackoff) {
            lastReconnectAttempt = now;
            if (connectMqtt()) {
                mqttBackoff = MQTT_RECONNECT_INTERVAL;  // Reset on success
            } else {
                mqttBackoff = min(mqttBackoff * 2, (unsigned long)MQTT_BACKOFF_MAX);
                Serial.printf("MQTT next retry in %lus\n", mqttBackoff / 1000);
            }
        }
    }
}

void mqttPublishLightState() {
    lastStatePublished = false;
    publishLightStateInternal();
    publishSlotState();
}

void mqttPublishScheduleChanged() {
    if (!mqtt.connected()) return;

    // Re-publish slot select discovery (options may have changed)
    publishDiscoverySlotSelect();

    // Publish current state
    lastStatePublished = false;
    publishLightStateInternal();
    publishSlotState();
}

void mqttDisconnect() {
    if (mqtt.connected()) {
        mqtt.publish(topicAvail, "offline", true);
        mqtt.disconnect();
        Serial.println(F("MQTT disconnected"));
    }
    mqttBackoff = MQTT_RECONNECT_INTERVAL;  // Reset backoff for fresh reconnect
}

bool isMqttConnected() {
    return mqtt.connected();
}
