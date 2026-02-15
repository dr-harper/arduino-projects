#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "config.h"

// Initialise MQTT client with current configuration.
void setupMqtt(GridConfig &cfg);

// Non-blocking MQTT loop — call from main loop().
void loopMqtt();

// Publish current state (brightness, effect, on/off) to HA.
void mqttPublishState();

// Cleanly disconnect from broker (call before applying new settings).
void mqttDisconnect();

// Returns true if connected to the MQTT broker.
bool isMqttConnected();

#endif // MQTT_CLIENT_H
