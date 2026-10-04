#include <WiFi.h>
#include <string.h>
#include "web_socket.h"

const char *backendHost = "192.168.8.234";
const int backendPort = 5000;

WebSocketsClient webSocket;
String wsDeviceId = "patient";
int pendingStage = -1;

static int parseStagePayload(uint8_t *payload, size_t length) {
    if (payload == nullptr || length == 0) {
        return -1;
    }

    char buf[96];
    if (length >= sizeof(buf)) {
        length = sizeof(buf) - 1;
    }
    memcpy(buf, payload, length);
    buf[length] = '\0';

    const char *p = buf;
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') {
        p++;
    }

    if (p[0] >= '0' && p[0] <= '3' && (p[1] == '\0' || p[1] == '\n' || p[1] == '\r')) {
        return p[0] - '0';
    }

    const char *key = strstr(buf, "stage");
    if (key != nullptr) {
        const char *colon = strchr(key, ':');
        if (colon != nullptr) {
            colon++;
            while (*colon == ' ' || *colon == '\"') {
                colon++;
            }
            if (*colon >= '0' && *colon <= '3') {
                return *colon - '0';
            }
        }
    }

    return -1;
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
    if (type != WStype_TEXT && type != WStype_BIN) {
        return;
    }

    if (payload != nullptr && length > 0) {
        Serial.write(payload, length);
        Serial.println();
    }

    if (type == WStype_TEXT) {
        int stage = parseStagePayload(payload, length);
        if (stage >= 0) {
            pendingStage = stage;
        }
    }
}

void socket_connect(const char *deviceId) {
    if (deviceId != nullptr && deviceId[0] != '\0') {
        wsDeviceId = deviceId;
    }

    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    String path = "/ws/device/" + wsDeviceId;
    webSocket.begin(backendHost, backendPort, path.c_str());
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(2000);
}

void socket_loop() {
    webSocket.loop();
}

bool socket_poll_stage(int &stageOut) {
    if (pendingStage < 0) {
        return false;
    }

    stageOut = pendingStage;
    pendingStage = -1;
    return true;
}
