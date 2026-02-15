#include "tetris_effect.h"
#include "led_effects.h"

// ─── Tetromino Definitions ─────────────────────────────────────────────────

#define NUM_PIECES  7

static const uint32_t PIECE_COLOURS[NUM_PIECES] = {
    Adafruit_NeoPixel::Color(0,   240, 240),  // I — cyan
    Adafruit_NeoPixel::Color(240, 240, 0),     // O — yellow
    Adafruit_NeoPixel::Color(160, 0,   240),   // T — purple
    Adafruit_NeoPixel::Color(0,   240, 0),     // S — green
    Adafruit_NeoPixel::Color(240, 0,   0),     // Z — red
    Adafruit_NeoPixel::Color(240, 60, 150),    // L — pink
    Adafruit_NeoPixel::Color(0,   0,   240),   // J — blue
};

// 4 rotations per piece, 16-bit bitmask (4x4 grid, MSB = top-left)
static const uint16_t SHAPES[NUM_PIECES][4] = {
    {0x0F00, 0x2222, 0x00F0, 0x4444},  // I
    {0x6600, 0x6600, 0x6600, 0x6600},  // O
    {0x4E00, 0x4640, 0x0E40, 0x4C40},  // T
    {0x6C00, 0x4620, 0x06C0, 0x8C40},  // S
    {0xC600, 0x2640, 0x0C60, 0x4C80},  // Z
    {0x2E00, 0x4460, 0x0E80, 0xC440},  // L
    {0x8E00, 0x6440, 0x0E20, 0x44C0},  // J
};

// ─── Runtime Config ──────────────────────────────────────────────────────────

static uint16_t cfgDropStartMs   = 300;
static uint16_t cfgDropMinMs     = 80;
static uint16_t cfgMoveIntervalMs = 70;
static uint16_t cfgRotIntervalMs = 150;
static uint8_t  cfgAiSkillPct    = 90;
static uint8_t  cfgJitterPct     = 20;
static uint32_t cfgBgColour      = 0;

// ─── State ──────────────────────────────────────────────────────────────────

static uint32_t board[GRID_HEIGHT][GRID_WIDTH];

// Current piece
static uint8_t  pieceType;
static uint8_t  pieceRot;
static int8_t   pieceX, pieceY;

// AI target
static int8_t   targetX;
static uint8_t  targetRot;
static bool     reachedTarget;
static int8_t   rotDir;
static uint8_t  rotStepsLeft;   // Rotation steps remaining (shortest path)
static bool     aiThinking;     // Reaction delay before first action
static unsigned long thinkStartMs;

// Manual mode
static bool     manualActive = false;
static bool     softDropActive = false;

// Timing
static unsigned long lastDropMs;
static unsigned long lastMoveMs;
static unsigned long lastRotMs;
static uint16_t dropIntervalMs;
static uint16_t piecesPlaced;

// Score tracking
static uint16_t totalScore = 0;
static uint16_t totalLines = 0;

// Row clearing
static bool     clearing;
static uint8_t  clearRows[GRID_HEIGHT];
static uint8_t  numClearRows;
static unsigned long clearStartMs;
#define CLEAR_FLASH_MS  400

// Game over
static bool     gameOver;
static unsigned long gameOverStartMs;
#define GAME_OVER_FLASH_MS  1500

// ─── Shape Helpers ──────────────────────────────────────────────────────────

static bool shapeCell(uint8_t type, uint8_t rot, uint8_t row, uint8_t col) {
    return (SHAPES[type][rot] >> (15 - (row * 4 + col))) & 1;
}

static bool pieceFits(uint8_t type, uint8_t rot, int8_t px, int8_t py) {
    for (uint8_t r = 0; r < 4; r++) {
        for (uint8_t c = 0; c < 4; c++) {
            if (!shapeCell(type, rot, r, c)) continue;
            int8_t bx = px + c;
            int8_t by = py + r;
            if (bx < 0 || bx >= GRID_WIDTH || by >= GRID_HEIGHT) return false;
            if (by >= 0 && board[by][bx] != 0) return false;
        }
    }
    return true;
}

