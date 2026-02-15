#include "persistence.h"
#include <Preferences.h>

#define NVS_NAMESPACE "ledgrid"

const char* const EFFECT_NAMES[] = {
    "Tetris", "Rainbow Wave", "Colour Wash", "Diagonal Rainbow",
    "Rain", "Clock", "Fire", "Aurora",
    "Lava", "Candle", "Twinkle", "Matrix",
    "Fireworks", "Life", "Plasma", "Spiral",
    "Valentines", "Snake"
};
static_assert(sizeof(EFFECT_NAMES) / sizeof(EFFECT_NAMES[0]) == EFFECT_COUNT,
              "EFFECT_NAMES must match Effect enum count");

void initDefaultConfig(GridConfig &cfg) {
    cfg.brightness    = DEFAULT_BRIGHTNESS;
    cfg.bgR           = 0;
    cfg.bgG           = 0;
    cfg.bgB           = 0;

    cfg.dropStartMs   = 300;
    cfg.dropMinMs     = 80;
    cfg.moveIntervalMs = 70;
    cfg.rotIntervalMs = 150;

    cfg.aiSkillPct    = 90;
    cfg.jitterPct     = 20;

    cfg.use24Hour     = true;
    cfg.clockTransition = 1;     // crossfade on by default
    cfg.clockFadeMs     = 500;
    cfg.clockMinMarker  = true;
    cfg.clockDigitColour = 0;    // warm white
    cfg.clockTrail      = true;

    cfg.currentEffect = EFFECT_TETRIS;
    cfg.manualMode    = false;

    strncpy(cfg.authPassword, DEFAULT_AUTH_PASS, sizeof(cfg.authPassword) - 1);
    cfg.authPassword[sizeof(cfg.authPassword) - 1] = '\0';

    cfg.mqtt.enabled  = false;
    cfg.mqtt.host[0]  = '\0';
    cfg.mqtt.port     = MQTT_DEFAULT_PORT;
    cfg.mqtt.username[0] = '\0';
    cfg.mqtt.password[0] = '\0';
}

void loadGridConfig(GridConfig &cfg) {
    initDefaultConfig(cfg);

    Preferences p;
    if (!p.begin(NVS_NAMESPACE, true)) return;

    cfg.brightness     = p.getUChar("brightness", cfg.brightness);
    cfg.bgR            = p.getUChar("bgR", cfg.bgR);
    cfg.bgG            = p.getUChar("bgG", cfg.bgG);
    cfg.bgB            = p.getUChar("bgB", cfg.bgB);

    cfg.dropStartMs    = p.getUShort("dropStart", cfg.dropStartMs);
    cfg.dropMinMs      = p.getUShort("dropMin", cfg.dropMinMs);
    cfg.moveIntervalMs = p.getUShort("moveInt", cfg.moveIntervalMs);
    cfg.rotIntervalMs  = p.getUShort("rotInt", cfg.rotIntervalMs);

    cfg.aiSkillPct     = p.getUChar("aiSkill", cfg.aiSkillPct);
    cfg.jitterPct      = p.getUChar("jitter", cfg.jitterPct);

    cfg.use24Hour      = p.getBool("use24Hour", cfg.use24Hour);
    cfg.clockTransition = p.getUChar("clkTrans", cfg.clockTransition);
    cfg.clockFadeMs     = p.getUShort("clkFadeMs", cfg.clockFadeMs);
    cfg.clockMinMarker  = p.getBool("clkMinMark", cfg.clockMinMarker);
    cfg.clockDigitColour = p.getUChar("clkDigCol", cfg.clockDigitColour);
    cfg.clockTrail      = p.getBool("clkTrail", cfg.clockTrail);

    cfg.currentEffect  = (Effect)p.getUChar("effect", (uint8_t)cfg.currentEffect);
    if (cfg.currentEffect >= EFFECT_COUNT) cfg.currentEffect = EFFECT_TETRIS;

    String pw = p.getString("authPass", cfg.authPassword);
    strncpy(cfg.authPassword, pw.c_str(), sizeof(cfg.authPassword) - 1);
    cfg.authPassword[sizeof(cfg.authPassword) - 1] = '\0';

    cfg.mqtt.enabled = p.getBool("mqttOn", cfg.mqtt.enabled);
    String mh = p.getString("mqttHost", "");
    if (mh.length() > 0) {
        strncpy(cfg.mqtt.host, mh.c_str(), sizeof(cfg.mqtt.host) - 1);
        cfg.mqtt.host[sizeof(cfg.mqtt.host) - 1] = '\0';
    }
    cfg.mqtt.port = p.getUShort("mqttPort", cfg.mqtt.port);
    String mu = p.getString("mqttUser", "");
    if (mu.length() > 0) {
        strncpy(cfg.mqtt.username, mu.c_str(), sizeof(cfg.mqtt.username) - 1);
        cfg.mqtt.username[sizeof(cfg.mqtt.username) - 1] = '\0';
    }
    String mp = p.getString("mqttPass", "");
    if (mp.length() > 0) {
        strncpy(cfg.mqtt.password, mp.c_str(), sizeof(cfg.mqtt.password) - 1);
        cfg.mqtt.password[sizeof(cfg.mqtt.password) - 1] = '\0';
    }

    p.end();
}

void saveGridConfig(const GridConfig &cfg) {
    Preferences p;
    if (!p.begin(NVS_NAMESPACE, false)) return;

    p.putUChar("brightness", cfg.brightness);
    p.putUChar("bgR", cfg.bgR);
    p.putUChar("bgG", cfg.bgG);
    p.putUChar("bgB", cfg.bgB);

    p.putUShort("dropStart", cfg.dropStartMs);
    p.putUShort("dropMin", cfg.dropMinMs);
    p.putUShort("moveInt", cfg.moveIntervalMs);
    p.putUShort("rotInt", cfg.rotIntervalMs);

    p.putUChar("aiSkill", cfg.aiSkillPct);
    p.putUChar("jitter", cfg.jitterPct);

    p.putBool("use24Hour", cfg.use24Hour);
    p.putUChar("clkTrans", cfg.clockTransition);
    p.putUShort("clkFadeMs", cfg.clockFadeMs);
    p.putBool("clkMinMark", cfg.clockMinMarker);
    p.putUChar("clkDigCol", cfg.clockDigitColour);
    p.putBool("clkTrail", cfg.clockTrail);

    p.putUChar("effect", (uint8_t)cfg.currentEffect);

    p.putString("authPass", cfg.authPassword);

    p.putBool("mqttOn",     cfg.mqtt.enabled);
    p.putString("mqttHost", cfg.mqtt.host);
    p.putUShort("mqttPort", cfg.mqtt.port);
    p.putString("mqttUser", cfg.mqtt.username);
    p.putString("mqttPass", cfg.mqtt.password);

    p.end();
}
