#include "led_effects.h"
#include "tetris_effect.h"
#include "snake_game.h"
#include <time.h>

// ─── Sine Lookup Table (PROGMEM) ────────────────────────────────────────────
// 256 entries, values -127..+127 (≈ sin(angle * 2π / 256) * 127)
static const int8_t SIN_TABLE[256] PROGMEM = {
      0,   3,   6,   9,  12,  15,  18,  21,  24,  27,  30,  33,  36,  39,  42,  45,
     48,  51,  54,  57,  59,  62,  65,  67,  70,  73,  75,  78,  80,  82,  85,  87,
     89,  91,  94,  96,  98, 100, 102, 103, 105, 107, 108, 110, 112, 113, 114, 116,
    117, 118, 119, 120, 121, 122, 123, 123, 124, 125, 125, 126, 126, 126, 126, 126,
    127, 126, 126, 126, 126, 126, 125, 125, 124, 123, 123, 122, 121, 120, 119, 118,
    117, 116, 114, 113, 112, 110, 108, 107, 105, 103, 102, 100,  98,  96,  94,  91,
     89,  87,  85,  82,  80,  78,  75,  73,  70,  67,  65,  62,  59,  57,  54,  51,
     48,  45,  42,  39,  36,  33,  30,  27,  24,  21,  18,  15,  12,   9,   6,   3,
      0,  -3,  -6,  -9, -12, -15, -18, -21, -24, -27, -30, -33, -36, -39, -42, -45,
    -48, -51, -54, -57, -59, -62, -65, -67, -70, -73, -75, -78, -80, -82, -85, -87,
    -89, -91, -94, -96, -98,-100,-102,-103,-105,-107,-108,-110,-112,-113,-114,-116,
   -117,-118,-119,-120,-121,-122,-123,-123,-124,-125,-125,-126,-126,-126,-126,-126,
   -127,-126,-126,-126,-126,-126,-125,-125,-124,-123,-123,-122,-121,-120,-119,-118,
   -117,-116,-114,-113,-112,-110,-108,-107,-105,-103,-102,-100, -98, -96, -94, -91,
    -89, -87, -85, -82, -80, -78, -75, -73, -70, -67, -65, -62, -59, -57, -54, -51,
    -48, -45, -42, -39, -36, -33, -30, -27, -24, -21, -18, -15, -12,  -9,  -6,  -3,
};

static inline int8_t fastSin(uint8_t angle) {
    return (int8_t)pgm_read_byte(&SIN_TABLE[angle]);
}

static inline int8_t fastCos(uint8_t angle) {
    return fastSin(angle + 64);
}

// ─── Fire Palette ───────────────────────────────────────────────────────────
// Maps heat value 0-255 to warm fire colours (black → red → orange → yellow → white)
static uint32_t heatColour(uint8_t temperature) {
    uint8_t r, g, b;
    if (temperature < 85) {
        // Black → Red
        r = temperature * 3;
        g = 0;
        b = 0;
    } else if (temperature < 170) {
        // Red → Orange/Yellow
        r = 255;
        g = (temperature - 85) * 3;
        b = 0;
    } else {
        // Yellow → White
        r = 255;
        g = 255;
        b = (temperature - 170) * 3;
    }
    return Adafruit_NeoPixel::Color(r, g, b);
}

// ─── Integer atan2 (0-255 angle) ────────────────────────────────────────────
// Returns angle 0-255 representing 0-2π
static uint8_t fastAtan2(int8_t dy, int8_t dx) {
    if (dx == 0 && dy == 0) return 0;
    // Use simple octant-based approximation
    uint8_t ax = abs(dx);
    uint8_t ay = abs(dy);
    // ratio ≈ min/max scaled to 0-64 range
    uint8_t ratio;
    if (ax >= ay) {
        ratio = (ay == 0) ? 0 : (uint8_t)((uint16_t)ay * 32 / ax);
    } else {
        ratio = 64 - (uint8_t)((uint16_t)ax * 32 / ay);
    }
    // Map to full angle based on octant
    if (dx >= 0 && dy >= 0) return ratio;               // Q1: 0-64
    if (dx < 0  && dy >= 0) return 128 - ratio;          // Q2: 64-128
    if (dx < 0  && dy < 0)  return 128 + ratio;          // Q3: 128-192
    return 0 - ratio;                                     // Q4: 192-256 (wraps)
}

// ─── Integer sqrt approximation ─────────────────────────────────────────────
static uint8_t fastSqrt(uint16_t val) {
    if (val == 0) return 0;
    uint8_t result = 1;
    // Newton's method, a few iterations
    result = (result + val / result) >> 1;
    result = (result + val / result) >> 1;
    result = (result + val / result) >> 1;
    return result;
}

// ─── Helpers ────────────────────────────────────────────────────────────────

