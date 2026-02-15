#include "mqtt_client.h"
#include "persistence.h"
#include "led_effects.h"
#include "tetris_effect.h"
#include "snake_game.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ─── State ────────────────────────────────────────────────────────────────
static WiFiClient   mqttWifi;
static PubSubClient mqttClient(mqttWifi);
static GridConfig  *cfgPtr = nullptr;

// Device ID from MAC (last 6 hex chars)
static char deviceId[7];

// Topics (built once in setupMqtt)
static char topicAvail[40];      // ledgrid/<ID>/availability
static char topicState[40];      // ledgrid/<ID>/light/state
static char topicCmd[40];        // ledgrid/<ID>/light/set
static char topicDiscovery[64];  // homeassistant/light/ledgrid_<ID>/config
static char topicDiagnostics[40]; // ledgrid/<ID>/diagnostics

// Reconnection with exponential backoff
static unsigned long lastReconnectMs = 0;
static unsigned long reconnectInterval = 5000;
static const unsigned long RECONNECT_CAP = 60000;

// Heartbeat
static unsigned long lastHeartbeatMs = 0;
static const unsigned long HEARTBEAT_INTERVAL = 60000;

// Deduplication — last published values
static uint8_t lastPubBrightness = 0;
static uint8_t lastPubEffect     = 0xFF;
static bool    lastPubOn         = true;
static bool    forcePublish      = false;

// "OFF" stores previous brightness so ON can restore it
static uint8_t priorBrightness = 0;

// Debounced persistence for MQTT-driven changes
static bool          mqttDirty    = false;
static unsigned long lastDirtyMs  = 0;

// ─── Helpers ──────────────────────────────────────────────────────────────

static void buildDeviceId() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(deviceId, sizeof(deviceId), "%02x%02x%02x", mac[3], mac[4], mac[5]);
}

static void buildTopics() {
    snprintf(topicAvail,       sizeof(topicAvail),       "ledgrid/%s/availability",  deviceId);
    snprintf(topicState,       sizeof(topicState),       "ledgrid/%s/light/state",   deviceId);
    snprintf(topicCmd,         sizeof(topicCmd),          "ledgrid/%s/light/set",     deviceId);
    snprintf(topicDiscovery,   sizeof(topicDiscovery),   "homeassistant/light/ledgrid_%s/config", deviceId);
    snprintf(topicDiagnostics, sizeof(topicDiagnostics), "ledgrid/%s/diagnostics",   deviceId);
}

// ─── Publish ──────────────────────────────────────────────────────────────

static void publishStateInternal() {
    if (!mqttClient.connected()) return;

    bool isOn = cfgPtr->brightness > 0;

    // Deduplication
    if (!forcePublish &&
        cfgPtr->brightness == lastPubBrightness &&
        (uint8_t)cfgPtr->currentEffect == lastPubEffect &&
        isOn == lastPubOn) {
        return;
    }
    forcePublish = false;

    const char *effectName = "Tetris";
    if (cfgPtr->currentEffect < EFFECT_COUNT) {
        effectName = EFFECT_NAMES[cfgPtr->currentEffect];
    }

    JsonDocument doc;
    doc["state"]      = isOn ? "ON" : "OFF";
    doc["brightness"] = cfgPtr->brightness;
    doc["color_mode"] = "brightness";
    doc["effect"]     = effectName;

    char buf[160];
    serializeJson(doc, buf, sizeof(buf));
    mqttClient.publish(topicState, buf, true);

    lastPubBrightness = cfgPtr->brightness;
    lastPubEffect     = (uint8_t)cfgPtr->currentEffect;
    lastPubOn         = isOn;
}

// ─── HA Auto-Discovery ───────────────────────────────────────────────────

// Shared device block for all discovery payloads
static void addDeviceBlock(JsonObject &dev) {
    char devId[24];
    snprintf(devId, sizeof(devId), "ledgrid_%s", deviceId);
    JsonArray ids = dev["ids"].to<JsonArray>();
    ids.add(devId);
    dev["name"] = "LED Grid";
    dev["mf"]   = "DIY";
    dev["mdl"]  = "ESP32-C3 LED Grid";
    dev["sw"]   = FW_VERSION;
}

