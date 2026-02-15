#include "websocket_handler.h"
#include "tetris_effect.h"
#include "snake_game.h"
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

static WebSocketsServer ws(81);
static bool clientConnected = false;
static bool clientAuthenticated = false;
static uint8_t activeClient = 0;
static unsigned long lastBroadcastMs = 0;
#define WS_BROADCAST_INTERVAL_MS 100  // 10 FPS

// Which game is currently active (for command routing + broadcast)
static Effect wsActiveEffect = EFFECT_TETRIS;

void setWsActiveEffect(Effect effect) {
    wsActiveEffect = effect;
}

// ─── Auth ──────────────────────────────────────────────────────────────────

static char wsAuthToken[20] = {0};

void setWebSocketAuthToken(const char *token) {
    strncpy(wsAuthToken, token, sizeof(wsAuthToken) - 1);
    wsAuthToken[sizeof(wsAuthToken) - 1] = '\0';
}

// ─── Command Handler ───────────────────────────────────────────────────────

static void handleCommand(uint8_t num, const String &text) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, text);
    if (err) return;

    const char *cmd = doc["cmd"];
    if (!cmd) return;

    // First message must be auth
    if (!clientAuthenticated) {
        if (strcmp(cmd, "auth") == 0) {
            const char *token = doc["token"];
            if (token && strcmp(token, wsAuthToken) == 0) {
                clientAuthenticated = true;
                ws.sendTXT(num, "{\"auth\":true}");
                Serial.printf("WS: Client %u authenticated\n", num);
            } else {
                ws.sendTXT(num, "{\"auth\":false}");
                ws.disconnect(num);
                Serial.printf("WS: Client %u auth failed\n", num);
            }
        }
        return;
    }

    // ── Snake commands ──
    if (wsActiveEffect == EFFECT_SNAKE) {
        if (strcmp(cmd, "left") == 0) {
            snakeSetDirection(3);  // DIR_LEFT
        } else if (strcmp(cmd, "right") == 0) {
            snakeSetDirection(1);  // DIR_RIGHT
        } else if (strcmp(cmd, "rotate") == 0) {
            snakeSetDirection(0);  // DIR_UP (rotate button = up on d-pad)
        } else if (strcmp(cmd, "drop") == 0) {
            snakeSetDirection(2);  // DIR_DOWN (drop button = down on d-pad)
        } else if (strcmp(cmd, "up") == 0) {
            snakeSetDirection(0);
        } else if (strcmp(cmd, "down") == 0) {
            snakeSetDirection(2);
        } else if (strcmp(cmd, "manual") == 0) {
            setSnakeManualMode(true);
        } else if (strcmp(cmd, "ai") == 0) {
            setSnakeManualMode(false);
        }
        return;
    }

    // ── Tetris commands (default) ──
    if (strcmp(cmd, "left") == 0) {
        manualMoveLeft();
    } else if (strcmp(cmd, "right") == 0) {
        manualMoveRight();
    } else if (strcmp(cmd, "rotate") == 0) {
        manualRotate();
    } else if (strcmp(cmd, "drop") == 0) {
        manualHardDrop();
    } else if (strcmp(cmd, "softdrop") == 0) {
        bool active = doc["active"] | false;
        manualSoftDrop(active);
    } else if (strcmp(cmd, "manual") == 0) {
        setManualMode(true);
    } else if (strcmp(cmd, "ai") == 0) {
        setManualMode(false);
    }
}

// ─── WebSocket Events ──────────────────────────────────────────────────────

static void onWsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial.printf("WS: Client %u connected (awaiting auth)\n", num);
            clientConnected = true;
            clientAuthenticated = false;
            activeClient = num;
            break;

        case WStype_DISCONNECTED:
            Serial.printf("WS: Client %u disconnected\n", num);
            if (num == activeClient) {
                clientConnected = false;
                clientAuthenticated = false;
                // Return to AI mode for whichever game is active
                if (wsActiveEffect == EFFECT_SNAKE) {
                    if (isSnakeManualMode()) setSnakeManualMode(false);
                } else {
                    if (isManualMode()) setManualMode(false);
                }
            }
            break;

        case WStype_TEXT:
            handleCommand(num, String((char *)payload));
            break;

        default:
            break;
    }
}

// ─── Broadcast State ───────────────────────────────────────────────────────

// Pre-allocated buffer for broadcast JSON.
// Worst case per cell: "16777215," = 9 chars. 256 cells * 9 = 2304.
// Header/footer ~80 chars. Total ~2400. Use 2600 for safety.
static char broadcastBuf[2600];

static void broadcastState() {
    static uint32_t gridBuf[GRID_WIDTH * GRID_HEIGHT];
    uint16_t score, lines;
    bool over;

    if (wsActiveEffect == EFFECT_SNAKE) {
        // Snake broadcast
        uint16_t snakeLength;
        getSnakeState(gridBuf, score, snakeLength, over);
        lines = snakeLength;
    } else {
        // Tetris broadcast
        uint8_t pType;
        int8_t px, py;
        uint8_t rot;
        bool clr;
        getTetrisState(gridBuf, pType, px, py, rot, score, lines, over, clr);
    }

    // Build JSON into fixed buffer to avoid heap fragmentation
    int pos = snprintf(broadcastBuf, sizeof(broadcastBuf),
        "{\"score\":%u,\"lines\":%u,\"gameOver\":%s,\"grid\":[",
        score, lines, over ? "true" : "false");

    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT && pos < (int)sizeof(broadcastBuf) - 12; i++) {
        if (i > 0) broadcastBuf[pos++] = ',';
        pos += snprintf(broadcastBuf + pos, sizeof(broadcastBuf) - pos, "%lu", (unsigned long)gridBuf[i]);
    }

    if (pos < (int)sizeof(broadcastBuf) - 3) {
        broadcastBuf[pos++] = ']';
        broadcastBuf[pos++] = '}';
        broadcastBuf[pos] = '\0';
    }

    ws.sendTXT(activeClient, broadcastBuf);
}

// ─── Public API ────────────────────────────────────────────────────────────

void setupWebSocket() {
    ws.begin();
    ws.onEvent(onWsEvent);
    Serial.println(F("WebSocket server started on port 81"));
}

void loopWebSocket() {
    ws.loop();

    // Only broadcast for interactive games
    bool isGame = (wsActiveEffect == EFFECT_TETRIS || wsActiveEffect == EFFECT_SNAKE);
    if (clientConnected && clientAuthenticated && isGame &&
        millis() - lastBroadcastMs >= WS_BROADCAST_INTERVAL_MS) {
        lastBroadcastMs = millis();
        broadcastState();
    }
}