uint16_t xyToIndex(uint8_t x, uint8_t y) {
    if (y >= GRID_HEIGHT || x >= GRID_WIDTH) return 0;

    // Column-major serpentine, rotated 180° to match panel orientation in case.
    // Flip both axes: physical col = last - logical col, physical row = last - logical row.
    uint8_t rx = (GRID_WIDTH - 1) - x;
    uint8_t ry = (GRID_HEIGHT - 1) - y;

    if (SERPENTINE_LAYOUT) {
        if (rx & 1) {
            return (uint16_t)rx * GRID_HEIGHT + (GRID_HEIGHT - 1 - ry);
        }
    }
    return (uint16_t)rx * GRID_HEIGHT + ry;
}

// HSV-like colour wheel: 0-255 maps to full hue rotation (R→G→B→R)
uint32_t colourWheel(uint8_t pos) {
    pos = 255 - pos;
    if (pos < 85) {
        return Adafruit_NeoPixel::Color(255 - pos * 3, 0, pos * 3);
    } else if (pos < 170) {
        pos -= 85;
        return Adafruit_NeoPixel::Color(0, pos * 3, 255 - pos * 3);
    } else {
        pos -= 170;
        return Adafruit_NeoPixel::Color(pos * 3, 255 - pos * 3, 0);
    }
}

// ─── Public API ─────────────────────────────────────────────────────────────

void initLeds(Adafruit_NeoPixel &strip) {
    strip.begin();
    strip.setBrightness(DEFAULT_BRIGHTNESS);
    strip.clear();
    strip.show();
}

Effect nextEffect(Effect current) {
    uint8_t next = ((uint8_t)current + 1) % EFFECT_COUNT;
    return (Effect)next;
}

// ─── Effects ────────────────────────────────────────────────────────────────

// Rainbow wave — hue ripples across the grid horizontally
static void effectRainbowWave(Adafruit_NeoPixel &strip) {
    uint32_t ms = millis();
    // Phase advances based on time; each column offset by hue
    uint8_t baseHue = (uint8_t)((ms * 256UL / RAINBOW_CYCLE_MS) % 256);

    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            // Spread 256 hue values across the grid width
            uint8_t hue = baseHue + (x * 256 / GRID_WIDTH);
            strip.setPixelColor(xyToIndex(x, y), colourWheel(hue));
        }
    }
    strip.show();
}

// Colour wash — entire grid is one solid colour, smoothly sweeping through hues
static void effectColourWash(Adafruit_NeoPixel &strip) {
    uint32_t ms = millis();
    uint8_t hue = (uint8_t)((ms * 256UL / COLOUR_WASH_CYCLE_MS) % 256);
    uint32_t colour = colourWheel(hue);

    strip.fill(colour);
    strip.show();
}

// Diagonal rainbow — hue bands run along the diagonal (x + y)
static void effectDiagonalRainbow(Adafruit_NeoPixel &strip) {
    uint32_t ms = millis();
    uint8_t baseHue = (uint8_t)((ms * 256UL / RAINBOW_CYCLE_MS) % 256);

    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            // Diagonal distance from top-left, mapped to hue
            uint8_t diag = x + y;  // 0..(GRID_WIDTH + GRID_HEIGHT - 2)
            uint8_t hue = baseHue + (diag * 256 / (GRID_WIDTH + GRID_HEIGHT));
            strip.setPixelColor(xyToIndex(x, y), colourWheel(hue));
        }
    }
    strip.show();
}

// Colour rain — coloured drops fall down each column at varying speeds
static void effectRain(Adafruit_NeoPixel &strip) {
    // Persistent state for drop positions
    static uint8_t dropY[GRID_WIDTH];
    static uint8_t dropHue[GRID_WIDTH];
    static uint8_t dropSpeed[GRID_WIDTH];  // Frames between advances
    static uint8_t dropCounter[GRID_WIDTH];
    static bool initialised = false;

    if (!initialised) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            dropY[x] = random(GRID_HEIGHT);
            dropHue[x] = random(256);
            dropSpeed[x] = 2 + random(5);  // 2-6 frames per step
            dropCounter[x] = 0;
        }
        initialised = true;
    }

    // Fade all pixels slightly (trail effect)
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        uint32_t c = strip.getPixelColor(i);
        uint8_t r = (uint8_t)(c >> 16);
        uint8_t g = (uint8_t)(c >> 8);
        uint8_t b = (uint8_t)c;
        // Fade by ~20%
        r = (r * 200) >> 8;
        g = (g * 200) >> 8;
        b = (b * 200) >> 8;
        strip.setPixelColor(i, Adafruit_NeoPixel::Color(r, g, b));
    }

    // Advance and draw drops
    for (uint8_t x = 0; x < GRID_WIDTH; x++) {
        dropCounter[x]++;
        if (dropCounter[x] >= dropSpeed[x]) {
            dropCounter[x] = 0;
            dropY[x]++;
            if (dropY[x] >= GRID_HEIGHT) {
                // Reset at top with new colour
                dropY[x] = 0;
                dropHue[x] = random(256);
                dropSpeed[x] = 2 + random(5);
            }
        }
        // Draw the leading pixel at full brightness
        strip.setPixelColor(xyToIndex(x, dropY[x]), colourWheel(dropHue[x]));
    }
    strip.show();
}

