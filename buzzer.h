#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
private:
    int pin;

    bool enabled = false;
    bool beepState = false;

    unsigned long lastToggle = 0;

    static const unsigned long BEEP_INTERVAL = 300;

    // Hardware is active-LOW: LOW sounds, HIGH is silent.
    static const int ON_LEVEL = LOW;
    static const int OFF_LEVEL = HIGH;

    void writeOff() {
        digitalWrite(pin, OFF_LEVEL);
    }

    void writeOn() {
        digitalWrite(pin, ON_LEVEL);
    }

public:

    Buzzer(int buzzerPin) {
        pin = buzzerPin;
    }

    void begin() {
        pinMode(pin, OUTPUT);
        writeOff();
    }

    // Start repeating:
    // 0.3 sec ON -> 0.3 sec OFF -> repeat
    void turn_on() {
        if (!enabled) {
            enabled = true;
            beepState = true;
            writeOn();
            lastToggle = millis();
        }
    }

    // Completely stop buzzer
    void turn_off() {
        enabled = false;
        beepState = false;
        writeOff();
    }

    // Must be called continuously from loop()
    void update() {
        if (!enabled) {
            writeOff();
            return;
        }

        unsigned long now = millis();

        if (now - lastToggle >= BEEP_INTERVAL) {
            lastToggle = now;
            beepState = !beepState;
            if (beepState) {
                writeOn();
            } else {
                writeOff();
            }
        }
    }
};

#endif
