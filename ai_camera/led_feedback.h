#ifndef LED_FEEDBACK_H
#define LED_FEEDBACK_H

#include <Arduino.h>

// LED pattern types for visual feedback
enum class LedPattern : uint8_t {
    OFF,            // LED off
    SOLID,          // LED constantly on
    BLINK_SLOW,     // 1Hz blink (WiFi connecting)
    BLINK_FAST,     // 4Hz blink (capturing/classifying)
    FLASH_ONCE,     // Single 200ms flash (capture complete)
    PULSE,          // Gradual on/off (idle, ready)
};

// Initialise the LED feedback system.
void initLedFeedback();

// Set the current LED pattern. Call from setup/loop context.
void setLedPattern(LedPattern pattern);

// Update the LED state. Call from loop() — non-blocking, uses millis().
void updateLed();

#endif // LED_FEEDBACK_H
