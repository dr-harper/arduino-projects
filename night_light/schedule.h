#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <Arduino.h>
#include "config.h"

// Load schedule from NVS Preferences.  Falls back to defaults if nothing saved.
void loadSchedule(NightLightConfig &cfg);

// Save the full configuration to NVS Preferences.
void saveSchedule(const NightLightConfig &cfg);

// Reset to factory defaults (in memory only — call saveSchedule to persist).
void resetScheduleDefaults(NightLightConfig &cfg);

// Reset only the schedule slots to defaults, preserving MQTT, timezone, auth, etc.
// Used when slot struct size changes (NVS migration).
void resetSlotDefaults(NightLightConfig &cfg);

// Evaluate which schedule slot is active for the given time.
// Returns slot index (0–7) or -1 if no slot matches.
int8_t evaluateSchedule(const NightLightConfig &cfg, int hour, int minute);

// Evaluate special dates for the given month/day.
// Returns matching index (0–3) or -1 if no match.
int8_t evaluateSpecialDates(const NightLightConfig &cfg, uint8_t month, uint8_t day);

// Convert a time to "minutes since midnight" for easy comparison.
uint16_t timeToMinutes(uint8_t hour, uint8_t minute);

#endif // SCHEDULE_H
