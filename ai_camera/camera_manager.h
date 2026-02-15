#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <Arduino.h>
#include "esp_camera.h"

// Initialise the OV2640 camera with PSRAM frame buffers.
// Returns true on success.
bool initCamera();

// Capture a single JPEG frame.
// Returns a camera frame buffer pointer — caller MUST call releaseFrame() when done.
// Returns nullptr on failure.
camera_fb_t* captureFrame();

// Release a frame buffer back to the camera driver.
void releaseFrame(camera_fb_t *fb);

// Temporarily switch to a different frame size (e.g. for streaming vs capture).
// Returns true on success.
bool setCameraFrameSize(framesize_t size);

// Temporarily switch JPEG quality.
void setCameraJpegQuality(uint8_t quality);

#endif // CAMERA_MANAGER_H
