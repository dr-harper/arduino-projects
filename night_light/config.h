#ifndef CONFIG_H
#define CONFIG_H

#define FW_VERSION "1.4.0"

// ─── Hardware ───────────────────────────────────────────────────────────────
#define LED_PIN         2        // GPIO2 on ESP32-C3
#define NUM_LEDS        55       // SK6812 RGBW strip length
#define LED_TYPE        (NEO_GRBW + NEO_KHZ800)

// ─── WiFi ───────────────────────────────────────────────────────────────────
#define AP_NAME         "NightLight-Setup"
#define AP_PASS         "nightlight"
#define AP_TIMEOUT_SEC  300      // 5 minutes before AP portal times out
#define MDNS_HOSTNAME   "nightlight"  // Accessible at http://nightlight.local

// ─── NTP ────────────────────────────────────────────────────────────────────
#define NTP_SERVER_1    "pool.ntp.org"
#define NTP_SERVER_2    "time.google.com"

// POSIX TZ string — handles automatic DST transitions.
// UK: GMT in winter, BST (GMT+1) in summer.
//   DST starts last Sunday in March at 1:00 UTC.
//   DST ends   last Sunday in October at 1:00 UTC.
#define DEFAULT_TIMEZONE  "GMT0BST,M3.5.0/1,M10.5.0"

// Legacy defaults kept for NVS migration compatibility
#define DEFAULT_GMT_OFFSET_SEC    0
#define DEFAULT_DST_OFFSET_SEC    3600

// ─── Schedule ───────────────────────────────────────────────────────────────
#define MAX_SCHEDULE_SLOTS  8
#define DEFAULT_TRANSITION_MINUTES  5

// LED effect types
enum EffectType : uint8_t {
    EFFECT_SOLID       = 0,
    EFFECT_BREATHING   = 1,
    EFFECT_SOFT_GLOW   = 2,
    EFFECT_STARRY      = 3,
    EFFECT_RAINBOW     = 4,
    EFFECT_CANDLE      = 5,
    EFFECT_SUNRISE     = 6,
    EFFECT_CHRISTMAS   = 7,
    EFFECT_HALLOWEEN   = 8,
    EFFECT_BIRTHDAY    = 9,
    EFFECT_COUNT       = 10
};

// Effect display names (for web UI)
static const char* EFFECT_NAMES[] = {
    "Solid",
    "Breathing",
    "Soft Glow",
    "Starry Twinkle",
    "Rainbow",
    "Candle Flicker",
    "Sunrise",
    "Christmas",
    "Halloween",
    "Birthday"
};

// ─── Special Dates ─────────────────────────────────────────────────────────
#define MAX_SPECIAL_DATES  4

struct SpecialDate {
    bool    enabled;
    uint8_t month;       // 1-12
    uint8_t day;         // 1-31
    uint8_t effect;      // EffectType
    uint8_t brightness;  // 0-255
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t white;
    char    label[20];   // e.g. "Mum's Birthday"
};

// ─── MQTT ───────────────────────────────────────────────────────────────────
#define MQTT_DEFAULT_PORT        1883
#define MQTT_RECONNECT_INTERVAL  5000    // ms between reconnect attempts
#define MQTT_STATE_INTERVAL      60000   // ms between periodic sensor updates
#define MQTT_BUFFER_SIZE         1024    // PubSubClient TX/RX buffer
#define MQTT_KEEPALIVE           60      // seconds
#define MQTT_BACKOFF_MAX         60000   // Max reconnect interval (exponential backoff cap)

struct MqttConfig {
    bool     enabled;
    char     host[64];
    uint16_t port;
    char     username[32];
    char     password[64];
};

// ─── Schedule Slot ──────────────────────────────────────────────────────────
struct ScheduleSlot {
    bool    enabled;
    uint8_t startHour;
    uint8_t startMinute;
    uint8_t endHour;
    uint8_t endMinute;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t white;
    uint8_t brightness;     // 0–255
    uint8_t effect;         // EffectType
    char    label[20];
};

// ─── Full Configuration ─────────────────────────────────────────────────────
struct NightLightConfig {
    ScheduleSlot slots[MAX_SCHEDULE_SLOTS];
    uint8_t  numSlots;
    char     timezone[48];        // POSIX TZ string (e.g. "GMT0BST,M3.5.0/1,M10.5.0")
    int16_t  gmtOffsetSec;        // Legacy — kept for NVS migration only
    int16_t  dstOffsetSec;        // Legacy — kept for NVS migration only
    uint8_t  transitionMinutes;   // Crossfade duration between slots
    char     authPassword[32];    // Web UI password
    bool     ledsEnabled;         // Master on/off toggle
    MqttConfig mqtt;              // MQTT broker settings
    SpecialDate specialDates[MAX_SPECIAL_DATES];
    uint8_t  numSpecialDates;
};

// ─── Authentication ─────────────────────────────────────────────────────────
#define DEFAULT_AUTH_PASSWORD  "nightlight"
#define SESSION_TOKEN_LENGTH   16

// ─── AP Mode Defaults ──────────────────────────────────────────────────────
#define AP_DEFAULT_BRIGHTNESS  128   // Default brightness when controls used in AP mode

// ─── Fallback (when NTP not synced) ─────────────────────────────────────────
#define FALLBACK_BRIGHTNESS  20
#define FALLBACK_R           255
#define FALLBACK_G           180
#define FALLBACK_B           100    // Warm white via RGB
#define FALLBACK_W           0

// ─── Default (outside all schedule slots) ──────────────────────────────────
#define DEFAULT_BRIGHTNESS   76     // ~30% — visible as a functional night light
#define DEFAULT_R            255
#define DEFAULT_G            140
#define DEFAULT_B            20     // Warm amber
#define DEFAULT_W            40     // Use the white channel for a proper warm glow

// ─── WiFi Reconnection ──────────────────────────────────────────────────────
#define WIFI_CHECK_INTERVAL_MS  30000   // Check WiFi status every 30 seconds
#define WIFI_MAX_FAILURES       10      // Reboot after 10 consecutive failures (~5 min)

// ─── Thermal Protection ─────────────────────────────────────────────────────
#define THERMAL_WARN_C          65.0f   // Start dimming LEDs above this temp (°C)
#define THERMAL_WARN_CLEAR_C    60.0f   // Clear throttling below this temp (5°C hysteresis)
#define THERMAL_CRIT_C          80.0f   // Force LEDs off above this temp (°C)
#define TEMP_READ_INTERVAL_MS   5000    // Read chip temperature every 5 seconds

// ─── LED Effect Timing ──────────────────────────────────────────────────────
#define BREATHING_CYCLE_MS   6000.0f   // 6 seconds per breath
#define GLOW_CYCLE_MS        20000.0f  // 20 seconds per glow cycle
#define TWINKLE_CHANCE       5         // % chance per LED per frame
#define CANDLE_FLICKER_MS    50        // Flicker update interval
#define LED_UPDATE_INTERVAL  30        // ms between LED updates (~33 fps)

// ─── Master Fade ───────────────────────────────────────────────────────────
#define FADE_DURATION_MS     1000      // Smooth fade when toggling LEDs on/off

#endif // CONFIG_H