static void publishDiscovery() {
    char buf[MQTT_BUFFER_SIZE];
    size_t len;

    // ── Light entity ─────────────────────────────────────────────────
    {
        JsonDocument doc;
        doc["name"]    = "Light";

        char uniqId[32];
        snprintf(uniqId, sizeof(uniqId), "ledgrid_%s_light", deviceId);
        doc["uniq_id"] = uniqId;
        doc["schema"]  = "json";
        doc["stat_t"]  = topicState;
        doc["cmd_t"]   = topicCmd;
        doc["avty_t"]  = topicAvail;
        doc["bri_scl"] = 255;
        doc["effect"]  = true;

        JsonArray colorModes = doc["supported_color_modes"].to<JsonArray>();
        colorModes.add("brightness");
        doc["color_mode"] = true;

        JsonArray effects = doc["effect_list"].to<JsonArray>();
        for (int i = 0; i < EFFECT_COUNT; i++) {
            effects.add(EFFECT_NAMES[i]);
        }

        JsonObject dev = doc["dev"].to<JsonObject>();
        addDeviceBlock(dev);

        len = serializeJson(doc, buf, sizeof(buf));
        if (len >= sizeof(buf)) {
            Serial.println(F("MQTT: Light discovery truncated!"));
        } else {
            mqttClient.publish(topicDiscovery, (const uint8_t*)buf, len, true);
        }
    }

    // ── IP Address sensor ────────────────────────────────────────────
    {
        JsonDocument doc;
        doc["name"]    = "IP Address";

        char uniqId[32];
        snprintf(uniqId, sizeof(uniqId), "ledgrid_%s_ip", deviceId);
        doc["uniq_id"] = uniqId;
        doc["stat_t"]  = topicDiagnostics;
        doc["val_tpl"] = "{{ value_json.ip }}";
        doc["avty_t"]  = topicAvail;
        doc["ic"]      = "mdi:ip-network";
        doc["ent_cat"] = "diagnostic";

        JsonObject dev = doc["dev"].to<JsonObject>();
        addDeviceBlock(dev);

        char topic[80];
        snprintf(topic, sizeof(topic),
                 "homeassistant/sensor/ledgrid_%s/ip/config", deviceId);

        len = serializeJson(doc, buf, sizeof(buf));
        if (len < sizeof(buf)) {
            mqttClient.publish(topic, (const uint8_t*)buf, len, true);
        }
    }

    // ── Uptime sensor ────────────────────────────────────────────────
    {
        JsonDocument doc;
        doc["name"]    = "Uptime";

        char uniqId[32];
        snprintf(uniqId, sizeof(uniqId), "ledgrid_%s_uptime", deviceId);
        doc["uniq_id"] = uniqId;
        doc["stat_t"]  = topicDiagnostics;
        doc["val_tpl"] = "{{ value_json.uptime }}";
        doc["avty_t"]  = topicAvail;
        doc["ic"]      = "mdi:clock-outline";
        doc["ent_cat"] = "diagnostic";

        JsonObject dev = doc["dev"].to<JsonObject>();
        addDeviceBlock(dev);

        char topic[80];
        snprintf(topic, sizeof(topic),
                 "homeassistant/sensor/ledgrid_%s/uptime/config", deviceId);

        len = serializeJson(doc, buf, sizeof(buf));
        if (len < sizeof(buf)) {
            mqttClient.publish(topic, (const uint8_t*)buf, len, true);
        }
    }

    // ── Free Heap sensor ─────────────────────────────────────────────
    {
        JsonDocument doc;
        doc["name"]    = "Free Heap";

        char uniqId[32];
        snprintf(uniqId, sizeof(uniqId), "ledgrid_%s_heap", deviceId);
        doc["uniq_id"] = uniqId;
        doc["stat_t"]  = topicDiagnostics;
        doc["val_tpl"] = "{{ value_json.heap }}";
        doc["unit_of_meas"] = "KB";
        doc["avty_t"]  = topicAvail;
        doc["ic"]      = "mdi:memory";
        doc["ent_cat"] = "diagnostic";

        JsonObject dev = doc["dev"].to<JsonObject>();
        addDeviceBlock(dev);

        char topic[80];
        snprintf(topic, sizeof(topic),
                 "homeassistant/sensor/ledgrid_%s/heap/config", deviceId);

        len = serializeJson(doc, buf, sizeof(buf));
        if (len < sizeof(buf)) {
            mqttClient.publish(topic, (const uint8_t*)buf, len, true);
        }
    }
}

// ─── Diagnostics Publish ─────────────────────────────────────────────────

static void publishDiagnostics() {
    if (!mqttClient.connected()) return;

    unsigned long up = millis() / 1000;
    unsigned long days  = up / 86400;
    unsigned long hours = (up % 86400) / 3600;
    unsigned long mins  = (up % 3600) / 60;

    JsonDocument doc;
    doc["ip"]     = WiFi.localIP().toString();

    char uptimeBuf[20];
    snprintf(uptimeBuf, sizeof(uptimeBuf), "%lud %luh %lum", days, hours, mins);
    doc["uptime"] = uptimeBuf;
    doc["heap"]   = (unsigned long)(ESP.getFreeHeap() / 1024);

    char buf[128];
    serializeJson(doc, buf, sizeof(buf));
    mqttClient.publish(topicDiagnostics, buf, true);
}

// ─── Command Callback ────────────────────────────────────────────────────

