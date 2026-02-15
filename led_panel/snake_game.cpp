#include "snake_game.h"
#include "led_effects.h"

// ─── Direction Constants ───────────────────────────────────────────────────
#define DIR_UP    0
#define DIR_RIGHT 1
#define DIR_DOWN  2
#define DIR_LEFT  3

static const int8_t DX[4] = {0, 1, 0, -1};
static const int8_t DY[4] = {-1, 0, 1, 0};

// ─── Snake State ───────────────────────────────────────────────────────────
#define MAX_SNAKE_LEN 256

static uint8_t bodyX[MAX_SNAKE_LEN];
static uint8_t bodyY[MAX_SNAKE_LEN];
static uint16_t headIdx = 0;
static uint16_t snakeLen = 3;
static uint8_t direction = DIR_RIGHT;
static uint8_t nextDirection = DIR_RIGHT;

// Food
static uint8_t foodX = 12;
static uint8_t foodY = 8;

// Game state
static uint16_t snakeScore = 0;
static bool gameOver = false;
static uint32_t lastMoveMs = 0;
static uint32_t gameOverMs = 0;
static bool manualMode = false;

// Occupied grid — bitfield for fast collision detection (32 bytes)
static uint16_t occupied[GRID_HEIGHT];

// Config
static uint8_t bgR = 0, bgG = 0, bgB = 0;

// ─── Forward Declarations ──────────────────────────────────────────────────
static void rebuildOccupied();
static void placeFood();
static uint8_t aiChoose();
static uint16_t floodCount(uint8_t sx, uint8_t sy);

// ─── Public API ────────────────────────────────────────────────────────────

void resetSnake() {
    headIdx = 2;
    snakeLen = 3;
    direction = DIR_RIGHT;
    nextDirection = DIR_RIGHT;
    snakeScore = 0;
    gameOver = false;
    lastMoveMs = millis();

    // Start in middle, heading right
    bodyX[0] = 6; bodyY[0] = 8;  // tail
    bodyX[1] = 7; bodyY[1] = 8;  // mid
    bodyX[2] = 8; bodyY[2] = 8;  // head

    rebuildOccupied();
    placeFood();
}

void setSnakeConfig(const GridConfig &cfg) {
    bgR = cfg.bgR;
    bgG = cfg.bgG;
    bgB = cfg.bgB;
}

void setSnakeManualMode(bool enabled) {
    manualMode = enabled;
}

bool isSnakeManualMode() {
    return manualMode;
}

void snakeSetDirection(uint8_t dir) {
    if (dir > 3) return;
    // Can't reverse direction
    if ((dir + 2) % 4 == direction) return;
    nextDirection = dir;
}

// ─── Occupied Grid ─────────────────────────────────────────────────────────

static void rebuildOccupied() {
    memset(occupied, 0, sizeof(occupied));
    for (uint16_t i = 0; i < snakeLen; i++) {
        uint16_t idx = (headIdx - i + MAX_SNAKE_LEN) % MAX_SNAKE_LEN;
        occupied[bodyY[idx]] |= (1 << bodyX[idx]);
    }
}

static bool isOccupied(uint8_t x, uint8_t y) {
    if (x >= GRID_WIDTH || y >= GRID_HEIGHT) return true;
    return (occupied[y] >> x) & 1;
}

// ─── Food Placement ────────────────────────────────────────────────────────

static void placeFood() {
    uint16_t empty = GRID_WIDTH * GRID_HEIGHT - snakeLen;
    if (empty == 0) {
        // Snake fills entire grid — victory!
        gameOver = true;
        gameOverMs = millis();
        return;
    }
    // Pick a random empty cell
    uint16_t target = random(empty);
    uint16_t count = 0;
    for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint8_t x = 0; x < GRID_WIDTH; x++) {
            if (!isOccupied(x, y)) {
                if (count == target) {
                    foodX = x;
                    foodY = y;
                    return;
                }
                count++;
            }
        }
    }
}

// ─── AI: Flood-Fill Safety Check ───────────────────────────────────────────
// BFS from (sx, sy) counting reachable empty cells.

