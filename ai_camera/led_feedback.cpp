#include "led_feedback.h"
#include "config.h"

static LedPattern currentPattern = LedPattern::OFF;
static unsigned long patternStartMs = 0;
static unsigned long lastToggleMs = 0;
static bool ledState = false;

// XIAO ESP32S3 onboard LED is active LOW on some variants — adjust if needed
static const bool LED_ACTIVE_LOW = false;

static void setLedRaw(bool on) {
    ledState = on;
    digitalWrite(LED_PIN, (on ^ LED_ACTIVE_LOW) ? HIGH : LOW);
}

void initLedFeedback() {
    pinMode(LED_PIN, OUTPUT);
    setLedRaw(false);
}

void setLedPattern(LedPattern pattern) {
    if (pattern == currentPattern) return;
    currentPattern = pattern;
    patternStartMs = millis();
    lastToggleMs = millis();

    // Immediate state for simple patterns
    if (pattern == LedPattern::OFF) {
        setLedRaw(false);
    } else if (pattern == LedPattern::SOLID) {
        setLedRaw(true);
    } else if (pattern == LedPattern::FLASH_ONCE) {
        setLedRaw(true);
    }
}

void updateLed() {
    unsigned long now = millis();

    switch (currentPattern) {
        case LedPattern::OFF:
        case LedPattern::SOLID:
            // Static — nothing to update
            break;

        case LedPattern::BLINK_SLOW: {
            // 1Hz: 500ms on, 500ms off
            if (now - lastToggleMs >= 500) {
                lastToggleMs = now;
                setLedRaw(!ledState);
            }
            break;
        }

        case LedPattern::BLINK_FAST: {
            // 4Hz: 125ms on, 125ms off
            if (now - lastToggleMs >= 125) {
                lastToggleMs = now;
                setLedRaw(!ledState);
            }
            break;
        }

        case LedPattern::FLASH_ONCE: {
            // Single 200ms flash then off
            if (now - patternStartMs >= 200) {
                setLedRaw(false);
                currentPattern = LedPattern::OFF;
            }
            break;
        }

        case LedPattern::PULSE: {
            // Soft pulse using PWM-like toggling (approximate with on/off timing)
            // Full cycle ~2 seconds: ramp up 1s, ramp down 1s
            unsigned long phase = (now - patternStartMs) % 2000;
            // Approximate brightness with variable duty cycle
            unsigned long onTime = (phase < 1000) ? (phase / 10) : ((2000 - phase) / 10);
            unsigned long cyclePos = (now - lastToggleMs);
            if (ledState && cyclePos >= onTime) {
                setLedRaw(false);
                lastToggleMs = now;
            } else if (!ledState && cyclePos >= (100 - onTime)) {
                setLedRaw(true);
                lastToggleMs = now;
            }
            break;
        }
    }
}
