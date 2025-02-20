#include <Arduino.h>
#include "battery.h"
#include "buttons.h"
#include "sleep.h"
#include "led.h"
#include "config.h"
#include "state.h"
#include "controller.h"
#include "crypto.h"

#define SERIAL_BAUDRATE 115200

void setup() {
  setupButtons();
  Serial.begin(SERIAL_BAUDRATE);
  loopButtons(); // read initial button states to handle deep sleep wakeup
  setupBattery();
  Serial.println("Starting setup");
  setupSleep();
  setupLed();
  state.loadState();
  setupCrypto();
  setupEspNow();
}

// PULSE: analogWrite(ledPin, fade); 0-255

int lastBatteryReadTime = 0;

void loop() {
  loopSleep();
  loopLed();
  int now = millis();
  if (now - lastBatteryReadTime > 10000) {
    float voltage = readBatteryVoltage();
    Serial.print("Battery voltage: ");
    Serial.print(voltage);
    Serial.println(" V");
    lastBatteryReadTime = now;
    //reportCurrentButtonState();
  }
  if (loopButtons()) {
    blickLed(3, 50, 200);
    for (int i = 0; i < NO_OF_BUTTONS; i++) {
      uint16_t pressMs = readButtonPressMs(i);
      if (pressMs > 0) {
        Serial.print("Button ");
        Serial.print(i);
        Serial.print(" press duration: ");
        Serial.print(pressMs);
        Serial.println(" ms");
        onButtonPress(i, pressMs);
      }
    }
  }
}

