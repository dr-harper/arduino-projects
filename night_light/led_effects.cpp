#include "led_effects.h"
#include <math.h>
#include <esp_task_wdt.h>

// Thermal dimming factor from main loop (1.0 = normal, 0.0 = fully throttled)
extern float thermalDimFactor;
// Master fade factor for smooth on/off toggle (1.0 = fully on, 0.0 = fully off)
extern float masterFadeFactor;

// ─── Helper: colour wheel (0–255 → rainbow) ────────────────────────────────
uint32_t wheel(Adafruit_NeoPixel &strip, uint8_t pos) {
    pos = 255 - pos;
    if (pos < 85) {
        return strip.Color(255 - pos * 3, 0, pos * 3, 0);
    } else if (pos < 170) {
        pos -= 85;
        return strip.Color(0, pos * 3, 255 - pos * 3, 0);
    } else {
        pos -= 170;
        return strip.Color(pos * 3, 255 - pos * 3, 0, 0);
    }
}

// ─── Helper: raw RGB from wheel position (no packing) ──────────────────────
void wheelRGB(uint8_t pos, uint8_t &r, uint8_t &g, uint8_t &b) {
    pos = 255 - pos;
    if (pos < 85) {
        r = 255 - pos * 3;
        g = 0;
        b = pos * 3;
    } else if (pos < 170) {
        pos -= 85;
        r = 0;
        g = pos * 3;
        b = 255 - pos * 3;
    } else {
        pos -= 170;
        r = pos * 3;
        g = 255 - pos * 3;
        b = 0;
    }
}

// ─── Helper: linear interpolation ───────────────────────────────────────────
static uint8_t lerp8(uint8_t a, uint8_t b, float t) {
    return (uint8_t)((float)a + ((float)b - (float)a) * t);
}

// ─── 0. Solid Colour ────────────────────────────────────────────────────────
void effectSolid(Adafruit_NeoPixel &strip,
                 uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t brightness) {
    strip.setBrightness(brightness);
    uint32_t colour = strip.Color(r, g, b, w);
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, colour);
    }
    strip.show();
}

// ─── 1. Breathing ───────────────────────────────────────────────────────────
// Gentle sinusoidal brightness pulse, 6-second cycle
void effectBreathing(Adafruit_NeoPixel &strip,
                     uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                     uint8_t brightness, unsigned long now) {
    // Sinusoidal brightness modulation
    float phase = (float)now / BREATHING_CYCLE_MS * 2.0f * PI;
    float factor = (sinf(phase) + 1.0f) / 2.0f;  // 0.0 → 1.0

    // Modulate between 15% and 100% of the target brightness
    uint8_t minBri = (uint8_t)(brightness * 0.15f);
    uint8_t actualBri = minBri + (uint8_t)((brightness - minBri) * factor);

    strip.setBrightness(actualBri);
    uint32_t colour = strip.Color(r, g, b, w);
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, colour);
    }
    strip.show();
}

// ─── 2. Soft Glow ──────────────────────────────────────────────────────────
// Very slow colour temperature drift within a warm range
void effectSoftGlow(Adafruit_NeoPixel &strip,
                    uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                    uint8_t brightness, unsigned long now) {
    strip.setBrightness(brightness);

    float phase = (float)now / GLOW_CYCLE_MS * 2.0f * PI;
    int numLeds = strip.numPixels();

    for (int i = 0; i < numLeds; i++) {
        // One smooth wave spanning the full strip length
        float ledPhase = phase + (float)i / (float)numLeds * 2.0f * PI;
        float drift = (sinf(ledPhase) + 1.0f) / 2.0f;  // 0.0 → 1.0

        // Drift the colour slightly warmer/cooler
        uint8_t rr = lerp8(r, (uint8_t)min(255, (int)r + 30), drift);
        uint8_t gg = lerp8((uint8_t)(g * 0.7f), g, drift);
        uint8_t bb = lerp8((uint8_t)(b * 0.5f), b, drift);
        uint8_t ww = lerp8((uint8_t)(w * 0.6f), w, drift);

        strip.setPixelColor(i, strip.Color(rr, gg, bb, ww));
    }
    strip.show();
}

