#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include "config.h"

// ─── Shared state (defined in ai_camera.ino) ───────────────────────────────

// Device configuration (persisted in NVS)
extern AiCameraConfig config;

// Camera state
extern bool     cameraReady;

// ML inference state
extern bool     mlReady;

// Capture state
extern bool     captureRequested;   // Set by button or API, cleared after capture
extern uint32_t lastCaptureMs;

// 64-bit uptime (handles millis() overflow)
extern uint64_t uptimeMs;

#endif // GLOBALS_H
