#ifndef WEB_SOCKET_H
#define WEB_SOCKET_H

#include <WebSocketsClient.h>

void socket_connect(const char *deviceId);
void socket_loop();
bool socket_poll_stage(int &stageOut);

#endif
