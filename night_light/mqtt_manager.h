#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Initialise MQTT client with a reference to the shared config.
// Call once from setup() after WiFi is connected.
void setupMqtt(NightLightConfig &cfg);

// Service MQTT connection and incoming messages.
// Call every iteration of loop().
void loopMqtt();

// Publish the current light state (on/off, brightness, colour, effect).
// Call whenever the LED state changes (slot transition, override, web UI).
void mqttPublishLightState();

// Re-publish HA discovery for the slot select entity (options may have changed)
// and broadcast the current slot name.  Call after schedule save or reset.
void mqttPublishScheduleChanged();

// Disconnect cleanly from the broker (e.g. before reconnecting with new settings).
void mqttDisconnect();

// Returns true if the MQTT client is currently connected to the broker.
bool isMqttConnected();

#endif // MQTT_MANAGER_H
