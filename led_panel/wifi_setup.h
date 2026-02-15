#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <Adafruit_NeoPixel.h>
#include "config.h"

// Set up WiFi via WiFiManager captive portal.
// Animates LEDs during AP mode. Blocks until connected.
// Starts mDNS at MDNS_HOSTNAME.local.
void setupWiFi(Adafruit_NeoPixel &strip);

#endif // WIFI_SETUP_H
