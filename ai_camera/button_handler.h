#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

// Initialise the button GPIO with internal pull-up and interrupt.
void initButton();

// Check if the button was pressed since last call.
// Returns true once per press (edge-triggered, debounced).
bool buttonPressed();

#endif // BUTTON_HANDLER_H
