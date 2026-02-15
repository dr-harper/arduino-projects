#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define FW_VERSION "0.5.0"

// ─── Hardware ───────────────────────────────────────────────────────────────
#define LED_PIN         4        // GPIO4 — data pin for the LED grid
#define GRID_WIDTH      16
#define GRID_HEIGHT     16
#define NUM_LEDS        (GRID_WIDTH * GRID_HEIGHT)  // 256

// LED strip type (WS2812B is most common for grids)
#define LED_TYPE        (NEO_GRB + NEO_KHZ800)

// ─── Defaults ─────────────────────────────────────────────────────────────
#define DEFAULT_BRIGHTNESS  40   // 0-255 — keep moderate to limit current draw

// ─── Animation Timing ──────────────────────────────────────────────────────
#define LED_UPDATE_INTERVAL_MS  30   // ~33 FPS
#define RAINBOW_CYCLE_MS      10000  // Full rainbow rotation period
#define COLOUR_WASH_CYCLE_MS   5000  // Solid hue sweep period

// ─── Grid Layout ───────────────────────────────────────────────────────────
#define SERPENTINE_LAYOUT  true

// ─── WiFi / Network ────────────────────────────────────────────────────────
#define MDNS_HOSTNAME   "tetris"       // http://tetris.local
#define AP_NAME         "Tetris-Setup"
#define AP_PASS         "tetris123"
#define DEFAULT_AUTH_PASS "tetris"

// ─── Effect Enum ───────────────────────────────────────────────────────────
enum Effect : uint8_t {
    EFFECT_TETRIS,           // Tetris simulation — pieces fall and stack
    EFFECT_RAINBOW_WAVE,     // Rainbow ripple across the grid
    EFFECT_COLOUR_WASH,      // Entire grid fades through hues
    EFFECT_DIAGONAL_RAINBOW, // Rainbow bands on the diagonal
    EFFECT_RAIN,             // Coloured drops falling down columns
    EFFECT_CLOCK,            // Digital clock — hours and minutes via NTP
    EFFECT_FIRE,             // Heat-based fire simulation
    EFFECT_AURORA,           // Flowing green/purple northern lights
    EFFECT_LAVA,             // Slow drifting metaball blobs
    EFFECT_CANDLE,           // Warm flickering candlelight
    EFFECT_TWINKLE,          // Random twinkling starfield
    EFFECT_MATRIX,           // Green falling code streams
    EFFECT_FIREWORKS,        // Particle launch and burst
    EFFECT_LIFE,             // Conway's Game of Life with colour
    EFFECT_PLASMA,           // Sine-wave interference patterns
    EFFECT_SPIRAL,           // Rotating colour pinwheel
    EFFECT_VALENTINES,       // Pulsing heart with sparkles
    EFFECT_SNAKE,            // Snake game — AI or manual phone control
    EFFECT_COUNT             // Sentinel — number of effects
};

// ─── MQTT ─────────────────────────────────────────────────────────────────
#define MQTT_DEFAULT_PORT    1883
#define MQTT_BUFFER_SIZE     1024

struct MqttConfig {
    bool     enabled;
    char     host[64];
    uint16_t port;
    char     username[32];
    char     password[64];
};

// ─── Effect Names (must match Effect enum order) ──────────────────────────
extern const char* const EFFECT_NAMES[];

// ─── Runtime Configuration ─────────────────────────────────────────────────
struct GridConfig {
    // Display
    uint8_t  brightness;         // 0-255
    uint8_t  bgR, bgG, bgB;     // Background colour for empty cells

    // Tetris speed
    uint16_t dropStartMs;        // Initial drop speed (100-600)
    uint16_t dropMinMs;          // Fastest drop speed (30-200)
    uint16_t moveIntervalMs;     // Horizontal slide speed (20-200)
    uint16_t rotIntervalMs;      // Rotation speed (50-300)

    // Tetris AI behaviour
    uint8_t  aiSkillPct;         // 0-100 (% optimal moves)
    uint8_t  jitterPct;          // 0-50 (% movement hesitation)

    // Clock
    bool     use24Hour;          // true = 24h, false = 12h (no AM/PM label)
    uint8_t  clockTransition;    // 0 = none, 1 = crossfade
    uint16_t clockFadeMs;        // crossfade duration in ms (200-2000)
    bool     clockMinMarker;     // show minute position marker on border
    uint8_t  clockDigitColour;   // digit colour preset (0-5)
    bool     clockTrail;         // show orbiting second trail on border

    // Current state
    Effect   currentEffect;
    bool     manualMode;         // true = phone controls Tetris

    // Auth
    char     authPassword[32];

    // MQTT
    MqttConfig mqtt;
};

#endif // CONFIG_H