static uint16_t floodCount(uint8_t sx, uint8_t sy) {
    if (sx >= GRID_WIDTH || sy >= GRID_HEIGHT) return 0;
    if (isOccupied(sx, sy)) return 0;

    // BFS queue (max 256 entries = full grid)
    static uint8_t qx[256], qy[256];
    uint16_t visited[GRID_HEIGHT];
    memcpy(visited, occupied, sizeof(occupied));

    uint16_t qHead = 0, qTail = 0;
    qx[qTail] = sx; qy[qTail] = sy; qTail++;
    visited[sy] |= (1 << sx);
    uint16_t count = 0;

    while (qHead < qTail) {
        uint8_t cx = qx[qHead]; uint8_t cy = qy[qHead]; qHead++;
        count++;

        for (uint8_t d = 0; d < 4; d++) {
            int8_t nx = (int8_t)cx + DX[d];
            int8_t ny = (int8_t)cy + DY[d];
            if (nx < 0 || nx >= GRID_WIDTH || ny < 0 || ny >= GRID_HEIGHT) continue;
            if ((visited[ny] >> nx) & 1) continue;
            visited[ny] |= (1 << nx);
            qx[qTail] = (uint8_t)nx; qy[qTail] = (uint8_t)ny; qTail++;
        }
    }
    return count;
}

// ─── AI: Direction Choice ──────────────────────────────────────────────────
// Greedy toward food, with flood-fill to avoid trapping itself.

static uint8_t aiChoose() {
    uint8_t hx = bodyX[headIdx];
    uint8_t hy = bodyY[headIdx];

    // Temporarily clear the tail from occupied (it will move away this step)
    uint16_t tailIdx = (headIdx - snakeLen + 1 + MAX_SNAKE_LEN) % MAX_SNAKE_LEN;
    uint8_t tailX = bodyX[tailIdx];
    uint8_t tailY = bodyY[tailIdx];
    occupied[tailY] &= ~(1 << tailX);

    int16_t bestScore = -9999;
    uint8_t bestDir = direction;

    for (uint8_t d = 0; d < 4; d++) {
        // Can't reverse
        if ((d + 2) % 4 == direction) continue;

        int8_t nx = (int8_t)hx + DX[d];
        int8_t ny = (int8_t)hy + DY[d];

        // Wall check
        if (nx < 0 || nx >= GRID_WIDTH || ny < 0 || ny >= GRID_HEIGHT) continue;

        // Body check (tail already removed from occupied)
        if (isOccupied((uint8_t)nx, (uint8_t)ny)) continue;

        // Score this direction
        int16_t dirScore = 0;

        // Manhattan distance to food (prefer closer)
        int16_t dist = abs((int16_t)nx - (int16_t)foodX) +
                       abs((int16_t)ny - (int16_t)foodY);
        dirScore -= dist * 10;

        // Flood-fill: prefer directions with more reachable space
        uint16_t reachable = floodCount((uint8_t)nx, (uint8_t)ny);

        if (reachable < snakeLen) {
            // Might trap ourselves — heavy penalty
            dirScore -= 5000;
        } else {
            dirScore += (int16_t)(reachable / 4);
        }

        // Slight bias for continuing straight
        if (d == direction) dirScore += 5;

        if (dirScore > bestScore) {
            bestScore = dirScore;
            bestDir = d;
        }
    }

    // Restore tail in occupied grid
    occupied[tailY] |= (1 << tailX);

    return bestDir;
}

// ─── Move Speed ────────────────────────────────────────────────────────────

static uint16_t getMoveInterval() {
    // Start at 280ms, decrease by 6ms per food eaten, min 80ms
    int16_t interval = 280 - (int16_t)snakeScore * 6;
    if (interval < 80) interval = 80;
    return (uint16_t)interval;
}

// ─── Update / Render ───────────────────────────────────────────────────────

