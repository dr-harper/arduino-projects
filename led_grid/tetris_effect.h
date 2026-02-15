#ifndef TETRIS_EFFECT_H
#define TETRIS_EFFECT_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Run one frame of the Tetris animation. Call at LED_UPDATE_INTERVAL_MS.
void updateTetris(Adafruit_NeoPixel &strip);

// Reset the Tetris board (called when switching to this effect).
void resetTetris();

// Apply runtime configuration (speed, AI skill, background colour, etc.)
void setTetrisConfig(const GridConfig &cfg);

// ─── Manual Control ────────────────────────────────────────────────────────
void setManualMode(bool enabled);
bool isManualMode();
bool manualMoveLeft();
bool manualMoveRight();
bool manualRotate();
void manualHardDrop();
void manualSoftDrop(bool active);

// ─── State Query (for WebSocket broadcast) ─────────────────────────────────
void getTetrisState(uint32_t *gridOut, uint8_t &pType, int8_t &px, int8_t &py,
                    uint8_t &rot, uint16_t &score, uint16_t &lines,
                    bool &over, bool &clr);

#endif // TETRIS_EFFECT_H