// ─── Clock ──────────────────────────────────────────────────────────────

// 4x6 pixel font for digits 0-9. Each digit is stored as 6 rows of 4 bits.
static const uint8_t DIGIT_FONT[10][6] = {
    {0b0110, 0b1001, 0b1001, 0b1001, 0b1001, 0b0110},  // 0
    {0b0010, 0b0110, 0b0010, 0b0010, 0b0010, 0b0111},  // 1
    {0b0110, 0b1001, 0b0001, 0b0010, 0b0100, 0b1111},  // 2
    {0b0110, 0b1001, 0b0010, 0b0001, 0b1001, 0b0110},  // 3
    {0b1001, 0b1001, 0b1111, 0b0001, 0b0001, 0b0001},  // 4
    {0b1111, 0b1000, 0b1110, 0b0001, 0b1001, 0b0110},  // 5
    {0b0110, 0b1000, 0b1110, 0b1001, 0b1001, 0b0110},  // 6
    {0b1111, 0b0001, 0b0010, 0b0010, 0b0100, 0b0100},  // 7
    {0b0110, 0b1001, 0b0110, 0b1001, 0b1001, 0b0110},  // 8
    {0b0110, 0b1001, 0b0111, 0b0001, 0b0010, 0b0100},  // 9
};

static void drawDigit(Adafruit_NeoPixel &strip, uint8_t digit, uint8_t startX,
                      uint8_t startY, uint32_t colour) {
    if (digit > 9) return;
    for (uint8_t row = 0; row < 6; row++) {
        for (uint8_t col = 0; col < 4; col++) {
            if (DIGIT_FONT[digit][row] & (0b1000 >> col)) {
                uint8_t x = startX + col;
                uint8_t y = startY + row;
                if (x < GRID_WIDTH && y < GRID_HEIGHT) {
                    strip.setPixelColor(xyToIndex(x, y), colour);
                }
            }
        }
    }
}

static void effectClock(Adafruit_NeoPixel &strip) {
    uint32_t now = millis();

    // Warm palette
    uint32_t bgColour    = Adafruit_NeoPixel::Color(1, 1, 4);
    uint32_t digitColour = Adafruit_NeoPixel::Color(230, 210, 170);

    // Horizontal layout: HH    MM (4-col gap between groups)
    // Row 0: top margin
    // Rows 1-6: 4x6 digits
    // Row 7: seconds progress dot
    //
    // H tens: x=5-8, H units: x=10-13, gap: x=14-17, M tens: x=18-21, M units: x=23-26
    // Left margin: 5 cols, right margin: 5 cols — perfectly centred
    uint8_t digitY = 1;

    strip.fill(bgColour);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) {
        // No NTP sync — show pulsing dashes as placeholder
        uint16_t phase = (now / 4) & 0xFF;
        uint8_t wave = (phase < 128) ? (uint8_t)(phase * 2) :
                                       (uint8_t)((255 - phase) * 2);
        uint8_t pulse = 50 + (uint8_t)((uint16_t)wave * 130 / 255);
        uint32_t dashCol = Adafruit_NeoPixel::Color(
            pulse, pulse * 210 / 230, pulse * 170 / 230);

        // Dashes at middle row of each digit position
        uint8_t dashRow = digitY + 3;
        for (uint8_t col = 0; col < 4; col++) {
            strip.setPixelColor(xyToIndex(5 + col, dashRow), dashCol);
            strip.setPixelColor(xyToIndex(10 + col, dashRow), dashCol);
            strip.setPixelColor(xyToIndex(18 + col, dashRow), dashCol);
            strip.setPixelColor(xyToIndex(23 + col, dashRow), dashCol);
        }
        strip.show();
        return;
    }

    uint8_t h = timeinfo.tm_hour;
    uint8_t m = timeinfo.tm_min;
    uint8_t s = timeinfo.tm_sec;

    // ── Seconds: sweeping dot along row 7 with trailing fade ──
    // Colour cycles each minute (full hue rotation per hour)
    uint8_t trailHue = (uint8_t)((uint16_t)m * 256 / 60);
    uint32_t trailBase = colourWheel(trailHue);
    uint8_t tR = (uint8_t)((trailBase >> 16) & 0xFF);
    uint8_t tG = (uint8_t)((trailBase >> 8) & 0xFF);
    uint8_t tB = (uint8_t)(trailBase & 0xFF);
    // Cap brightness so trail doesn't overpower digits
    tR = (uint8_t)((uint16_t)tR * 190 >> 8);
    tG = (uint8_t)((uint16_t)tG * 190 >> 8);
    tB = (uint8_t)((uint16_t)tB * 190 >> 8);

    // Map second (0-59) to x position across 32 columns
    uint8_t dotX = (uint8_t)((uint16_t)s * GRID_WIDTH / 60);

    // 6-pixel trail with quadratic fade — draw oldest first so head wins
    for (int8_t trail = 5; trail >= 0; trail--) {
        int8_t tx = (int8_t)dotX - trail;
        if (tx < 0) tx += GRID_WIDTH;
        uint16_t inv = (uint16_t)(6 - trail);  // 1..6
        uint8_t fade = (uint8_t)(inv * inv * 255 / 36);
        strip.setPixelColor(xyToIndex((uint8_t)tx, GRID_HEIGHT - 1),
            Adafruit_NeoPixel::Color(
                (uint8_t)((uint16_t)tR * fade >> 8),
                (uint8_t)((uint16_t)tG * fade >> 8),
                (uint8_t)((uint16_t)tB * fade >> 8)));
    }

    // ── Hours (x=5 and x=10) ──
    drawDigit(strip, h / 10, 5, digitY, digitColour);
    drawDigit(strip, h % 10, 10, digitY, digitColour);

    // ── Minutes (x=18 and x=23) ──
    drawDigit(strip, m / 10, 18, digitY, digitColour);
    drawDigit(strip, m % 10, 23, digitY, digitColour);

    strip.show();
}