// ─── 3. Starry Twinkle ─────────────────────────────────────────────────────
// Base colour with occasional individual LEDs briefly brightening
void effectStarryTwinkle(Adafruit_NeoPixel &strip,
                         uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                         uint8_t brightness, unsigned long now) {
    static uint8_t twinkleBrightness[NUM_LEDS];
    static bool initialised = false;

    if (!initialised) {
        memset(twinkleBrightness, 0, sizeof(twinkleBrightness));
        initialised = true;
    }

    strip.setBrightness(brightness);

    // Base colour at reduced intensity
    uint8_t baseR = (uint8_t)(r * 0.4f);
    uint8_t baseG = (uint8_t)(g * 0.4f);
    uint8_t baseB = (uint8_t)(b * 0.4f);
    uint8_t baseW = (uint8_t)(w * 0.4f);

    int numLeds = strip.numPixels();

    for (int i = 0; i < numLeds; i++) {
        // Randomly spark new twinkles
        if (twinkleBrightness[i] == 0 && (int)random(100) < TWINKLE_CHANCE) {
            twinkleBrightness[i] = (uint8_t)random(150, 255);
        }

        // Blend between base colour and bright white based on twinkle level
        if (twinkleBrightness[i] > 0) {
            float t = (float)twinkleBrightness[i] / 255.0f;
            uint8_t rr = lerp8(baseR, r, t);
            uint8_t gg = lerp8(baseG, g, t);
            uint8_t bb = lerp8(baseB, (uint8_t)min(255, (int)b + 40), t);
            uint8_t ww = lerp8(baseW, (uint8_t)min(255, (int)w + 40), t);
            strip.setPixelColor(i, strip.Color(rr, gg, bb, ww));

            // Fade the twinkle
            if (twinkleBrightness[i] > 8) {
                twinkleBrightness[i] -= 8;
            } else {
                twinkleBrightness[i] = 0;
            }
        } else {
            strip.setPixelColor(i, strip.Color(baseR, baseG, baseB, baseW));
        }
    }
    strip.show();
}

// ─── 4. Rainbow ─────────────────────────────────────────────────────────────
// Very slow rainbow cycle at low brightness (pure RGB, no user white)
void effectRainbow(Adafruit_NeoPixel &strip,
                   uint8_t brightness, unsigned long now) {
    strip.setBrightness(brightness);

    // Very slow rotation: full cycle in ~30 seconds
    uint8_t offset = (uint8_t)((now / 120) & 0xFF);
    int numLeds = strip.numPixels();

    for (int i = 0; i < numLeds; i++) {
        uint8_t pos = (uint8_t)((i * 256 / numLeds + offset) & 0xFF);
        strip.setPixelColor(i, wheel(strip, pos));
    }
    strip.show();
}

// ─── 5. Candle Flicker ─────────────────────────────────────────────────────
// Warm amber with randomised per-LED brightness variation
void effectCandleFlicker(Adafruit_NeoPixel &strip,
                         uint8_t w, uint8_t brightness, unsigned long now) {
    static unsigned long lastFlicker = 0;
    static uint8_t flickerValues[NUM_LEDS];
    static bool initialised = false;

    if (!initialised) {
        for (int i = 0; i < NUM_LEDS; i++) {
            flickerValues[i] = (uint8_t)random(100, 255);
        }
        initialised = true;
    }

    if (now - lastFlicker >= CANDLE_FLICKER_MS) {
        lastFlicker = now;

        int numLeds = strip.numPixels();
        // Update a few random LEDs each frame for organic feel
        int updates = max(1, numLeds / 5);
        for (int u = 0; u < updates; u++) {
            int idx = random(numLeds);
            // Target a new random flicker value, biased warm
            uint8_t target = (uint8_t)random(80, 255);
            // Smooth towards target (don't jump instantly)
            flickerValues[idx] = lerp8(flickerValues[idx], target, 0.4f);
        }
    }

    strip.setBrightness(brightness);

    int numLeds = strip.numPixels();
    for (int i = 0; i < numLeds; i++) {
        float f = (float)flickerValues[i] / 255.0f;
        // Warm amber/orange palette
        uint8_t r = (uint8_t)(255 * f);
        uint8_t g = (uint8_t)(120 * f * f);   // Less green at lower flicker
        uint8_t b = (uint8_t)(15 * f * f * f); // Almost no blue
        uint8_t wLed = (uint8_t)(w * f * 0.8f); // Warm white glow
        strip.setPixelColor(i, strip.Color(r, g, b, wLed));
    }
    strip.show();
}

