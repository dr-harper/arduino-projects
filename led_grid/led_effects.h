#ifndef LED_EFFECTS_H
#define LED_EFFECTS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Initialise the LED strip.
void initLeds(Adafruit_NeoPixel &strip);

// Run one frame of the current effect. Call from loop() at LED_UPDATE_INTERVAL_MS.
void updateEffect(Adafruit_NeoPixel &strip, Effect effect);

// Convert (x, y) grid coordinates to the linear LED index,
// accounting for serpentine wiring.
uint16_t xyToIndex(uint8_t x, uint8_t y);

// HSV-like colour wheel: 0-255 maps to full hue rotation (R→G→B→R)
uint32_t colourWheel(uint8_t pos);

// Cycle to the next effect. Returns the new effect.
Effect nextEffect(Effect current);

// Set clock to 24-hour (true) or 12-hour (false) display.
void setClockUse24Hour(bool use24h);

// Configure clock digit transition: 0 = none, 1 = crossfade.
void setClockTransition(uint8_t mode);

// Set crossfade duration in milliseconds (200-1000).
void setClockFadeMs(uint16_t ms);

// Show/hide minute position marker on clock border.
void setClockMinMarker(bool show);

// Set clock digit colour preset (0=warm white, 1=cool white, 2=amber, 3=green, 4=blue, 5=red).
void setClockDigitColour(uint8_t preset);

// Show/hide the orbiting second trail on clock border.
void setClockTrail(bool show);

#endif // LED_EFFECTS_H
