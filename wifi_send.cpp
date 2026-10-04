#include "wifi_send.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

// Portable router + computer on that LAN.
// Change these if the router SSID or the computer IP changes.
static const char *WIFI_SSID = "GL-SFT1200-4e5";
static const char *WIFI_PASSWORD = "goodlife";
static const char *SERVER_URL = "http://192.168.8.234:5000/sensor-data";

static const unsigned long CONNECT_TIMEOUT_MS = 15000;
static const unsigned long RECONNECT_INTERVAL_MS = 5000;
static const int HTTP_TIMEOUT_MS = 800;
static const size_t JSON_BUF_SIZE = 4096;

static char jsonBuf[JSON_BUF_SIZE];
static unsigned long lastReconnectMs = 0;

static bool appendJson(int &offset, const char *fmt, ...) {
    if (offset < 0 || (size_t)offset >= JSON_BUF_SIZE) {
        return false;
    }

    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(jsonBuf + offset, JSON_BUF_SIZE - offset, fmt, args);
    va_end(args);

    if (written < 0 || (size_t)offset + (size_t)written >= JSON_BUF_SIZE) {
        return false;
    }

    offset += written;
    return true;
}

bool wifi_connected() {
    return WiFi.status() == WL_CONNECTED;
}

static bool wifi_ensure_connected() {
    if (wifi_connected()) {
        return true;
    }

    unsigned long now = millis();
    if (now - lastReconnectMs >= RECONNECT_INTERVAL_MS) {
        lastReconnectMs = now;
        WiFi.reconnect();
    }

    return false;
}

void wifi_begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startMs = millis();
    while (!wifi_connected() && (millis() - startMs) < CONNECT_TIMEOUT_MS) {
        delay(250);
    }

    if (!wifi_connected()) {
        lastReconnectMs = millis();
        return;
    }

    WiFi.setTxPower(WIFI_POWER_11dBm);
}

bool wifi_send_telemetry(
    const char *patientId,
    int heartRate,
    int spo2,
    bool contact,
    bool hrValid,
    bool spo2Valid,
    float tempC,
    const int *ecg,
    int ecgCount
) {
    if (!wifi_ensure_connected()) {
        return false;
    }

    if (patientId == nullptr) {
        patientId = "";
    }
    if (ecg == nullptr) {
        ecgCount = 0;
    }

    int offset = 0;
    bool ok = appendJson(
        offset,
        "{\"patient_id\":\"%s\",\"device_id\":\"%s\",\"heart_rate\":%d,\"spo2\":%d,"
        "\"contact\":%d,\"hr_valid\":%d,\"spo2_valid\":%d,\"temp\":",
        patientId,
        patientId,
        heartRate,
        spo2,
        contact ? 1 : 0,
        hrValid ? 1 : 0,
        spo2Valid ? 1 : 0
    );

    if (ok) {
        if (isnan(tempC)) {
            ok = appendJson(offset, "null");
        } else {
            ok = appendJson(offset, "%.2f", tempC);
        }
    }

    ok = ok && appendJson(offset, ",\"ecg_count\":%d,\"ecg\":[", ecgCount);

    for (int i = 0; ok && i < ecgCount; i++) {
        ok = appendJson(offset, (i == 0) ? "%d" : ",%d", ecg[i]);
    }

    ok = ok && appendJson(offset, "]}");
    if (!ok) {
        return false;
    }

    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT_MS);
    if (!http.begin(SERVER_URL)) {
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    int response = http.POST(reinterpret_cast<uint8_t *>(jsonBuf), offset);
    http.end();

    return response > 0;
}