// ─── 6. Sunrise ─────────────────────────────────────────────────────────────
// Gradual warm-up from deep red through amber to warm white.
// Uses a 30-minute cycle by default; when driven by a schedule slot,
// the caller can map slot progress to time.
void effectSunrise(Adafruit_NeoPixel &strip,
                   uint8_t w, uint8_t brightness, unsigned long now) {
    // 30-minute repeating cycle
    const unsigned long cycleMs = 30UL * 60UL * 1000UL;
    float progress = (float)(now % cycleMs) / (float)cycleMs;  // 0.0 → 1.0

    // Brightness ramps up from 5% to full over the cycle
    uint8_t minBri = (uint8_t)(brightness * 0.05f);
    uint8_t actualBri = minBri + (uint8_t)((brightness - minBri) * progress);
    strip.setBrightness(actualBri);

    // Colour transitions: deep red → amber → warm white
    uint8_t r, g, b;
    uint8_t wLed;
    if (progress < 0.33f) {
        // Phase 1: deep red to orange, white off
        float t = progress / 0.33f;
        r = lerp8(120, 255, t);
        g = lerp8(0, 80, t);
        b = 0;
        wLed = 0;
    } else if (progress < 0.66f) {
        // Phase 2: orange to warm amber, white ramps to 30%
        float t = (progress - 0.33f) / 0.33f;
        r = 255;
        g = lerp8(80, 180, t);
        b = lerp8(0, 30, t);
        wLed = (uint8_t)(w * 0.3f * t);
    } else {
        // Phase 3: warm amber to warm white, white ramps to full
        float t = (progress - 0.66f) / 0.33f;
        r = 255;
        g = lerp8(180, 230, t);
        b = lerp8(30, 120, t);
        wLed = lerp8((uint8_t)(w * 0.3f), w, t);
    }

    uint32_t colour = strip.Color(r, g, b, wLed);
    int numLeds = strip.numPixels();
    for (int i = 0; i < numLeds; i++) {
        strip.setPixelColor(i, colour);
    }
    strip.show();
}

// ─── 7. Christmas ───────────────────────────────────────────────────────────
// Red/green alternating LEDs with sinusoidal breathing
void effectChristmas(Adafruit_NeoPixel &strip,
                     uint8_t brightness, unsigned long now) {
    float phase = (float)now / BREATHING_CYCLE_MS * 2.0f * PI;
    float factor = (sinf(phase) + 1.0f) / 2.0f;

    uint8_t minBri = (uint8_t)(brightness * 0.3f);
    uint8_t actualBri = minBri + (uint8_t)((brightness - minBri) * factor);
    strip.setBrightness(actualBri);

    // Slow shift offset so the pattern moves along the strip
    int offset = (int)(now / 500) % 4;
    int numLeds = strip.numPixels();

    for (int i = 0; i < numLeds; i++) {
        int pos = (i + offset) % 4;
        if (pos < 2) {
            // Red
            strip.setPixelColor(i, strip.Color(255, 0, 0, 0));
        } else {
            // Green
            strip.setPixelColor(i, strip.Color(0, 200, 0, 0));
        }
    }
    strip.show();
}

// ─── 8. Halloween ───────────────────────────────────────────────────────────
// Orange/purple alternating with candle-like flicker
void effectHalloween(Adafruit_NeoPixel &strip,
                     uint8_t brightness, unsigned long now) {
    static uint8_t flickerValues[NUM_LEDS];
    static unsigned long lastFlicker = 0;
    static bool initialised = false;

    if (!initialised) {
        for (int i = 0; i < NUM_LEDS; i++) {
            flickerValues[i] = (uint8_t)random(150, 255);
        }
        initialised = true;
    }

    if (now - lastFlicker >= CANDLE_FLICKER_MS) {
        lastFlicker = now;
        int numLeds = strip.numPixels();
        int updates = max(1, numLeds / 4);
        for (int u = 0; u < updates; u++) {
            int idx = random(numLeds);
            uint8_t target = (uint8_t)random(120, 255);
            flickerValues[idx] = lerp8(flickerValues[idx], target, 0.3f);
        }
    }

    strip.setBrightness(brightness);
    int numLeds = strip.numPixels();

    for (int i = 0; i < numLeds; i++) {
        float f = (float)flickerValues[i] / 255.0f;
        if (i % 3 == 0) {
            // Purple
            uint8_t r = (uint8_t)(120 * f);
            uint8_t g = 0;
            uint8_t b = (uint8_t)(200 * f);
            strip.setPixelColor(i, strip.Color(r, g, b, 0));
        } else {
            // Orange
            uint8_t r = (uint8_t)(255 * f);
            uint8_t g = (uint8_t)(100 * f * f);
            uint8_t b = 0;
            strip.setPixelColor(i, strip.Color(r, g, b, 0));
        }
    }
    strip.show();
}

// ─── 9. Birthday ────────────────────────────────────────────────────────────
// Rainbow base colours with sparkle twinkles
void effectBirthday(Adafruit_NeoPixel &strip,
                    uint8_t brightness, unsigned long now) {
    static uint8_t sparkle[NUM_LEDS];
    static bool initialised = false;

    if (!initialised) {
        memset(sparkle, 0, sizeof(sparkle));
        initialised = true;
    }

    strip.setBrightness(brightness);

    uint8_t offset = (uint8_t)((now / 80) & 0xFF);
    int numLeds = strip.numPixels();

    for (int i = 0; i < numLeds; i++) {
        // Slowly rotating rainbow base
        uint8_t pos = (uint8_t)((i * 256 / numLeds + offset) & 0xFF);
        uint8_t r, g, b;
        wheelRGB(pos, r, g, b);

        // Random sparkle: bright white flash
        if (sparkle[i] == 0 && (int)random(100) < 3) {
            sparkle[i] = 255;
        }

        if (sparkle[i] > 0) {
            float t = (float)sparkle[i] / 255.0f;
            r = lerp8(r, 255, t);
            g = lerp8(g, 255, t);
            b = lerp8(b, 255, t);
            strip.setPixelColor(i, strip.Color(r, g, b, (uint8_t)(sparkle[i])));

            if (sparkle[i] > 12) {
                sparkle[i] -= 12;
            } else {
                sparkle[i] = 0;
            }
        } else {
            strip.setPixelColor(i, strip.Color(r, g, b, 0));
        }
    }
    strip.show();
}

