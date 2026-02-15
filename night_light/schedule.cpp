#include "schedule.h"
#include <Preferences.h>

static Preferences prefs;

// ─── Defaults ───────────────────────────────────────────────────────────────
void resetScheduleDefaults(NightLightConfig &cfg) {
    memset(&cfg, 0, sizeof(NightLightConfig));

    cfg.numSlots          = 3;
    strncpy(cfg.timezone, DEFAULT_TIMEZONE, sizeof(cfg.timezone) - 1);
    cfg.timezone[sizeof(cfg.timezone) - 1] = '\0';
    cfg.gmtOffsetSec      = DEFAULT_GMT_OFFSET_SEC;
    cfg.dstOffsetSec      = DEFAULT_DST_OFFSET_SEC;
    cfg.transitionMinutes = DEFAULT_TRANSITION_MINUTES;
    cfg.ledsEnabled       = true;
    strncpy(cfg.authPassword, DEFAULT_AUTH_PASSWORD, sizeof(cfg.authPassword) - 1);

    // Special dates defaults — none configured
    cfg.numSpecialDates = 0;
    memset(cfg.specialDates, 0, sizeof(cfg.specialDates));

    // MQTT defaults — disabled until configured
    cfg.mqtt.enabled = false;
    cfg.mqtt.host[0] = '\0';
    cfg.mqtt.port    = MQTT_DEFAULT_PORT;
    cfg.mqtt.username[0] = '\0';
    cfg.mqtt.password[0] = '\0';

    // Slot 0 — Bedtime: warm amber, breathing
    ScheduleSlot &bed = cfg.slots[0];
    bed.enabled     = true;
    bed.startHour   = 19;  bed.startMinute = 0;
    bed.endHour     = 20;  bed.endMinute   = 0;
    bed.red         = 255; bed.green = 140; bed.blue = 20;
    bed.white       = 60;
    bed.brightness  = 102; // ~40%
    bed.effect      = EFFECT_BREATHING;
    strncpy(bed.label, "Bedtime", sizeof(bed.label) - 1);

    // Slot 1 — Sleep: dim red, solid
    ScheduleSlot &sleep = cfg.slots[1];
    sleep.enabled     = true;
    sleep.startHour   = 20; sleep.startMinute = 0;
    sleep.endHour     = 6;  sleep.endMinute   = 0;
    sleep.red         = 180; sleep.green = 10; sleep.blue = 0;
    sleep.white       = 0;
    sleep.brightness  = 25;  // ~10%
    sleep.effect      = EFFECT_SOLID;
    strncpy(sleep.label, "Sleep time", sizeof(sleep.label) - 1);

    // Slot 2 — OK to wake: soft green, soft glow
    ScheduleSlot &wake = cfg.slots[2];
    wake.enabled     = true;
    wake.startHour   = 6;  wake.startMinute = 0;
    wake.endHour     = 7;  wake.endMinute   = 0;
    wake.red         = 20; wake.green = 200; wake.blue = 60;
    wake.white       = 20;
    wake.brightness  = 76; // ~30%
    wake.effect      = EFFECT_SOFT_GLOW;
    strncpy(wake.label, "OK to wake", sizeof(wake.label) - 1);
}

// ─── Slot-only reset (preserves MQTT, timezone, auth, etc.) ────────────────
void resetSlotDefaults(NightLightConfig &cfg) {
    memset(cfg.slots, 0, sizeof(cfg.slots));
    cfg.numSlots = 3;

    // Slot 0 — Bedtime
    ScheduleSlot &bed = cfg.slots[0];
    bed.enabled     = true;
    bed.startHour   = 19;  bed.startMinute = 0;
    bed.endHour     = 20;  bed.endMinute   = 0;
    bed.red         = 255; bed.green = 140; bed.blue = 20;
    bed.white       = 60;
    bed.brightness  = 102;
    bed.effect      = EFFECT_BREATHING;
    strncpy(bed.label, "Bedtime", sizeof(bed.label) - 1);

    // Slot 1 — Sleep
    ScheduleSlot &sleep = cfg.slots[1];
    sleep.enabled     = true;
    sleep.startHour   = 20; sleep.startMinute = 0;
    sleep.endHour     = 6;  sleep.endMinute   = 0;
    sleep.red         = 180; sleep.green = 10; sleep.blue = 0;
    sleep.white       = 0;
    sleep.brightness  = 25;
    sleep.effect      = EFFECT_SOLID;
    strncpy(sleep.label, "Sleep time", sizeof(sleep.label) - 1);

    // Slot 2 — OK to wake
    ScheduleSlot &wake = cfg.slots[2];
    wake.enabled     = true;
    wake.startHour   = 6;  wake.startMinute = 0;
    wake.endHour     = 7;  wake.endMinute   = 0;
    wake.red         = 20; wake.green = 200; wake.blue = 60;
    wake.white       = 20;
    wake.brightness  = 76;
    wake.effect      = EFFECT_SOFT_GLOW;
    strncpy(wake.label, "OK to wake", sizeof(wake.label) - 1);

    Serial.println(F("Slots reset to defaults (settings preserved)"));
}