static void mqttCallback(char *topic, byte *payload, unsigned int length) {
    if (strcmp(topic, topicCmd) != 0) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) return;

    bool changed = false;

    // Handle on/off
    if (doc.containsKey("state")) {
        const char *state = doc["state"];
        if (state != nullptr) {
            if (strcmp(state, "OFF") == 0 && cfgPtr->brightness > 0) {
                priorBrightness = cfgPtr->brightness;
                cfgPtr->brightness = 0;
                changed = true;
            } else if (strcmp(state, "ON") == 0 && cfgPtr->brightness == 0) {
                cfgPtr->brightness = priorBrightness > 0 ? priorBrightness : DEFAULT_BRIGHTNESS;
                changed = true;
            }
        }
    }

    // Handle brightness
    if (doc.containsKey("brightness")) {
        uint8_t b = doc["brightness"].as<uint8_t>();
        if (b != cfgPtr->brightness) {
            cfgPtr->brightness = b;
            if (b > 0) priorBrightness = b;
            changed = true;
        }
    }

    // Handle effect
    if (doc.containsKey("effect")) {
        const char *name = doc["effect"];
        if (name != nullptr) {
            for (int i = 0; i < EFFECT_COUNT; i++) {
                if (strcmp(name, EFFECT_NAMES[i]) == 0) {
                    if (cfgPtr->currentEffect != (Effect)i) {
                        cfgPtr->currentEffect = (Effect)i;
                        if (cfgPtr->currentEffect == EFFECT_TETRIS) {
                            resetTetris();
                        } else if (cfgPtr->currentEffect == EFFECT_SNAKE) {
                            resetSnake();
                        }
                        changed = true;
                    }
                    break;
                }
            }
        }
    }

    if (changed) {
        mqttDirty   = true;
        lastDirtyMs = millis();
        publishStateInternal();
    }
}

// ─── Connect ─────────────────────────────────────────────────────────────

static bool connectToBroker() {
    if (cfgPtr->mqtt.host[0] == '\0') return false;

    mqttClient.setServer(cfgPtr->mqtt.host, cfgPtr->mqtt.port);

    char clientId[24];
    snprintf(clientId, sizeof(clientId), "ledgrid_%s", deviceId);

    bool ok;
    if (cfgPtr->mqtt.username[0] != '\0') {
        ok = mqttClient.connect(clientId,
                                cfgPtr->mqtt.username,
                                cfgPtr->mqtt.password,
                                topicAvail, 1, true, "offline");
    } else {
        ok = mqttClient.connect(clientId,
                                NULL, NULL,
                                topicAvail, 1, true, "offline");
    }

    if (ok) {
        Serial.println(F("MQTT: Connected"));
        reconnectInterval = 5000;

        mqttClient.publish(topicAvail, "online", true);
        mqttClient.subscribe(topicCmd, 1);

        publishDiscovery();
        forcePublish = true;
        publishStateInternal();
        publishDiagnostics();
    } else {
        Serial.printf("MQTT: Connect failed (rc=%d)\n", mqttClient.state());
    }
    return ok;
}

// ─── Public API ──────────────────────────────────────────────────────────

void setupMqtt(GridConfig &cfg) {
    cfgPtr = &cfg;
    buildDeviceId();
    buildTopics();

    reconnectInterval = 5000;
    lastReconnectMs   = 0;

    mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
    mqttClient.setCallback(mqttCallback);

    priorBrightness = cfg.brightness > 0 ? cfg.brightness : DEFAULT_BRIGHTNESS;

    if (cfg.mqtt.enabled && cfg.mqtt.host[0] != '\0') {
        Serial.printf("MQTT: Connecting to %s:%d\n", cfg.mqtt.host, cfg.mqtt.port);
        connectToBroker();
    } else {
        Serial.println(F("MQTT: Disabled"));
    }
}

void loopMqtt() {
    if (!cfgPtr || !cfgPtr->mqtt.enabled) return;

    if (mqttClient.connected()) {
        mqttClient.loop();

        unsigned long now = millis();

        // Heartbeat — re-publish state + diagnostics periodically
        if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL) {
            lastHeartbeatMs = now;
            forcePublish = true;
            publishStateInternal();
            publishDiagnostics();
        }

        // Debounced save for MQTT-driven changes (avoid NVS wear)
        if (mqttDirty && (now - lastDirtyMs > 5000)) {
            saveGridConfig(*cfgPtr);
            mqttDirty = false;
        }
        return;
    }

    // Not connected — attempt reconnect with backoff
    if (cfgPtr->mqtt.host[0] == '\0') return;

    unsigned long now = millis();
    if (now - lastReconnectMs < reconnectInterval) return;
    lastReconnectMs = now;

    Serial.printf("MQTT: Reconnecting (interval %lums)\n", reconnectInterval);
    if (!connectToBroker()) {
        reconnectInterval = min(reconnectInterval * 2, RECONNECT_CAP);
    }
}

void mqttPublishState() {
    publishStateInternal();
}

void mqttDisconnect() {
    if (mqttClient.connected()) {
        mqttClient.publish(topicAvail, "offline", true);
        mqttClient.disconnect();
        Serial.println(F("MQTT: Disconnected"));
    }
}

bool isMqttConnected() {
    return mqttClient.connected();
}