// ─── Fire ───────────────────────────────────────────────────────────────────
// Heat-based fire simulation. Heat rises from bottom, cools as it goes up.

static void effectFire(Adafruit_NeoPixel &strip) {
    static uint8_t heat[GRID_HEIGHT][GRID_WIDTH];
    static bool initialised = false;

    if (!initialised) {
        memset(heat, 0, sizeof(heat));
        initialised = true;
    }

    // Cool each cell by a small random amount
    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            uint8_t cooling = random(0, 18);  // Higher cooling for shorter grid
            heat[y][x] = (heat[y][x] > cooling) ? heat[y][x] - cooling : 0;
        }
    }

    // Propagate heat upward with diffusion (bottom = high y index)
    for (uint8_t y = 0; y < GRID_HEIGHT - 1; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            uint8_t xl = (x > 0) ? x - 1 : 0;
            uint8_t xr = (x < GRID_WIDTH - 1) ? x + 1 : GRID_WIDTH - 1;
            heat[y][x] = (heat[y + 1][xl] + heat[y + 1][x] * 2 + heat[y + 1][xr]) / 4;
        }
    }

    // Ignite bottom row
    for (uint8_t x = 0; x < GRID_WIDTH; x++) {
        heat[GRID_HEIGHT - 1][x] = 160 + random(96);
    }

    // Render
    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            strip.setPixelColor(xyToIndex(x, y), heatColour(heat[y][x]));
        }
    }
    strip.show();
}

// ─── Aurora ─────────────────────────────────────────────────────────────────
// Flowing green/blue/purple bands that undulate horizontally.

static void effectAurora(Adafruit_NeoPixel &strip) {
    uint32_t ms = millis();
    uint8_t timePhase1 = (uint8_t)(ms / 40);
    uint8_t timePhase2 = (uint8_t)(ms / 60);
    uint8_t timePhase3 = (uint8_t)(ms / 80);

    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            // Overlapping sine waves — frequencies scaled for 32x8
            int16_t wave1 = fastSin((uint8_t)(x * 10 + timePhase1));
            int16_t wave2 = fastSin((uint8_t)(x * 6 + y * 16 + timePhase2));
            int16_t wave3 = fastSin((uint8_t)(y * 32 + timePhase3));

            // Combine and map to vertical band position
            int16_t combined = (wave1 + wave2 + wave3) / 3;
            // Map y position relative to wave centre (scaled to 0-255 range)
            int16_t bandDist = abs((int16_t)y * 32 - 128 - combined);

            // Brightness falls off with distance from band centre
            uint8_t bright = (bandDist < 60) ? (uint8_t)(255 - bandDist * 4) : 0;

            // Aurora palette: deep blue → teal → green → purple
            uint8_t hueBase = (uint8_t)(x * 6 + timePhase1);
            // Shift to green/blue/purple range (avoid reds)
            uint8_t r = (uint8_t)((uint16_t)bright * (uint16_t)(fastSin(hueBase + 170) + 128) >> 9);
            uint8_t g = (uint8_t)((uint16_t)bright * (uint16_t)(fastSin(hueBase + 64) + 128) >> 8);
            uint8_t b = (uint8_t)((uint16_t)bright * (uint16_t)(fastSin(hueBase) + 128) >> 8);

            strip.setPixelColor(xyToIndex(x, y), Adafruit_NeoPixel::Color(r, g, b));
        }
    }
    strip.show();
}