static int8_t hardDropY(uint8_t type, uint8_t rot, int8_t px) {
    int8_t py = -2;
    while (pieceFits(type, rot, px, py + 1)) {
        py++;
    }
    return py;
}

// ─── AI: Board Scoring ─────────────────────────────────────────────────────

static float scorePlacement(uint8_t type, uint8_t rot, int8_t px) {
    int8_t py = hardDropY(type, rot, px);

    uint8_t placed = 0;
    int8_t placedCells[8];
    for (uint8_t r = 0; r < 4; r++) {
        for (uint8_t c = 0; c < 4; c++) {
            if (!shapeCell(type, rot, r, c)) continue;
            int8_t bx = px + c;
            int8_t by = py + r;
            if (bx >= 0 && bx < GRID_WIDTH && by >= 0 && by < GRID_HEIGHT) {
                board[by][bx] = 1;
                placedCells[placed * 2]     = bx;
                placedCells[placed * 2 + 1] = by;
                placed++;
            }
        }
    }

    int colHeights[GRID_WIDTH];
    int aggregateHeight = 0;
    for (uint8_t x = 0; x < GRID_WIDTH; x++) {
        colHeights[x] = 0;
        for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
            if (board[y][x] != 0) {
                colHeights[x] = GRID_HEIGHT - y;
                break;
            }
        }
        aggregateHeight += colHeights[x];
    }

    int completedLines = 0;
    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        bool full = true;
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            if (board[y][x] == 0) { full = false; break; }
        }
        if (full) completedLines++;
    }

    int holes = 0;
    for (uint8_t x = 0; x < GRID_WIDTH; x++) {
        bool foundFilled = false;
        for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
            if (board[y][x] != 0) foundFilled = true;
            else if (foundFilled) holes++;
        }
    }

    int bumpiness = 0;
    for (uint8_t x = 0; x < GRID_WIDTH - 1; x++) {
        int diff = colHeights[x] - colHeights[x + 1];
        bumpiness += (diff < 0) ? -diff : diff;
    }

    for (uint8_t i = 0; i < placed; i++) {
        board[placedCells[i * 2 + 1]][placedCells[i * 2]] = 0;
    }

    return -0.35f * aggregateHeight
           + 1.40f * completedLines
           - 0.50f * holes
           - 0.15f * bumpiness;
}

// ─── AI: Choose Placement ──────────────────────────────────────────────────

struct Placement {
    int8_t  x;
    uint8_t rot;
    float   score;
};

static void aiChoosePlacement(uint8_t type) {
    Placement best;
    best.score = -999999.0f;
    best.x = (GRID_WIDTH / 2) - 2;
    best.rot = 0;

    #define AI_TOP_N  5
    Placement topN[AI_TOP_N];
    uint8_t topCount = 0;

    for (uint8_t rot = 0; rot < 4; rot++) {
        for (int8_t px = -2; px < (int8_t)GRID_WIDTH; px++) {
            if (!pieceFits(type, rot, px, -2)) continue;
            int8_t landY = hardDropY(type, rot, px);
            if (landY < -1) continue;

            float s = scorePlacement(type, rot, px);
            s += (float)random(-15, 16) / 100.0f;

            if (s > best.score) {
                best.score = s;
                best.x = px;
                best.rot = rot;
            }

            if (topCount < AI_TOP_N) {
                topN[topCount++] = {px, rot, s};
            } else {
                uint8_t worstIdx = 0;
                for (uint8_t i = 1; i < AI_TOP_N; i++) {
                    if (topN[i].score < topN[worstIdx].score) worstIdx = i;
                }
                if (s > topN[worstIdx].score) {
                    topN[worstIdx] = {px, rot, s};
                }
            }
        }
    }

    // Use cfgAiSkillPct to determine optimal vs random pick
    uint8_t randPct = 100 - cfgAiSkillPct;
    if (random(100) < randPct && topCount > 1) {
        uint8_t pick = random(topCount);
        targetX = topN[pick].x;
        targetRot = topN[pick].rot;
    } else {
        targetX = best.x;
        targetRot = best.rot;
    }
}

