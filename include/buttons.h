#pragma once
#include <Arduino.h>

void setupButtons();
bool loopButtons();
void reportCurrentButtonState();
uint16_t readButtonPressMs(uint8_t buttonIndex);
uint8_t* getButtonPins();
unsigned long getLastButtonActivityTime();