void updateSnake(Adafruit_NeoPixel &strip) {
    uint32_t now = millis();

    // Handle game over — flash snake in red, restart after 3s
    if (gameOver) {
        bool flash = ((now - gameOverMs) / 300) % 2 == 0;
        strip.clear();
        if (!flash) {
            for (uint16_t i = 0; i < snakeLen; i++) {
                uint16_t idx = (headIdx - i + MAX_SNAKE_LEN) % MAX_SNAKE_LEN;
                strip.setPixelColor(xyToIndex(bodyX[idx], bodyY[idx]),
                    Adafruit_NeoPixel::Color(180, 0, 0));
            }
        }
        strip.show();

        if (now - gameOverMs > 3000) {
            resetSnake();
        }
        return;
    }

    // Move at the appropriate interval
    if (now - lastMoveMs >= getMoveInterval()) {
        lastMoveMs = now;

        // Choose direction
        if (!manualMode) {
            nextDirection = aiChoose();
        }
        direction = nextDirection;

        // Calculate new head position
        int8_t newX = (int8_t)bodyX[headIdx] + DX[direction];
        int8_t newY = (int8_t)bodyY[headIdx] + DY[direction];

        // Wall collision
        if (newX < 0 || newX >= GRID_WIDTH || newY < 0 || newY >= GRID_HEIGHT) {
            gameOver = true;
            gameOverMs = now;
            return;
        }

        // Self collision
        // Tail will move away this step (unless eating), so allow tail cell
        uint16_t tailRingIdx = (headIdx - snakeLen + 1 + MAX_SNAKE_LEN) % MAX_SNAKE_LEN;
        bool hittingTail = ((uint8_t)newX == bodyX[tailRingIdx] &&
                            (uint8_t)newY == bodyY[tailRingIdx]);

        if (isOccupied((uint8_t)newX, (uint8_t)newY)) {
            if (!hittingTail) {
                gameOver = true;
                gameOverMs = now;
                return;
            }
            // If hitting tail AND about to eat food (so tail won't move), also die
            if ((uint8_t)newX == foodX && (uint8_t)newY == foodY) {
                gameOver = true;
                gameOverMs = now;
                return;
            }
        }

        // Advance head
        headIdx = (headIdx + 1) % MAX_SNAKE_LEN;
        bodyX[headIdx] = (uint8_t)newX;
        bodyY[headIdx] = (uint8_t)newY;

        // Check food
        if ((uint8_t)newX == foodX && (uint8_t)newY == foodY) {
            snakeLen++;
            snakeScore++;
            rebuildOccupied();
            placeFood();
        } else {
            rebuildOccupied();
        }
    }

    // ── Render ──
    uint32_t bg = Adafruit_NeoPixel::Color(bgR, bgG, bgB);
    strip.fill(bg);

    // Draw snake body with colour gradient
    for (uint16_t i = 0; i < snakeLen; i++) {
        uint16_t idx = (headIdx - i + MAX_SNAKE_LEN) % MAX_SNAKE_LEN;
        uint8_t x = bodyX[idx];
        uint8_t y = bodyY[idx];

        if (i == 0) {
            // Head: bright white-green
            strip.setPixelColor(xyToIndex(x, y),
                Adafruit_NeoPixel::Color(200, 255, 200));
        } else {
            // Body gradient: bright green (near head) → dark teal (tail)
            uint8_t frac = (snakeLen > 1) ?
                (uint8_t)((uint32_t)i * 255 / (snakeLen - 1)) : 0;
            uint8_t r = 0;
            uint8_t g = 255 - (uint8_t)((uint16_t)frac * 175 / 255);  // 255→80
            uint8_t b = (uint8_t)((uint16_t)frac * 80 / 255);          // 0→80
            strip.setPixelColor(xyToIndex(x, y),
                Adafruit_NeoPixel::Color(r, g, b));
        }
    }

    // Draw food — pulsing red
    uint16_t phase = (now / 4) & 0xFF;
    uint8_t wave = (phase < 128) ? (uint8_t)(phase * 2) :
                                   (uint8_t)((255 - phase) * 2);
    uint8_t foodBright = 140 + (uint8_t)((uint16_t)wave * 115 / 255);
    strip.setPixelColor(xyToIndex(foodX, foodY),
        Adafruit_NeoPixel::Color(foodBright, 0, 0));

    strip.show();
}

// ─── State Query (for WebSocket broadcast) ─────────────────────────────────

void getSnakeState(uint32_t *gridOut, uint16_t &outScore, uint16_t &outLength,
                   bool &over) {
    memset(gridOut, 0, sizeof(uint32_t) * GRID_WIDTH * GRID_HEIGHT);

    if (!gameOver) {
        // Draw snake
        for (uint16_t i = 0; i < snakeLen; i++) {
            uint16_t idx = (headIdx - i + MAX_SNAKE_LEN) % MAX_SNAKE_LEN;
            uint8_t x = bodyX[idx];
            uint8_t y = bodyY[idx];
            uint16_t gIdx = y * GRID_WIDTH + x;

            if (i == 0) {
                gridOut[gIdx] = 0x00C8FFC8;  // white-green head
            } else {
                uint8_t frac = (snakeLen > 1) ?
                    (uint8_t)((uint32_t)i * 255 / (snakeLen - 1)) : 0;
                uint8_t g = 255 - (uint8_t)((uint16_t)frac * 175 / 255);
                uint8_t b = (uint8_t)((uint16_t)frac * 80 / 255);
                gridOut[gIdx] = ((uint32_t)g << 8) | b;
            }
        }

        // Draw food
        uint16_t foodIdx = foodY * GRID_WIDTH + foodX;
        gridOut[foodIdx] = 0x00E00000;  // red
    }

    outScore = snakeScore;
    outLength = snakeLen;
    over = gameOver;
}