// ─── Piece Lifecycle ────────────────────────────────────────────────────────

static void lockPiece() {
    for (uint8_t r = 0; r < 4; r++) {
        for (uint8_t c = 0; c < 4; c++) {
            if (!shapeCell(pieceType, pieceRot, r, c)) continue;
            int8_t bx = pieceX + c;
            int8_t by = pieceY + r;
            if (bx >= 0 && bx < GRID_WIDTH && by >= 0 && by < GRID_HEIGHT) {
                board[by][bx] = PIECE_COLOURS[pieceType];
            }
        }
    }
    piecesPlaced++;
}

static uint8_t findFullRows() {
    numClearRows = 0;
    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        bool full = true;
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            if (board[y][x] == 0) { full = false; break; }
        }
        if (full) {
            clearRows[numClearRows++] = y;
        }
    }
    return numClearRows;
}

static void removeRows() {
    totalLines += numClearRows;
    totalScore += numClearRows * numClearRows * 100;  // 1=100, 2=400, 3=900, 4=1600

    for (int8_t i = numClearRows - 1; i >= 0; i--) {
        uint8_t row = clearRows[i];
        for (int8_t y = row; y > 0; y--) {
            for (uint8_t x = 0; x < GRID_WIDTH; x++) {
                board[y][x] = board[y - 1][x];
            }
        }
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            board[0][x] = 0;
        }
    }
}

static void spawnPiece() {
    pieceType = random(NUM_PIECES);
    pieceX = (GRID_WIDTH / 2) - 2;
    pieceY = -1;
    reachedTarget = false;
    softDropActive = false;

    if (manualActive) {
        pieceRot = 0;
        rotStepsLeft = 0;
        aiThinking = false;
    } else {
        // Always spawn at rotation 0 (natural, like a real game)
        pieceRot = 0;

        // AI decides where to place
        aiChoosePlacement(pieceType);

        // Shortest-path rotation (0-2 steps, like a human would do)
        uint8_t cwDist  = (targetRot - pieceRot + 4) % 4;
        uint8_t ccwDist = (pieceRot - targetRot + 4) % 4;
        if (cwDist <= ccwDist) {
            rotDir = 1;
            rotStepsLeft = cwDist;
        } else {
            rotDir = -1;
            rotStepsLeft = ccwDist;
        }

        // Human-like reaction delay: piece drops a couple of rows
        // before the "player" starts moving/rotating (150-500ms)
        aiThinking = true;
        thinkStartMs = millis();
    }

    if (!pieceFits(pieceType, pieceRot, pieceX, pieceY)) {
        gameOver = true;
        gameOverStartMs = millis();
    }
}

// ─── Rendering ──────────────────────────────────────────────────────────────

static void render(Adafruit_NeoPixel &strip) {
    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            uint32_t c = board[y][x];
            strip.setPixelColor(xyToIndex(x, y), c != 0 ? c : cfgBgColour);
        }
    }

    // Draw the falling piece
    if (!clearing && !gameOver) {
        for (uint8_t r = 0; r < 4; r++) {
            for (uint8_t c = 0; c < 4; c++) {
                if (!shapeCell(pieceType, pieceRot, r, c)) continue;
                int8_t bx = pieceX + c;
                int8_t by = pieceY + r;
                if (bx >= 0 && bx < GRID_WIDTH && by >= 0 && by < GRID_HEIGHT) {
                    strip.setPixelColor(xyToIndex(bx, by), PIECE_COLOURS[pieceType]);
                }
            }
        }
    }

    strip.show();
}

// ─── Public API ─────────────────────────────────────────────────────────────

void setTetrisConfig(const GridConfig &cfg) {
    cfgDropStartMs    = constrain(cfg.dropStartMs, 100, 600);
    cfgDropMinMs      = constrain(cfg.dropMinMs, 30, 200);
    cfgMoveIntervalMs = constrain(cfg.moveIntervalMs, 20, 200);
    cfgRotIntervalMs  = constrain(cfg.rotIntervalMs, 50, 300);
    cfgAiSkillPct     = constrain(cfg.aiSkillPct, 0, 100);
    cfgJitterPct      = constrain(cfg.jitterPct, 0, 50);
    cfgBgColour       = Adafruit_NeoPixel::Color(cfg.bgR, cfg.bgG, cfg.bgB);
}

