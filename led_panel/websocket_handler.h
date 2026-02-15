#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

#include "config.h"

void setupWebSocket();
void loopWebSocket();
bool isWebSocketAuthenticated();
void setWebSocketAuthToken(const char *token);

// Tell the WS handler which game/effect is active (for command routing)
void setWsActiveEffect(Effect effect);

#endif // WEBSOCKET_HANDLER_H
