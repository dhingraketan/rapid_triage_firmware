#ifndef MAX30102_SENSOR_H
#define MAX30102_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

class MAX30102Sensor {

private:
    MAX30105 sensor;

    // Temporary wrist-contact threshold
    static const uint32_t CONTACT_THRESHOLD = 10000;

    // Maxim algorithm uses a 100-sample window
    static const int SPO2_SAMPLE_COUNT = 100;

    uint32_t irBuffer[SPO2_SAMPLE_COUNT];
    uint32_t redBuffer[SPO2_SAMPLE_COUNT];

    int sampleIndex = 0;

    bool contact = false;

    int32_t heartRate = 0;
    int8_t heartRateValidFlag = 0;

    int32_t spo2 = 0;
    int8_t spo2ValidFlag = 0;


    void resetReadings() {
        sampleIndex = 0;

        heartRate = 0;
        heartRateValidFlag = 0;

        spo2 = 0;
        spo2ValidFlag = 0;
    }


    void calculateVitals() {

        maxim_heart_rate_and_oxygen_saturation(
            irBuffer,
            SPO2_SAMPLE_COUNT,
            redBuffer,

            &spo2,
            &spo2ValidFlag,

            &heartRate,
            &heartRateValidFlag
        );
    }


public:

  bool begin() {

    // Give all I2C devices time to power up
    delay(500);

    // Try MAX30102 at its known address
    if (!sensor.begin(
        Wire,
        I2C_SPEED_STANDARD,
        0x57
    )) {
        return false;
    }

    sensor.setup(
        60,     // LED brightness
        4,      // sample averaging
        2,      // Red + IR
        100,    // sample rate
        411,    // pulse width
        4096    // ADC range
    );

    resetReadings();

    return true;
  }


    void update() {

        sensor.check();

        while (sensor.available()) {

            uint32_t red = sensor.getRed();
            uint32_t ir = sensor.getIR();

            sensor.nextSample();


            // =========================
            // CONTACT DETECTION
            // =========================

            if (ir < CONTACT_THRESHOLD) {

                if (contact) {
                    resetReadings();
                }

                contact = false;
                continue;
            }

            contact = true;


            // =========================
            // COLLECT SAMPLES
            // =========================

            irBuffer[sampleIndex] = ir;
            redBuffer[sampleIndex] = red;

            sampleIndex++;


            // =========================
            // CALCULATE HR + SpO2
            // =========================

            if (sampleIndex >= SPO2_SAMPLE_COUNT) {

                calculateVitals();

                sampleIndex = 0;
            }
        }
    }


    // =========================
    // OUTPUTS
    // =========================

    bool hasContact() {
        return contact;
    }


    float getHeartRate() {
        return (float)heartRate;
    }


    float getSpO2() {
        return (float)spo2;
    }


    bool isHeartRateValid() {

        if (heartRateValidFlag != 1) {
            return false;
        }

        // Basic physiological sanity filter
        if (heartRate < 30 || heartRate > 220) {
            return false;
        }

        return true;
    }


    bool isSpO2Valid() {

        if (spo2ValidFlag != 1) {
            return false;
        }

        if (spo2 < 70 || spo2 > 100) {
            return false;
        }

        return true;
    }
};

#endif