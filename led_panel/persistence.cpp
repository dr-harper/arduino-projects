#include "persistence.h"
#include <Preferences.h>

#define NVS_NAMESPACE "ledgrid"

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

    cfg.currentEffect = DEFAULT_EFFECT;
    cfg.manualMode    = false;

    strncpy(cfg.authPassword, DEFAULT_AUTH_PASS, sizeof(cfg.authPassword) - 1);
    cfg.authPassword[sizeof(cfg.authPassword) - 1] = '\0';
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

    cfg.currentEffect  = (Effect)p.getUChar("effect", (uint8_t)cfg.currentEffect);
    if (cfg.currentEffect >= EFFECT_COUNT) cfg.currentEffect = DEFAULT_EFFECT;

    String pw = p.getString("authPass", cfg.authPassword);
    strncpy(cfg.authPassword, pw.c_str(), sizeof(cfg.authPassword) - 1);
    cfg.authPassword[sizeof(cfg.authPassword) - 1] = '\0';

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

    p.putUChar("effect", (uint8_t)cfg.currentEffect);

    p.putString("authPass", cfg.authPassword);

    p.end();
}