void resetTetris() {
    memset(board, 0, sizeof(board));
    clearing = false;
    gameOver = false;
    numClearRows = 0;
    piecesPlaced = 0;
    totalScore = 0;
    totalLines = 0;
    dropIntervalMs = cfgDropStartMs;
    lastDropMs = millis();
    lastMoveMs = millis();
    lastRotMs = millis();
    spawnPiece();
}

void setManualMode(bool enabled) {
    if (manualActive != enabled) {
        manualActive = enabled;
        resetTetris();
    }
}

bool isManualMode() {
    return manualActive;
}

bool manualMoveLeft() {
    if (!manualActive || clearing || gameOver) return false;
    if (pieceFits(pieceType, pieceRot, pieceX - 1, pieceY)) {
        pieceX--;
        return true;
    }
    return false;
}

bool manualMoveRight() {
    if (!manualActive || clearing || gameOver) return false;
    if (pieceFits(pieceType, pieceRot, pieceX + 1, pieceY)) {
        pieceX++;
        return true;
    }
    return false;
}

bool manualRotate() {
    if (!manualActive || clearing || gameOver) return false;
    uint8_t newRot = (pieceRot + 1) % 4;
    if (pieceFits(pieceType, newRot, pieceX, pieceY)) {
        pieceRot = newRot;
        return true;
    }
    return false;
}

void manualHardDrop() {
    if (!manualActive || clearing || gameOver) return;
    while (pieceFits(pieceType, pieceRot, pieceX, pieceY + 1)) {
        pieceY++;
    }
    // Force immediate lock on next frame
    lastDropMs = 0;
}

void manualSoftDrop(bool active) {
    softDropActive = active;
}

void getTetrisState(uint32_t *gridOut, uint8_t &pType, int8_t &px, int8_t &py,
                    uint8_t &rot, uint16_t &score, uint16_t &lines,
                    bool &over, bool &clr) {
    // Copy board state
    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            uint32_t c = board[y][x];
            gridOut[y * GRID_WIDTH + x] = c != 0 ? c : cfgBgColour;
        }
    }
    // Draw current piece onto the grid copy
    if (!clearing && !gameOver) {
        for (uint8_t r = 0; r < 4; r++) {
            for (uint8_t c = 0; c < 4; c++) {
                if (!shapeCell(pieceType, pieceRot, r, c)) continue;
                int8_t bx = pieceX + c;
                int8_t by = pieceY + r;
                if (bx >= 0 && bx < GRID_WIDTH && by >= 0 && by < GRID_HEIGHT) {
                    gridOut[by * GRID_WIDTH + bx] = PIECE_COLOURS[pieceType];
                }
            }
        }
    }
    pType = pieceType;
    px = pieceX;
    py = pieceY;
    rot = pieceRot;
    score = totalScore;
    lines = totalLines;
    over = gameOver;
    clr = clearing;
}

