#ifndef WIFI_MANAGER_SETUP_H
#define WIFI_MANAGER_SETUP_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Initialise WiFiManager — pulses LEDs while waiting for connection.
void setupWiFiManager(Adafruit_NeoPixel &strip);

// Configure NTP time sync with a POSIX TZ string
void setupNTP(const char *tz);

// Reconfigure NTP with a new timezone (called when user changes timezone)
void reconfigureNTP(const char *tz);

// Get current local time.  Returns true if NTP is synced.
bool getCurrentTime(int &hour, int &minute);

// Get a formatted time string (HH:MM:SS).  Returns "—" if NTP not synced.
String getFormattedTime();

// Get current date.  Returns true if NTP is synced.
bool getCurrentDate(int &year, int &month, int &day);

// Returns true if NTP has successfully synced at least once
bool isTimeSynced();

#endif // WIFI_MANAGER_SETUP_H
