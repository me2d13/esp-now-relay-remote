#include <Arduino.h>
#include "buttons.h"
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "config.h"

#define INACTIVITY_TIMEOUT_TO_SLEEP 30000

gpio_num_t wakeUpPins[NO_OF_BUTTONS] = {GPIO_NUM_1, GPIO_NUM_2};

void setupSleep() {
    // report wakeup reason
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
        default : Serial.printf("Wakeup was not caused by deep sleep: %d",wakeup_reason); break;
    }
}

void loopSleep() {
    unsigned long now = millis();
    if (now - getLastButtonActivityTime() > INACTIVITY_TIMEOUT_TO_SLEEP) {
        Serial.println("Going to sleep");
        // esp_sleep_enable_timer_wakeup(10 * 1000000);
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
        uint64_t mask = 0;
        for (int i = 0; i < NO_OF_BUTTONS; i++) {
            rtc_gpio_pullup_en(wakeUpPins[i]);
            rtc_gpio_pulldown_dis(wakeUpPins[i]);
            mask |= 1 << wakeUpPins[i];
        }
        if (esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW) == ESP_OK) {
            Serial.println("Buttons will wake up the device");
        } else {
            Serial.println("Buttons wake up source setting failed");
        }
        Serial.flush();
        Serial.end();
        esp_deep_sleep_start();
    }
}