void updateTetris(Adafruit_NeoPixel &strip) {
    unsigned long now = millis();

    // ── Game over: flash then reset ──
    if (gameOver) {
        bool flashOn = ((now - gameOverStartMs) / 200) & 1;
        for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
            for (uint8_t x = 0; x < GRID_WIDTH; x++) {
                if (flashOn && board[y][x] != 0) {
                    strip.setPixelColor(xyToIndex(x, y),
                        Adafruit_NeoPixel::Color(255, 255, 255));
                } else {
                    uint32_t c = board[y][x];
                    strip.setPixelColor(xyToIndex(x, y), c != 0 ? c : cfgBgColour);
                }
            }
        }
        strip.show();
        if (now - gameOverStartMs >= GAME_OVER_FLASH_MS) {
            resetTetris();
        }
        return;
    }

    // ── Row clearing animation — rainbow sweep then fade ──
    if (clearing) {
        unsigned long elapsed = now - clearStartMs;
        // Rainbow sweeps across for first 300ms, then fades out
        uint8_t fade = (elapsed < CLEAR_FLASH_MS)
            ? 255 - (uint8_t)(elapsed * 255UL / CLEAR_FLASH_MS)
            : 0;
        uint8_t hueOffset = (uint8_t)(elapsed / 2);  // Fast hue rotation

        for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
            for (uint8_t x = 0; x < GRID_WIDTH; x++) {
                bool isClearing = false;
                for (uint8_t i = 0; i < numClearRows; i++) {
                    if (clearRows[i] == y) { isClearing = true; break; }
                }
                if (isClearing) {
                    uint8_t hue = hueOffset + x * 16;
                    uint32_t rainbow = colourWheel(hue);
                    uint8_t r = (uint8_t)((rainbow >> 16) * fade >> 8);
                    uint8_t g = (uint8_t)(((rainbow >> 8) & 0xFF) * fade >> 8);
                    uint8_t b = (uint8_t)((rainbow & 0xFF) * fade >> 8);
                    strip.setPixelColor(xyToIndex(x, y),
                        Adafruit_NeoPixel::Color(r, g, b));
                } else {
                    uint32_t c = board[y][x];
                    strip.setPixelColor(xyToIndex(x, y), c != 0 ? c : cfgBgColour);
                }
            }
        }
        strip.show();
        if (elapsed >= CLEAR_FLASH_MS) {
            removeRows();
            clearing = false;
            spawnPiece();
            lastDropMs = now;
            lastMoveMs = now;
        }
        return;
    }

    // ── AI mode: human-like rotate and slide toward target ──
    if (!manualActive) {
        // Reaction delay — piece must be on-screen and "thinking" time elapsed
        if (aiThinking) {
            unsigned long thinkMs = 150 + (esp_random() % 350);  // 150-500ms
            if (pieceY >= 2 && now - thinkStartMs >= thinkMs) {
                aiThinking = false;
            }
        }

        // Rotate and move happen simultaneously (like a real player)
        if (!aiThinking) {
            // Rotate via shortest path
            if (rotStepsLeft > 0 && now - lastRotMs >= cfgRotIntervalMs) {
                lastRotMs = now;
                uint8_t nextRot = (pieceRot + rotDir + 4) % 4;
                if (pieceFits(pieceType, nextRot, pieceX, pieceY)) {
                    pieceRot = nextRot;
                    rotStepsLeft--;
                } else {
                    rotStepsLeft = 0;
                }
            }
        }

        if (!aiThinking && !reachedTarget && now - lastMoveMs >= cfgMoveIntervalMs) {
            lastMoveMs = now;
            if (random(100) < (int)cfgJitterPct) {
                // Hesitate — skip this tick
            } else if (pieceX < targetX) {
                if (pieceFits(pieceType, pieceRot, pieceX + 1, pieceY)) {
                    pieceX++;
                } else {
                    reachedTarget = true;
                }
            } else if (pieceX > targetX) {
                if (pieceFits(pieceType, pieceRot, pieceX - 1, pieceY)) {
                    pieceX--;
                } else {
                    reachedTarget = true;
                }
            } else {
                reachedTarget = true;
            }
        }
    }

    // ── Drop the piece ──
    uint16_t effectiveDropMs = softDropActive ? cfgDropMinMs : dropIntervalMs;
    if (now - lastDropMs >= effectiveDropMs) {
        lastDropMs = now;

        if (pieceFits(pieceType, pieceRot, pieceX, pieceY + 1)) {
            pieceY++;
        } else {
            lockPiece();

            if (findFullRows() > 0) {
                clearing = true;
                clearStartMs = now;
            } else {
                spawnPiece();
                lastMoveMs = now;
            }

            // Speed up every 10 pieces
            if (piecesPlaced % 10 == 0 && dropIntervalMs > cfgDropMinMs) {
                dropIntervalMs -= 25;
                if (dropIntervalMs < cfgDropMinMs) dropIntervalMs = cfgDropMinMs;
            }
        }
    }

    render(strip);
}