// ─── Lava Lamp ──────────────────────────────────────────────────────────────
// Slow-moving coloured blobs (metaballs).

static void effectLava(Adafruit_NeoPixel &strip) {
    uint32_t ms = millis();

    // 3 blob centres drifting on slow sine paths
    struct Blob {
        int16_t cx, cy;
        uint8_t hue;
    };

    Blob blobs[3];

    // Map to normalised 256x256 coordinate space
    // For 32x8: px = x*8+4, py = y*32+16

    // Blob 0: slow drift
    blobs[0].cx = 128 + (int16_t)fastSin((uint8_t)(ms / 50)) * 80 / 127;
    blobs[0].cy = 128 + (int16_t)fastCos((uint8_t)(ms / 70)) * 80 / 127;
    blobs[0].hue = (uint8_t)(ms / 100);

    // Blob 1: medium drift
    blobs[1].cx = 128 + (int16_t)fastSin((uint8_t)(ms / 40 + 85)) * 80 / 127;
    blobs[1].cy = 128 + (int16_t)fastCos((uint8_t)(ms / 55 + 85)) * 80 / 127;
    blobs[1].hue = (uint8_t)(ms / 100 + 85);

    // Blob 2: faster drift
    blobs[2].cx = 128 + (int16_t)fastSin((uint8_t)(ms / 35 + 170)) * 80 / 127;
    blobs[2].cy = 128 + (int16_t)fastCos((uint8_t)(ms / 45 + 170)) * 80 / 127;
    blobs[2].hue = (uint8_t)(ms / 100 + 170);

    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            // Normalised coordinates (0-255 in both axes)
            int16_t px = (int16_t)x * (256 / GRID_WIDTH) + (128 / GRID_WIDTH);
            int16_t py = (int16_t)y * (256 / GRID_HEIGHT) + (128 / GRID_HEIGHT);

            // Sum of inverse distances to blob centres
            uint16_t energy = 0;
            uint16_t hueSum = 0;
            uint16_t weightSum = 0;

            for (uint8_t i = 0; i < 3; i++) {
                int16_t dx = px - blobs[i].cx;
                int16_t dy = py - blobs[i].cy;
                uint16_t distSq = (uint16_t)(dx * dx + dy * dy);
                if (distSq < 4) distSq = 4;
                uint16_t inv = 10000 / distSq;
                energy += inv;
                hueSum += (uint16_t)blobs[i].hue * inv;
                weightSum += inv;
            }

            // Threshold for blob edge
            uint8_t bright;
            if (energy > 80) bright = 255;
            else if (energy > 40) bright = (uint8_t)((energy - 40) * 255 / 40);
            else bright = 0;

            if (bright > 0 && weightSum > 0) {
                uint8_t hue = (uint8_t)(hueSum / weightSum);
                uint32_t c = colourWheel(hue);
                uint8_t r = (uint8_t)((uint16_t)((c >> 16) & 0xFF) * bright >> 8);
                uint8_t g = (uint8_t)((uint16_t)((c >> 8) & 0xFF) * bright >> 8);
                uint8_t b = (uint8_t)((uint16_t)(c & 0xFF) * bright >> 8);
                strip.setPixelColor(xyToIndex(x, y), Adafruit_NeoPixel::Color(r, g, b));
            } else {
                strip.setPixelColor(xyToIndex(x, y), 0);
            }
        }
    }
    strip.show();
}

// ─── Candle ─────────────────────────────────────────────────────────────────
// Warm flickering candlelight — brighter in the centre, random fluctuations.

