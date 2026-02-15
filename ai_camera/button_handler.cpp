#include "button_handler.h"
#include "config.h"

static volatile bool isrFlag = false;
static unsigned long lastPressMs = 0;

static void IRAM_ATTR buttonISR() {
    isrFlag = true;
}

void initButton() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
    Serial.println(F("Button initialised on GPIO0"));
}

bool buttonPressed() {
    if (!isrFlag) return false;
    isrFlag = false;

    unsigned long now = millis();
    if (now - lastPressMs < BUTTON_DEBOUNCE_MS) {
        return false;
    }
    lastPressMs = now;
    return true;
}
