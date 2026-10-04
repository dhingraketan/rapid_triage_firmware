#ifndef LED_H
#define LED_H

#include <Arduino.h>

class LED {
private:
    int redPin;
    int greenPin;

    enum Mode {
        OFF,
        RED_SOLID,
        RED_FLASH,
        GREEN_SOLID,
        GREEN_FLASH
    };

    Mode mode = OFF;

    bool flashState = false;
    unsigned long lastToggle = 0;

    static const unsigned long FLASH_INTERVAL = 300;

    void allOff() {
        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, LOW);
    }

public:

    LED(int red, int green) {
        redPin = red;
        greenPin = green;
    }

    void begin() {
        pinMode(redPin, OUTPUT);
        pinMode(greenPin, OUTPUT);

        allOff();
    }

    // Solid red
    void glow_red() {
        mode = RED_SOLID;

        digitalWrite(redPin, HIGH);
        digitalWrite(greenPin, LOW);
    }

    // Flash red continuously
    void flash_red() {
        mode = RED_FLASH;

        digitalWrite(greenPin, LOW);

        flashState = true;
        digitalWrite(redPin, HIGH);

        lastToggle = millis();
    }

    // Solid green
    void glow_green() {
        mode = GREEN_SOLID;

        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, HIGH);
    }

    // Flash green continuously
    void flash_green() {
        mode = GREEN_FLASH;

        digitalWrite(redPin, LOW);

        flashState = true;
        digitalWrite(greenPin, HIGH);

        lastToggle = millis();
    }

    // Turn everything off
    void turn_off() {
        mode = OFF;
        flashState = false;

        allOff();
    }

    // Call continuously from loop()
    void update() {
        if (mode != RED_FLASH && mode != GREEN_FLASH) {
            return;
        }

        unsigned long now = millis();

        if (now - lastToggle >= FLASH_INTERVAL) {
            lastToggle = now;
            flashState = !flashState;

            if (mode == RED_FLASH) {
                digitalWrite(redPin, flashState ? HIGH : LOW);
            }
            else if (mode == GREEN_FLASH) {
                digitalWrite(greenPin, flashState ? HIGH : LOW);
            }
        }
    }
};

#endif