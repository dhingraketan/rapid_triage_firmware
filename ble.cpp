#include "ble.h"
#include <Arduino.h>
#include <stdio.h>

static char bleName[20] = "";

const char *get_ble_name() {
  return bleName;
}

void adv_ble() {
  // Each ESP32 has a unique factory MAC. Use the low 4 bytes so
  // nearby triage tags advertise different names, e.g. patient_A3F21C0B
  uint64_t mac = ESP.getEfuseMac();
  snprintf(
      bleName,
      sizeof(bleName),
      "patient_%04X%04X",
      (uint16_t)(mac >> 16),
      (uint16_t)mac
  );

  BLEDevice::init(bleName);

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->start();
}
