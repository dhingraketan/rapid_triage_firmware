#ifndef WIFI_SEND_H
#define WIFI_SEND_H

#include <Arduino.h>

void wifi_begin();
bool wifi_connected();
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
);

#endif
