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

#endif // LED_EFFECTS_H
