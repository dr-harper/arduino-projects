#ifndef WIFI_MANAGER_SETUP_H
#define WIFI_MANAGER_SETUP_H

#include <Arduino.h>

// Initialise WiFiManager — blocks until WiFi is connected or AP mode is active.
void setupWiFiManager();

// Configure NTP time sync with a POSIX TZ string.
void setupNTP(const char *tz);

// Reconfigure NTP with a new timezone.
void reconfigureNTP(const char *tz);

// Get current local time. Returns true if NTP is synced.
bool getCurrentTime(int &hour, int &minute);

// Get formatted time string (HH:MM:SS). Returns "--:--:--" if not synced.
String getFormattedTime();

// Get current date. Returns true if NTP is synced.
bool getCurrentDate(int &year, int &month, int &day);

// Returns true if NTP has successfully synced at least once.
bool isTimeSynced();

// Get ISO 8601 timestamp string (e.g. "2026-02-11T20:15:30").
// Returns empty string if NTP not synced.
String getIsoTimestamp();

#endif // WIFI_MANAGER_SETUP_H
