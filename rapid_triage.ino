#include <Wire.h>
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "ECGSensor.h"
#include "MAX30102Sensor.h"
#include "bodytemp.h"
#include "LED.h"
#include "LCD.h"
#include "buzzer.h"
#include "ble.h"
#include "wifi_send.h"
#include "web_socket.h"

// =========================
// PINS
// =========================

#define ECG_PIN        34
#define LED_RED_PIN    27
#define LED_GREEN_PIN  26
#define BUZZER_PIN     25

// =========================
// TIMING / BUFFERS
// =========================

static const unsigned long SEND_INTERVAL_MS = 1000;
static const int ECG_BUFFER_SIZE = 200;

// =========================
// STAGE FLAGS
// 0: in-queue
// 1: help-coming
// 2: being helped
// 3: out of triage
// =========================

static const int STAGE_IN_QUEUE     = 0;
static const int STAGE_HELP_COMING  = 1;
static const int STAGE_BEING_HELPED = 2;
static const int STAGE_OUT_OF_TRIAGE = 3;

// =========================
// MODULES
// =========================

ECGSensor ecg(ECG_PIN);
MAX30102Sensor pulseOx;
LED statusLed(LED_RED_PIN, LED_GREEN_PIN);
LCD display;
Buzzer buzzer(BUZZER_PIN);

// =========================
// STATE
// =========================

int ecgBuffer[ECG_BUFFER_SIZE];
int ecgCount = 0;

bool pulseOxReady = false;
int currentStage = STAGE_IN_QUEUE;
unsigned long lastSendMs = 0;
RTC_DATA_ATTR int bootCount = 0;

void silenceBuzzer() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH);
}

// =========================
// STAGE OUTPUTS
// =========================

void applyStage(int stage) {
    switch (stage) {
        case STAGE_IN_QUEUE:
            statusLed.glow_red();
            buzzer.turn_off();
            silenceBuzzer();
            display.red();
            break;

        case STAGE_HELP_COMING:
            statusLed.flash_red();
            buzzer.turn_on();
            display.red();
            break;

        case STAGE_BEING_HELPED:
            statusLed.turn_off();
            buzzer.turn_off();
            display.green();
            break;

        case STAGE_OUT_OF_TRIAGE:
            statusLed.glow_green();
            buzzer.turn_off();
            silenceBuzzer();
            display.green();
            break;

        default:
            return;
    }

    currentStage = stage;
}

void pollStageFlag() {
    int nextStage = currentStage;
    if (socket_poll_stage(nextStage) && nextStage != currentStage) {
        applyStage(nextStage);
    }
}

// =========================
// VITALS
// =========================

int heartRateToSend() {
    if (!pulseOxReady || !pulseOx.hasContact() || !pulseOx.isHeartRateValid()) {
        return -1;
    }
    return (int)pulseOx.getHeartRate();
}

int spo2ToSend() {
    if (!pulseOxReady || !pulseOx.hasContact() || !pulseOx.isSpO2Valid()) {
        return -1;
    }
    return (int)pulseOx.getSpO2();
}

void updateLcd(int hr, int spo2, float tempC) {
    char line1[17];
    char line2[17];

    snprintf(line1, sizeof(line1), "HR:%3d SpO2:%3d", hr, spo2);

    if (isnan(tempC)) {
        snprintf(line2, sizeof(line2), "Temp: -- C");
    } else {
        snprintf(line2, sizeof(line2), "Temp: %4.1f C", tempC);
    }

    display.display(String(line1), String(line2));
}

// Drain every pending ECG sample into the 1-second RAM buffer.
void drainEcgBuffer() {
    while (ecg.available() && ecgCount < ECG_BUFFER_SIZE) {
        ecgBuffer[ecgCount++] = ecg.getValue();
        ecg.readComplete();
    }
}

void sendTelemetry(int hr, int spo2, bool contact, bool hrValid, bool spo2Valid, float tempC) {
    wifi_send_telemetry(
        get_ble_name(),
        hr,
        spo2,
        contact,
        hrValid,
        spo2Valid,
        tempC,
        ecgBuffer,
        ecgCount
    );
}

void sendOncePerSecond() {
    unsigned long now = millis();
    if (now - lastSendMs < SEND_INTERVAL_MS) {
        return;
    }
    lastSendMs += SEND_INTERVAL_MS;
    drainEcgBuffer();

    int hr = heartRateToSend();
    int spo2 = spo2ToSend();
    bool contact = pulseOxReady && pulseOx.hasContact();
    bool hrValid = pulseOxReady && pulseOx.isHeartRateValid();
    bool spo2Valid = pulseOxReady && pulseOx.isSpO2Valid();
    float tempC = get_body_temp();

    updateLcd(hr, spo2, tempC);
    sendTelemetry(hr, spo2, contact, hrValid, spo2Valid, tempC);
    socket_loop();

    // Start a fresh 200-sample ECG window for the next second.
    ecgCount = 0;
}

// =========================
// SETUP / LOOP
// =========================

void setup() {
    gpio_hold_dis((gpio_num_t)BUZZER_PIN);
    rtc_gpio_hold_dis((gpio_num_t)BUZZER_PIN);
    silenceBuzzer();

    Serial.begin(115200);
    delay(100);
    while (Serial.available()) {
        Serial.read();
    }

    bootCount++;

    Wire.begin();

    display.begin();
    statusLed.begin();
    buzzer.begin();
    applyStage(STAGE_IN_QUEUE);

    ecg.begin();

    pulseOxReady = pulseOx.begin();

    wifi_begin();
    applyStage(STAGE_IN_QUEUE);
    delay(300);

    adv_ble();
    socket_connect(get_ble_name());
    applyStage(STAGE_IN_QUEUE);

    ecg.clear();
    lastSendMs = millis();
}

void loop() {
    drainEcgBuffer();

    if (pulseOxReady) {
        pulseOx.update();
    }

    socket_loop();
    pollStageFlag();

    statusLed.update();
    if (currentStage == STAGE_HELP_COMING) {
        buzzer.update();
    } else {
        digitalWrite(BUZZER_PIN, HIGH);
    }

    sendOncePerSecond();
}