// ─── Effect Dispatcher ──────────────────────────────────────────────────────
void applyEffect(Adafruit_NeoPixel &strip, EffectType effect,
                 uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                 uint8_t brightness, unsigned long now) {
    // Apply thermal throttling and master fade to brightness
    brightness = (uint8_t)(brightness * thermalDimFactor * masterFadeFactor);

    switch (effect) {
        case EFFECT_SOLID:
            effectSolid(strip, r, g, b, w, brightness);
            break;
        case EFFECT_BREATHING:
            effectBreathing(strip, r, g, b, w, brightness, now);
            break;
        case EFFECT_SOFT_GLOW:
            effectSoftGlow(strip, r, g, b, w, brightness, now);
            break;
        case EFFECT_STARRY:
            effectStarryTwinkle(strip, r, g, b, w, brightness, now);
            break;
        case EFFECT_RAINBOW:
            effectRainbow(strip, brightness, now);
            break;
        case EFFECT_CANDLE:
            effectCandleFlicker(strip, w, brightness, now);
            break;
        case EFFECT_SUNRISE:
            effectSunrise(strip, w, brightness, now);
            break;
        case EFFECT_CHRISTMAS:
            effectChristmas(strip, brightness, now);
            break;
        case EFFECT_HALLOWEEN:
            effectHalloween(strip, brightness, now);
            break;
        case EFFECT_BIRTHDAY:
            effectBirthday(strip, brightness, now);
            break;
        default:
            effectSolid(strip, r, g, b, w, brightness);
            break;
    }
}

// ─── Boot Animation ────────────────────────────────────────────────────────
// Blocking rainbow wave with brightness ramp-up.  Runs once during setup()
// so the LEDs light up immediately on power-on.
void playBootAnimation(Adafruit_NeoPixel &strip, unsigned long durationMs) {
    unsigned long start = millis();
    int numLeds = strip.numPixels();
    // Ramp brightness from 0→255 over the first 1s
    const unsigned long rampMs = 1000;
    // Full rainbow cycle time — wave travels the strip in this period
    const unsigned long cycleMs = 2000;

    while (true) {
        esp_task_wdt_reset();  // keep watchdog happy

        unsigned long now     = millis();
        unsigned long elapsed = now - start;
        if (elapsed >= durationMs) break;

        // Brightness ramp-up then hold
        uint8_t bri = (elapsed < rampMs)
                       ? (uint8_t)(255UL * elapsed / rampMs)
                       : 255;
        strip.setBrightness(bri);

        // Offset moves the rainbow along the strip
        uint8_t offset = (uint8_t)((elapsed * 256UL / cycleMs) & 0xFF);

        for (int i = 0; i < numLeds; i++) {
            uint8_t pos = (uint8_t)((i * 256 / numLeds + offset) & 0xFF);
            strip.setPixelColor(i, wheel(strip, pos));
        }
        strip.show();

        delay(LED_UPDATE_INTERVAL);
    }
}

// ─── Transition Between Slots ───────────────────────────────────────────────
// Linear interpolation of colour and brightness between two slots.
// If brightnessOverride >= 0, that value is used instead of lerping.
void applyTransition(Adafruit_NeoPixel &strip,
                     const ScheduleSlot &from, const ScheduleSlot &to,
                     float progress, unsigned long now,
                     int16_t brightnessOverride) {
    uint8_t r   = lerp8(from.red,        to.red,        progress);
    uint8_t g   = lerp8(from.green,      to.green,      progress);
    uint8_t b   = lerp8(from.blue,       to.blue,       progress);
    uint8_t w   = lerp8(from.white,      to.white,      progress);
    uint8_t bri = (brightnessOverride >= 0)
                  ? (uint8_t)brightnessOverride
                  : lerp8(from.brightness, to.brightness, progress);

    // Use the destination effect once past 50% transition
    EffectType effect = (progress < 0.5f)
                        ? (EffectType)from.effect
                        : (EffectType)to.effect;

    applyEffect(strip, effect, r, g, b, w, bri, now);
}
