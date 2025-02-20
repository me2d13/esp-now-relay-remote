#include <Arduino.h>
#include "buttons.h"
#include "config.h"

#define DEBOUNCE_TIME 50
#define DEBUG 1

// pin numbers
uint8_t buttonPins[NO_OF_BUTTONS] = {D2, D1};

// last button states
uint8_t lastButtonStates[NO_OF_BUTTONS] = {HIGH, HIGH};

// last event times in milis
unsigned long lastEventTimes[NO_OF_BUTTONS] = {0, 0};

uint16_t buttonPressMs[NO_OF_BUTTONS] = {0, 0};

bool initialRead = true;


void setupButtons() {
    // setup pins as input pullup
    for (int i = 0; i < NO_OF_BUTTONS; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);
    }
}

// function to read button state and detect button press and release
// returns true if any button was released
bool loopButton(uint8_t buttonIndex) {
    bool result = false;
    uint8_t buttonState = digitalRead(buttonPins[buttonIndex]);
    if (DEBUG && initialRead) {
        Serial.print("Initial read of button ");
        Serial.print(buttonIndex);
        Serial.print(" state: ");
        Serial.println(buttonState);
    }
    unsigned long now = millis();
    if (buttonState != lastButtonStates[buttonIndex]) {
        // when device was woken up by button press, ignore debounce time
        if (initialRead || (now - lastEventTimes[buttonIndex] > DEBOUNCE_TIME)) {
            if (buttonState == LOW) {
                if (DEBUG) {
                    Serial.print("Button ");
                    Serial.print(buttonIndex);
                    Serial.println(" pressed");
                }
                buttonPressMs[buttonIndex] = 0;
            } else {
                if (DEBUG) {
                    Serial.print("Button ");
                    Serial.print(buttonIndex);
                    Serial.println(" released");
                }
                buttonPressMs[buttonIndex] = now - lastEventTimes[buttonIndex];
                result = true;
            }
            lastButtonStates[buttonIndex] = buttonState;
            lastEventTimes[buttonIndex] = now;
        }
    }
    return result;
}

uint16_t readButtonPressMs(uint8_t buttonIndex) {
    uint16_t result = buttonPressMs[buttonIndex];
    buttonPressMs[buttonIndex] = 0;
    return result;
}

bool loopButtons() {
    bool result = false;
    for (int i = 0; i < NO_OF_BUTTONS; i++) {
        loopButton(i);
        if (buttonPressMs[i] > 0) {
            result = true;
        }
    }
    initialRead = false;
    return result;
}

void reportCurrentButtonState() {
    for (int i = 0; i < NO_OF_BUTTONS; i++) {
        uint8_t buttonState = digitalRead(buttonPins[i]);
        if (buttonState == LOW) {
            Serial.print("Button ");
            Serial.print(i);
            Serial.println(" is pressed");
        } else {
            Serial.print("Button ");
            Serial.print(i);
            Serial.println(" is released");
        }
    }
}

uint8_t* getButtonPins() {
    return buttonPins;
}

unsigned long getLastButtonActivityTime() {
    unsigned long result = 0;
    for (int i = 0; i < NO_OF_BUTTONS; i++) {
        if (lastEventTimes[i] > result) {
            result = lastEventTimes[i];
        }
    }
    return result;
}