static void effectCandle(Adafruit_NeoPixel &strip) {
    static uint8_t flicker[GRID_WIDTH];
    static bool initialised = false;

    if (!initialised) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) flicker[x] = 128;
        initialised = true;
    }

    // Update flicker per column — smooth random walk
    for (uint8_t x = 0; x < GRID_WIDTH; x++) {
        int16_t delta = (int16_t)random(0, 40) - 20;
        int16_t nv = (int16_t)flicker[x] + delta;
        // Bias toward centre (128)
        nv = (nv * 3 + 128) / 4;
        if (nv < 60) nv = 60;
        if (nv > 220) nv = 220;
        flicker[x] = (uint8_t)nv;
    }

    // Centre of grid — generic for any dimensions
    float cx = (float)(GRID_WIDTH - 1) / 2.0f;
    float cy = (float)(GRID_HEIGHT - 1) / 2.0f;
    float maxDist = sqrtf(cx * cx + cy * cy);

    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            // Distance from centre, normalised 0-255
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float dist = sqrtf(dx * dx + dy * dy);
            uint8_t distBright = (dist < maxDist) ? (uint8_t)(255 - dist * 255 / maxDist) : 0;

            // Combine distance and flicker
            uint8_t bright = (uint8_t)((uint16_t)distBright * flicker[x] >> 8);

            // Warm amber palette: R=255, G=100, B=20, scaled by brightness
            uint8_t r = (uint8_t)((uint16_t)255 * bright >> 8);
            uint8_t g = (uint8_t)((uint16_t)100 * bright >> 8);
            uint8_t b = (uint8_t)((uint16_t)20 * bright >> 8);

            strip.setPixelColor(xyToIndex(x, y), Adafruit_NeoPixel::Color(r, g, b));
        }
    }
    strip.show();
}

// ─── Twinkle Stars ──────────────────────────────────────────────────────────
// Random pixels light up and fade out like a starfield.

static void effectTwinkle(Adafruit_NeoPixel &strip) {
    static uint8_t starBright[NUM_LEDS];
    static uint8_t starHue[NUM_LEDS];
    static bool initialised = false;

    if (!initialised) {
        memset(starBright, 0, sizeof(starBright));
        memset(starHue, 0, sizeof(starHue));
        initialised = true;
    }

    // Fade all stars
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        if (starBright[i] > 4) starBright[i] -= 4;
        else starBright[i] = 0;
    }

    // Spawn 1-2 new stars per frame
    uint8_t toSpawn = 1 + random(2);
    for (uint8_t s = 0; s < toSpawn; s++) {
        uint16_t idx = random(NUM_LEDS);
        if (starBright[idx] == 0) {
            starBright[idx] = 200 + random(56);
            starHue[idx] = random(256);
        }
    }

    // Render
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        if (starBright[i] > 0) {
            uint32_t c = colourWheel(starHue[i]);
            uint8_t r = (uint8_t)((uint16_t)((c >> 16) & 0xFF) * starBright[i] >> 8);
            uint8_t g = (uint8_t)((uint16_t)((c >> 8) & 0xFF) * starBright[i] >> 8);
            uint8_t b = (uint8_t)((uint16_t)(c & 0xFF) * starBright[i] >> 8);
            strip.setPixelColor(i, Adafruit_NeoPixel::Color(r, g, b));
        } else {
            strip.setPixelColor(i, 0);
        }
    }
    strip.show();
}

// ─── Matrix ─────────────────────────────────────────────────────────────────
// Green falling code streams (The Matrix).

static void effectMatrix(Adafruit_NeoPixel &strip) {
    static uint8_t headY[GRID_WIDTH];
    static uint8_t speed[GRID_WIDTH];
    static uint8_t counter[GRID_WIDTH];
    static bool initialised = false;

    if (!initialised) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            headY[x] = random(GRID_HEIGHT);
            speed[x] = 1 + random(4);
            counter[x] = 0;
        }
        initialised = true;
    }

    // Fade all pixels — green channel fades slower for trail effect
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        uint32_t c = strip.getPixelColor(i);
        uint8_t r = (uint8_t)(c >> 16);
        uint8_t g = (uint8_t)(c >> 8);
        uint8_t b = (uint8_t)c;
        r = (r * 140) >> 8;
        g = (g * 200) >> 8;
        b = (b * 140) >> 8;
        strip.setPixelColor(i, Adafruit_NeoPixel::Color(r, g, b));
    }

    // Advance heads
    for (uint8_t x = 0; x < GRID_WIDTH; x++) {
        counter[x]++;
        if (counter[x] >= speed[x]) {
            counter[x] = 0;
            headY[x]++;
            if (headY[x] >= GRID_HEIGHT) {
                headY[x] = 0;
                speed[x] = 1 + random(4);
            }
        }
        // Head pixel: bright white-green
        strip.setPixelColor(xyToIndex(x, headY[x]),
                            Adafruit_NeoPixel::Color(200, 255, 200));
    }
    strip.show();
}

// ─── Fireworks ──────────────────────────────────────────────────────────────
// Particles launch upward then explode into expanding coloured rings.

