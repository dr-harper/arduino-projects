#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include "config.h"

// Load configuration from NVS (or defaults if first boot).
void loadConfig(AiCameraConfig &cfg);

// Save configuration to NVS.
void saveConfig(const AiCameraConfig &cfg);

// Initialise web server routes and start listening on port 80.
void setupWebServer(AiCameraConfig &cfg);

// Call from loop() to service incoming HTTP requests.
void handleWebClient();

#endif // WEB_SERVER_H
