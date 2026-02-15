#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include "config.h"

// ─── Shared state (defined in night_light.ino) ─────────────────────────────

// Manual override ("Pretty mode")
extern bool     manualOverride;
extern uint8_t  overrideBrightness;
extern uint8_t  overrideEffect;
extern uint8_t  overrideR, overrideG, overrideB, overrideW;

// Schedule tracking
extern int8_t   currentSlotIndex;
extern int8_t   previousSlotIndex;

// Independent user brightness
extern bool     userBrightnessActive;
extern uint8_t  userBrightness;

// Special date state
extern int8_t   currentSpecialDateIndex;
extern bool     specialDateOverrideActive;

// Thermal monitoring
extern float    chipTempC;
extern float    thermalDimFactor;
extern bool     thermalShutdown;

// Master fade (smooth on/off toggle)
extern float    masterFadeFactor;

// 64-bit uptime (handles millis() overflow)
extern uint64_t uptimeMs;

#endif // GLOBALS_H