// ─── Load from NVS ──────────────────────────────────────────────────────────
void loadSchedule(NightLightConfig &cfg) {
    prefs.begin("nightlight", true);  // read-only

    if (!prefs.isKey("numSlots")) {
        prefs.end();
        Serial.println(F("No saved schedule — loading defaults"));
        resetScheduleDefaults(cfg);
        return;
    }

    cfg.numSlots          = prefs.getUChar("numSlots", 3);
    cfg.transitionMinutes = prefs.getUChar("transition", DEFAULT_TRANSITION_MINUTES);
    cfg.ledsEnabled       = prefs.getBool("ledsOn", true);

    // Timezone — migrate from legacy gmtOffset/dstOffset if no TZ string saved
    String tz = prefs.getString("timezone", "");
    if (tz.length() > 0) {
        strncpy(cfg.timezone, tz.c_str(), sizeof(cfg.timezone) - 1);
    } else {
        strncpy(cfg.timezone, DEFAULT_TIMEZONE, sizeof(cfg.timezone) - 1);
    }
    cfg.timezone[sizeof(cfg.timezone) - 1] = '\0';

    // Legacy fields (kept for struct compatibility)
    cfg.gmtOffsetSec      = prefs.getShort("gmtOffset", DEFAULT_GMT_OFFSET_SEC);
    cfg.dstOffsetSec      = prefs.getShort("dstOffset", DEFAULT_DST_OFFSET_SEC);

    String pass = prefs.getString("authPass", DEFAULT_AUTH_PASSWORD);
    strncpy(cfg.authPassword, pass.c_str(), sizeof(cfg.authPassword) - 1);
    cfg.authPassword[sizeof(cfg.authPassword) - 1] = '\0';

    // MQTT settings
    cfg.mqtt.enabled = prefs.getBool("mqttOn", false);
    cfg.mqtt.port    = prefs.getUShort("mqttPort", MQTT_DEFAULT_PORT);

    String mqttHost = prefs.getString("mqttHost", "");
    strncpy(cfg.mqtt.host, mqttHost.c_str(), sizeof(cfg.mqtt.host) - 1);
    cfg.mqtt.host[sizeof(cfg.mqtt.host) - 1] = '\0';

    String mqttUser = prefs.getString("mqttUser", "");
    strncpy(cfg.mqtt.username, mqttUser.c_str(), sizeof(cfg.mqtt.username) - 1);
    cfg.mqtt.username[sizeof(cfg.mqtt.username) - 1] = '\0';

    String mqttPass = prefs.getString("mqttPass", "");
    strncpy(cfg.mqtt.password, mqttPass.c_str(), sizeof(cfg.mqtt.password) - 1);
    cfg.mqtt.password[sizeof(cfg.mqtt.password) - 1] = '\0';

    // Special dates
    cfg.numSpecialDates = prefs.getUChar("numSpecDates", 0);
    for (uint8_t i = 0; i < cfg.numSpecialDates && i < MAX_SPECIAL_DATES; i++) {
        char key[12];
        snprintf(key, sizeof(key), "specDate%u", i);
        prefs.getBytes(key, &cfg.specialDates[i], sizeof(SpecialDate));
    }

    for (uint8_t i = 0; i < cfg.numSlots && i < MAX_SCHEDULE_SLOTS; i++) {
        char key[12];
        snprintf(key, sizeof(key), "slot%u", i);
        size_t len = prefs.getBytes(key, &cfg.slots[i], sizeof(ScheduleSlot));
        if (len != sizeof(ScheduleSlot)) {
            Serial.print(F("Warning: slot "));
            Serial.print(i);
            Serial.println(F(" size mismatch — resetting slots only (preserving settings)"));
            prefs.end();
            // Only reset slots to defaults; preserve MQTT, timezone, auth, etc.
            resetSlotDefaults(cfg);
            saveSchedule(cfg);
            return;
        }
    }

    prefs.end();
    Serial.println(F("Schedule loaded from NVS"));
}

