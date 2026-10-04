#ifndef ECG_SENSOR_H
#define ECG_SENSOR_H

#include <Arduino.h>
#include "esp_timer.h"

class ECGSensor {

private:
    int pin;

    // 5 ms = 200 samples/second
    static const uint64_t SAMPLE_INTERVAL_US = 5000;
    static const uint32_t RING_SIZE = 256;

    // ISR/timer fills this so loop() blocking cannot drop the 200 Hz stream
    volatile int ring[RING_SIZE];
    volatile uint32_t head = 0;
    volatile uint32_t tail = 0;

    esp_timer_handle_t timer = nullptr;

    static void onTimer(void *arg) {
        static_cast<ECGSensor *>(arg)->sampleNow();
    }

    void sampleNow() {
        uint32_t next = (head + 1) % RING_SIZE;
        if (next == tail) {
            return;
        }

        ring[head] = analogRead(pin);
        head = next;
    }


public:

    ECGSensor(int ecgPin) {
        pin = ecgPin;
    }


    // =========================
    // BEGIN
    // =========================

    void begin() {

        pinMode(pin, INPUT);

        // ESP32 ADC = 12-bit
        // Values range from 0 - 4095
        analogReadResolution(12);

        esp_timer_create_args_t args = {};
        args.callback = &ECGSensor::onTimer;
        args.arg = this;
        args.name = "ecg_200hz";

        if (esp_timer_create(&args, &timer) != ESP_OK) {
            timer = nullptr;
            return;
        }

        esp_timer_start_periodic(timer, SAMPLE_INTERVAL_US);
    }


    // =========================
    // UPDATE
    // =========================

    // Sampling is timer-driven. Kept so the main loop API stays the same.
    void update() {
    }


    // =========================
    // GET ECG VALUE
    // =========================

    int getValue() {
        return ring[tail];
    }


    // =========================
    // NEW SAMPLE AVAILABLE?
    // =========================

    bool available() {
        return head != tail;
    }


    // =========================
    // MARK SAMPLE AS READ
    // =========================

    void readComplete() {
        if (head == tail) {
            return;
        }
        tail = (tail + 1) % RING_SIZE;
    }


    // Drop unread samples, e.g. after setup delays
    void clear() {
        tail = head;
    }
};

#endif
