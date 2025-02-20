#include <Arduino.h>
#include "led.h"

static uint8_t ledPin = LED_BUILTIN;   // yellow LED cathode connected to digital pin
static uint8_t ledOn = LOW;            // the LED anode is connected to 3.3V via 1.5K resistor

unsigned long nextChangeTs = 0;
bool isLedOn = false;
int remainingBlicks = 0;
int onTime = 0;
int offTime = 0;

void setupLed() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, 1-ledOn);
}

void loopLed() {
    if (remainingBlicks > 0 || isLedOn) {
        if (millis() >= nextChangeTs) {
            if (isLedOn) {
                digitalWrite(ledPin, 1-ledOn);
                isLedOn = false;
                nextChangeTs = millis() + offTime;
            } else {
                digitalWrite(ledPin, ledOn);
                isLedOn = true;
                nextChangeTs = millis() + onTime;
                remainingBlicks--;
            }
        }
    }
}

void blickLed(int delayMs) {
    blickLed(1, delayMs, 0);
}

void blickLed(int count, int onMs, int offMs) {
    remainingBlicks = count - 1;
    nextChangeTs = millis() + onMs;
    isLedOn = true;
    onTime = onMs;
    offTime = offMs;
    digitalWrite(ledPin, ledOn);
}