// ─── Save to NVS ────────────────────────────────────────────────────────────
void saveSchedule(const NightLightConfig &cfg) {
    prefs.begin("nightlight", false);  // read-write

    prefs.putUChar("numSlots", cfg.numSlots);
    prefs.putString("timezone", cfg.timezone);
    prefs.putUChar("transition", cfg.transitionMinutes);
    prefs.putBool("ledsOn", cfg.ledsEnabled);
    prefs.putString("authPass", cfg.authPassword);

    // Special dates
    prefs.putUChar("numSpecDates", cfg.numSpecialDates);
    for (uint8_t i = 0; i < cfg.numSpecialDates && i < MAX_SPECIAL_DATES; i++) {
        char key[12];
        snprintf(key, sizeof(key), "specDate%u", i);
        prefs.putBytes(key, &cfg.specialDates[i], sizeof(SpecialDate));
    }
    // Clear leftover special dates
    for (uint8_t i = cfg.numSpecialDates; i < MAX_SPECIAL_DATES; i++) {
        char key[12];
        snprintf(key, sizeof(key), "specDate%u", i);
        if (prefs.isKey(key)) {
            prefs.remove(key);
        }
    }

    // MQTT settings
    prefs.putBool("mqttOn", cfg.mqtt.enabled);
    prefs.putUShort("mqttPort", cfg.mqtt.port);
    prefs.putString("mqttHost", cfg.mqtt.host);
    prefs.putString("mqttUser", cfg.mqtt.username);
    prefs.putString("mqttPass", cfg.mqtt.password);

    for (uint8_t i = 0; i < cfg.numSlots && i < MAX_SCHEDULE_SLOTS; i++) {
        char key[12];
        snprintf(key, sizeof(key), "slot%u", i);
        prefs.putBytes(key, &cfg.slots[i], sizeof(ScheduleSlot));
    }

    // Clear any leftover slots from a previous larger schedule
    for (uint8_t i = cfg.numSlots; i < MAX_SCHEDULE_SLOTS; i++) {
        char key[12];
        snprintf(key, sizeof(key), "slot%u", i);
        if (prefs.isKey(key)) {
            prefs.remove(key);
        }
    }

    prefs.end();
    Serial.println(F("Schedule saved to NVS"));
}

// ─── Time helpers ───────────────────────────────────────────────────────────
uint16_t timeToMinutes(uint8_t hour, uint8_t minute) {
    return (uint16_t)hour * 60 + minute;
}

// ─── Evaluate schedule ──────────────────────────────────────────────────────
int8_t evaluateSchedule(const NightLightConfig &cfg, int hour, int minute) {
    uint16_t nowMins = timeToMinutes((uint8_t)hour, (uint8_t)minute);

    for (uint8_t i = 0; i < cfg.numSlots && i < MAX_SCHEDULE_SLOTS; i++) {
        const ScheduleSlot &slot = cfg.slots[i];
        if (!slot.enabled) continue;

        uint16_t startMins = timeToMinutes(slot.startHour, slot.startMinute);
        uint16_t endMins   = timeToMinutes(slot.endHour, slot.endMinute);

        // Skip zero-duration slots (start == end) — likely misconfigured
        if (startMins == endMins) continue;

        if (startMins < endMins) {
            // Normal range: e.g. 19:00 – 20:00
            if (nowMins >= startMins && nowMins < endMins) {
                return (int8_t)i;
            }
        } else {
            // Overnight range: e.g. 20:00 – 06:00
            if (nowMins >= startMins || nowMins < endMins) {
                return (int8_t)i;
            }
        }
    }

    return -1;  // No matching slot
}

// ─── Evaluate special dates ────────────────────────────────────────────────
int8_t evaluateSpecialDates(const NightLightConfig &cfg, uint8_t month, uint8_t day) {
    for (uint8_t i = 0; i < cfg.numSpecialDates && i < MAX_SPECIAL_DATES; i++) {
        const SpecialDate &sd = cfg.specialDates[i];
        if (sd.enabled && sd.month == month && sd.day == day) {
            return (int8_t)i;
        }
    }
    return -1;
}