static void effectFireworks(Adafruit_NeoPixel &strip) {
    enum Phase : uint8_t { IDLE, LAUNCH, BURST };
    static Phase phase = IDLE;
    static uint8_t launchX;
    static int8_t  launchY;
    static uint8_t burstHue;
    static uint32_t phaseStart;
    static int16_t particleX[12];
    static int16_t particleY[12];
    static int8_t  particleVX[12];
    static int8_t  particleVY[12];
    static uint8_t numParticles;

    uint32_t now = millis();

    // Fade everything
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        uint32_t c = strip.getPixelColor(i);
        uint8_t r = (uint8_t)((uint16_t)((c >> 16) & 0xFF) * 180 >> 8);
        uint8_t g = (uint8_t)((uint16_t)((c >> 8) & 0xFF) * 180 >> 8);
        uint8_t b = (uint8_t)((uint16_t)(c & 0xFF) * 180 >> 8);
        strip.setPixelColor(i, Adafruit_NeoPixel::Color(r, g, b));
    }

    switch (phase) {
        case IDLE:
            if (now - phaseStart > 300 + random(800)) {
                phase = LAUNCH;
                launchX = 6 + random(GRID_WIDTH - 12);
                launchY = GRID_HEIGHT - 1;
                burstHue = random(256);
                phaseStart = now;
            }
            break;

        case LAUNCH:
            launchY--;
            if (launchY >= 0 && launchY < GRID_HEIGHT) {
                strip.setPixelColor(xyToIndex(launchX, (uint8_t)launchY),
                                    Adafruit_NeoPixel::Color(255, 255, 220));
            }
            if (launchY <= 1 + (int8_t)random(2)) {
                // Explode near the top
                phase = BURST;
                phaseStart = now;
                numParticles = 8 + random(5);
                for (uint8_t p = 0; p < numParticles; p++) {
                    particleX[p] = (int16_t)launchX * 16;
                    particleY[p] = (int16_t)launchY * 16;
                    uint8_t angle = p * (256 / numParticles) + random(10);
                    particleVX[p] = fastSin(angle + 64) / 16;
                    particleVY[p] = fastSin(angle) / 16;
                }
            }
            break;

        case BURST: {
            uint32_t elapsed = now - phaseStart;
            if (elapsed > 1200) {
                phase = IDLE;
                phaseStart = now;
                break;
            }

            uint8_t fade = (elapsed < 800) ? 255 : (uint8_t)(255 - (elapsed - 800) * 255 / 400);

            for (uint8_t p = 0; p < numParticles; p++) {
                particleX[p] += particleVX[p];
                particleY[p] += particleVY[p];
                // Gravity
                particleVY[p] += 1;

                uint8_t px = (uint8_t)(particleX[p] / 16);
                uint8_t py = (uint8_t)(particleY[p] / 16);
                if (px < GRID_WIDTH && py < GRID_HEIGHT) {
                    uint32_t c = colourWheel(burstHue + p * 15);
                    uint8_t r = (uint8_t)((uint16_t)((c >> 16) & 0xFF) * fade >> 8);
                    uint8_t g = (uint8_t)((uint16_t)((c >> 8) & 0xFF) * fade >> 8);
                    uint8_t b = (uint8_t)((uint16_t)(c & 0xFF) * fade >> 8);
                    strip.setPixelColor(xyToIndex(px, py), Adafruit_NeoPixel::Color(r, g, b));
                }
            }
            break;
        }
    }
    strip.show();
}

// ─── Game of Life ───────────────────────────────────────────────────────────
// Conway's Game of Life with colour — auto-reseeds on stagnation.

static void effectLife(Adafruit_NeoPixel &strip) {
    static bool grid[2][GRID_HEIGHT][GRID_WIDTH];
    static uint8_t hueGrid[GRID_HEIGHT][GRID_WIDTH];
    static uint8_t current = 0;
    static uint32_t lastUpdate = 0;
    static uint16_t lastPop[3] = {0, 0, 0};
    static uint8_t popIdx = 0;
    static bool initialised = false;

    auto seed = [&]() {
        for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
            for (uint8_t x = 0; x < GRID_WIDTH; x++) {
                grid[current][y][x] = random(100) < 35;
                hueGrid[y][x] = random(256);
            }
        }
    };

    if (!initialised) {
        memset(grid, 0, sizeof(grid));
        seed();
        initialised = true;
    }

    uint32_t now = millis();
    if (now - lastUpdate >= 150) {
        lastUpdate = now;

        uint8_t next = 1 - current;
        uint16_t population = 0;

        for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
            for (uint8_t x = 0; x < GRID_WIDTH; x++) {
                // Count neighbours (wrapping)
                uint8_t neighbours = 0;
                for (int8_t dy = -1; dy <= 1; dy++) {
                    for (int8_t dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        uint8_t nx = (x + dx + GRID_WIDTH) % GRID_WIDTH;
                        uint8_t ny = (y + dy + GRID_HEIGHT) % GRID_HEIGHT;
                        if (grid[current][ny][nx]) neighbours++;
                    }
                }

                bool alive = grid[current][y][x];
                if (alive) {
                    grid[next][y][x] = (neighbours == 2 || neighbours == 3);
                } else {
                    grid[next][y][x] = (neighbours == 3);
                }

                if (grid[next][y][x]) {
                    population++;
                    // Shift hue slightly each generation
                    if (!alive) hueGrid[y][x] = random(256);  // Newborn
                    else hueGrid[y][x] += 2;
                }
            }
        }

        current = next;

        // Detect stagnation (population stable or zero)
        lastPop[popIdx] = population;
        popIdx = (popIdx + 1) % 3;
        if (population == 0 ||
            (lastPop[0] == lastPop[1] && lastPop[1] == lastPop[2])) {
            seed();
        }
    }

    // Render current state
    strip.clear();
    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            if (grid[current][y][x]) {
                strip.setPixelColor(xyToIndex(x, y), colourWheel(hueGrid[y][x]));
            }
        }
    }
    strip.show();
}

