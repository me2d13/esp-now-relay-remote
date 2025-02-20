#include <Arduino.h>
#include <ArduinoJson.h>
#include "controller.h"
#include "state.h"
#include "led.h"
#include "config.h"
#include "crypto.h"
#include <WiFi.h>
#include <esp_now.h>

#define LONG_PRESS_MS 3000
#define ENABLE_ENCRYPTION true

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // All stations broadcast address
esp_now_peer_info_t broadcastPeerInfo;
byte espNowMessageBuffer[251];

void sendRequest(uint8_t index)
{
    // send request
}

void pairDevice(uint8_t index)
{
    // pair device
    JsonDocument doc;
    StateItem *stateItem = state.getStateItem(index);
    stateItem->pairPhase = PAIR_REQUEST_SENT;
    stateItem->pairId = random(0, 1000000);
    doc["pairMagic"] = PAIRING_MAGIC;
    doc["counter"] = stateItem->counter;
    doc["from"] = MY_NAME;
    doc["id"] = stateItem->pairId;
    
    String dataStr;
    serializeJson(doc, dataStr);
    uint8_t dataBytes[250];
  
    int length = messageToByteArray(dataStr.c_str(), dataBytes, ENABLE_ENCRYPTION);
    if (length > 0) {
      Serial.print("Sending ");
      logMessageToSerial(dataBytes, length, ENABLE_ENCRYPTION);
      esp_now_send(broadcastAddress, dataBytes, length);
    }
    state.saveState();
}

void onButtonPress(int index, int durationMs)
{
    auto stateItem = state.getStateItem(index);
    bool paired = stateItem != nullptr && stateItem->pairPhase == PAIRED;
    if (durationMs < LONG_PRESS_MS)
    {
        if (!paired)
        {
            // attempt to send request when not paired
            blickLed(5, 50, 200);
        }
        else
        {
            // send request
            blickLed(1, 500, 0);
            sendRequest(index);
        }
    }
    else
    {
        // pairing attempt
        blickLed(3, 1000, 300);
        pairDevice(index);
    }
}

// callback when data is sent
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    char macStr[18];
    Serial.print("Packet to: ");
    // Copies the sender mac address to a string
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print(macStr);
    Serial.print(" send status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void onDataReceived(const uint8_t* mac, unsigned char *incomingData, uint8_t len) {
    memcpy(espNowMessageBuffer, incomingData, len);
    Serial.print("Bytes received ");
    Serial.print(len);
    Serial.print(": ");
    logMessageToSerial(incomingData, len, ENABLE_ENCRYPTION);
    if (ENABLE_ENCRYPTION) {
        inPlaceDecrypt(espNowMessageBuffer, len);
    }
    // parse decrypted message to json
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, espNowMessageBuffer);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }
    // find pairing entry and update mac
    for (int i = 0; i < NO_OF_BUTTONS; i++) {
        StateItem *stateItem = state.getStateItem(i);
        if (stateItem != nullptr && stateItem->pairPhase == PAIR_REQUEST_SENT) {
            if (doc["pairMagic"] == PAIRING_MAGIC && doc["id"] == stateItem->pairId) {
                stateItem->pairPhase = PAIRED;
                memcpy(stateItem->mac, mac, 6);
                state.saveState();
                blickLed(3, 100, 100);
                return;
            }
        }
    }
  }
  
void setupEspNow()
{
    // setup esp now
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != 0)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    // register peer
    broadcastPeerInfo.channel = 0;
    broadcastPeerInfo.encrypt = false;
    // register first peer
    memcpy(broadcastPeerInfo.peer_addr, broadcastAddress, 6);
    if (esp_now_add_peer(&broadcastPeerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return;
    }
    esp_now_register_send_cb(onDataSent);
    Serial.println("ESP-NOW init done");
}