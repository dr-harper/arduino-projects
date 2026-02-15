#ifndef LED_EFFECTS_H
#define LED_EFFECTS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Apply a single effect to the strip with the given colour and brightness.
void applyEffect(Adafruit_NeoPixel &strip, EffectType effect,
                 uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                 uint8_t brightness, unsigned long now);

// Crossfade between two schedule slots.  progress: 0.0 → 1.0
// brightnessOverride: if >= 0, use this brightness instead of lerping between slots.
void applyTransition(Adafruit_NeoPixel &strip,
                     const ScheduleSlot &from, const ScheduleSlot &to,
                     float progress, unsigned long now,
                     int16_t brightnessOverride = -1);

// Individual effect functions (public for direct testing if needed)
void effectSolid(Adafruit_NeoPixel &strip,
                 uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t brightness);

void effectBreathing(Adafruit_NeoPixel &strip,
                     uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                     uint8_t brightness, unsigned long now);

void effectSoftGlow(Adafruit_NeoPixel &strip,
                    uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                    uint8_t brightness, unsigned long now);

void effectStarryTwinkle(Adafruit_NeoPixel &strip,
                         uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                         uint8_t brightness, unsigned long now);

void effectRainbow(Adafruit_NeoPixel &strip,
                   uint8_t brightness, unsigned long now);

void effectCandleFlicker(Adafruit_NeoPixel &strip,
                         uint8_t w, uint8_t brightness, unsigned long now);

void effectSunrise(Adafruit_NeoPixel &strip,
                   uint8_t w, uint8_t brightness, unsigned long now);

// Holiday/birthday effects
void effectChristmas(Adafruit_NeoPixel &strip,
                     uint8_t brightness, unsigned long now);

void effectHalloween(Adafruit_NeoPixel &strip,
                     uint8_t brightness, unsigned long now);

void effectBirthday(Adafruit_NeoPixel &strip,
                    uint8_t brightness, unsigned long now);

// Helper: colour wheel (0–255 → packed RGBW colour)
uint32_t wheel(Adafruit_NeoPixel &strip, uint8_t pos);

// Helper: extract raw RGB from colour wheel position (no packing)
void wheelRGB(uint8_t pos, uint8_t &r, uint8_t &g, uint8_t &b);

// Blocking boot animation — rainbow wave with brightness ramp-up.
// Call once during setup() before WiFi/NTP.
void playBootAnimation(Adafruit_NeoPixel &strip, unsigned long durationMs);

#endif // LED_EFFECTS_H
