#ifndef SNAKE_GAME_H
#define SNAKE_GAME_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Run one frame of the Snake game. Call at LED_UPDATE_INTERVAL_MS.
void updateSnake(Adafruit_NeoPixel &strip);

// Reset the Snake board (called when switching to this effect).
void resetSnake();

// Apply runtime configuration (background colour, etc.)
void setSnakeConfig(const GridConfig &cfg);

// ─── Manual Control ────────────────────────────────────────────────────────
// Direction: 0=up, 1=right, 2=down, 3=left
void snakeSetDirection(uint8_t dir);
void setSnakeManualMode(bool enabled);
bool isSnakeManualMode();

// ─── State Query (for WebSocket broadcast) ─────────────────────────────────
void getSnakeState(uint32_t *gridOut, uint16_t &score, uint16_t &length,
                   bool &over);

#endif // SNAKE_GAME_H
