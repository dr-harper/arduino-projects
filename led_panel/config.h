#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define FW_VERSION "0.1.0"

// ─── Hardware ───────────────────────────────────────────────────────────────
#define LED_PIN         4        // GPIO4 — data pin for the LED panel
#define GRID_WIDTH      32       // 32 columns (horizontal)
#define GRID_HEIGHT     8        // 8 rows (vertical)
#define NUM_LEDS        (GRID_WIDTH * GRID_HEIGHT)  // 256

// LED strip type (WS2812B is most common for panels)
#define LED_TYPE        (NEO_GRB + NEO_KHZ800)

// ─── Defaults ─────────────────────────────────────────────────────────────
#define DEFAULT_BRIGHTNESS  128  // 0-255 — higher for opaque diffuser panel
#define DEFAULT_EFFECT      EFFECT_CLOCK

// ─── Animation Timing ──────────────────────────────────────────────────────
#define LED_UPDATE_INTERVAL_MS  30   // ~33 FPS
#define RAINBOW_CYCLE_MS      10000  // Full rainbow rotation period
#define COLOUR_WASH_CYCLE_MS   5000  // Solid hue sweep period

// ─── Grid Layout ───────────────────────────────────────────────────────────
#define SERPENTINE_LAYOUT  true

// ─── WiFi / Network ────────────────────────────────────────────────────────
#define MDNS_HOSTNAME   "ledpanel"       // http://ledpanel.local
#define AP_NAME         "LedPanel-Setup"
#define AP_PASS         "ledpanel123"
#define DEFAULT_AUTH_PASS "ledpanel"

// ─── Effect Enum ───────────────────────────────────────────────────────────
enum Effect : uint8_t {
    EFFECT_CLOCK,            // Digital clock — hours and minutes via NTP
    EFFECT_RAINBOW_WAVE,     // Rainbow ripple across the grid
    EFFECT_COLOUR_WASH,      // Entire grid fades through hues
    EFFECT_DIAGONAL_RAINBOW, // Rainbow bands on the diagonal
    EFFECT_RAIN,             // Coloured drops falling down columns
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
    EFFECT_TETRIS,           // Tetris simulation — pieces fall and stack
    EFFECT_SNAKE,            // Snake game — AI or manual phone control
    EFFECT_COUNT             // Sentinel — number of effects
};

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

    // Current state
    Effect   currentEffect;
    bool     manualMode;         // true = phone controls game

    // Auth
    char     authPassword[32];
};

#endif // CONFIG_H
