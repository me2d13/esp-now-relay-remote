#include <Arduino.h>
#include "battery.h"

void setupBattery() {
    // setup A0 pin as analog input
    pinMode(A0, INPUT);
}

// function to read battery voltage
float readBatteryVoltage() {
    // read the raw value from A0 pin
    int rawValue = analogRead(A0);
    // convert the raw value to voltage
    float voltage = rawValue * (3.3 / 1023.0 /2.0); // we have a voltage divider with 2 resistors
    return voltage;
}