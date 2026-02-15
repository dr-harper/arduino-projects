#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Initialise web server routes and start listening on port 80.
// Stores references to the shared config and strip objects.
void setupWebServer(NightLightConfig &cfg, Adafruit_NeoPixel &strip);

// Call from loop() to service incoming HTTP requests.
void handleWebClient();

#endif // WEB_SERVER_H