// ─── Plasma ─────────────────────────────────────────────────────────────────
// Classic demoscene sine-wave interference patterns.

static void effectPlasma(Adafruit_NeoPixel &strip) {
    uint32_t ms = millis();
    uint8_t t1 = (uint8_t)(ms / 30);
    uint8_t t2 = (uint8_t)(ms / 40);
    uint8_t t3 = (uint8_t)(ms / 50);
    uint8_t t4 = (uint8_t)(ms / 35);

    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            // Four overlapping sine components — scaled for 32x8
            int16_t v1 = fastSin((uint8_t)(x * 8 + t1));
            int16_t v2 = fastSin((uint8_t)(y * 32 + t2));
            int16_t v3 = fastSin((uint8_t)((x + y) * 6 + t3));
            // Radial component — centred on the grid
            uint8_t dx = (x > GRID_WIDTH / 2) ? x - GRID_WIDTH / 2 : GRID_WIDTH / 2 - x;
            uint8_t dy = (y > GRID_HEIGHT / 2) ? y - GRID_HEIGHT / 2 : GRID_HEIGHT / 2 - y;
            uint8_t dist = fastSqrt((uint16_t)(dx * dx + dy * dy) * 64);
            int16_t v4 = fastSin((uint8_t)(dist + t4));

            // Combine and map to hue (0-255)
            int16_t total = v1 + v2 + v3 + v4;  // Range: -508..+508
            uint8_t hue = (uint8_t)((total + 508) * 255 / 1016);

            strip.setPixelColor(xyToIndex(x, y), colourWheel(hue));
        }
    }
    strip.show();
}

// ─── Spiral ─────────────────────────────────────────────────────────────────
// Rotating colour pinwheel from the centre.

static void effectSpiral(Adafruit_NeoPixel &strip) {
    uint32_t ms = millis();
    uint8_t timeSpin = (uint8_t)(ms / 20);  // Rotation speed

    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            int8_t dx = (int8_t)x - (GRID_WIDTH / 2);
            int8_t dy = (int8_t)y - (GRID_HEIGHT / 2);

            // Angle from centre (0-255)
            uint8_t angle = fastAtan2(dy, dx);

            // Distance from centre
            uint8_t dist = fastSqrt((uint16_t)(dx * dx + dy * dy) * 16);

            // 4 spiral arms, tightness controlled by dist multiplier
            uint8_t hue = angle * 4 + dist * 3 + timeSpin;

            strip.setPixelColor(xyToIndex(x, y), colourWheel(hue));
        }
    }
    strip.show();
}

// ─── Dispatcher ─────────────────────────────────────────────────────────────

void updateEffect(Adafruit_NeoPixel &strip, Effect effect) {
    switch (effect) {
        case EFFECT_CLOCK:            effectClock(strip);           break;
        case EFFECT_RAINBOW_WAVE:     effectRainbowWave(strip);     break;
        case EFFECT_COLOUR_WASH:      effectColourWash(strip);      break;
        case EFFECT_DIAGONAL_RAINBOW: effectDiagonalRainbow(strip); break;
        case EFFECT_RAIN:             effectRain(strip);            break;
        case EFFECT_FIRE:             effectFire(strip);            break;
        case EFFECT_AURORA:           effectAurora(strip);          break;
        case EFFECT_LAVA:             effectLava(strip);            break;
        case EFFECT_CANDLE:           effectCandle(strip);          break;
        case EFFECT_TWINKLE:          effectTwinkle(strip);         break;
        case EFFECT_MATRIX:           effectMatrix(strip);          break;
        case EFFECT_FIREWORKS:        effectFireworks(strip);       break;
        case EFFECT_LIFE:             effectLife(strip);            break;
        case EFFECT_PLASMA:           effectPlasma(strip);          break;
        case EFFECT_SPIRAL:           effectSpiral(strip);          break;
        case EFFECT_TETRIS:           updateTetris(strip);          break;
        case EFFECT_SNAKE:            updateSnake(strip);           break;
        default:                      effectClock(strip);           break;
    }
}
