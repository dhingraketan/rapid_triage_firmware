#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include <Wire.h>
#include "rgb_lcd.h"

class LCD {
private:
    rgb_lcd lcd;

    // Print exactly 16 characters so old text gets overwritten
    void printLine(const String &text) {
        String output = text;

        // Cut off anything longer than 16 characters
        if (output.length() > 16) {
            output = output.substring(0, 16);
        }

        // Pad shorter strings with spaces
        while (output.length() < 16) {
            output += " ";
        }

        lcd.print(output);
    }

public:

    void begin() {
        lcd.begin(16, 2);
        lcd.clear();

        // Default backlight
        green();
    }

    // Display two lines
    void display(const String &line1, const String &line2) {
        lcd.setCursor(0, 0);
        printLine(line1);

        lcd.setCursor(0, 1);
        printLine(line2);
    }

    // Green backlight
    void green() {
        lcd.setRGB(0, 255, 0);
    }

    // Red backlight
    void red() {
        lcd.setRGB(255, 0, 0);
    }

    // Orange backlight
    void orange() {
        lcd.setRGB(255, 80, 0);
    }
};

#endif