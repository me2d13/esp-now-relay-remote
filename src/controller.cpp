#include <Arduino.h>
#include <ArduinoJson.h>
#include "controller.h"
#include "state.h"
#include "led.h"
#include "config.h"
#include "crypto.h"
#include <WiFi.h>
#include <esp_now.h>
#include "battery.h"
#include "tools.h"

#define LONG_PRESS_MS 3000
#define VERY_LONG_PRESS_MS 10000
#define ENABLE_ENCRYPTION true

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // All stations broadcast address
byte espNowMessageBuffer[251];

void registerPeersFromState();

String buildName(uint8_t index)
{
    String nameStr = MY_NAME;
    nameStr += "-";
    nameStr += (index + 1);
    return nameStr;
}

void sendDocument(JsonDocument &doc, uint8_t *mac)
{
    String dataStr;
    serializeJson(doc, dataStr);
    uint8_t dataBytes[250];
    int length = messageToByteArray(dataStr.c_str(), dataBytes, ENABLE_ENCRYPTION);
    if (length > 0)
    {
        Serial.print("Sending ");
        logMessageToSerial(dataBytes, length, ENABLE_ENCRYPTION);
        esp_now_send(mac, dataBytes, length);
    }
}

void sendPress(uint8_t index)
{
    float voltage = readBatteryVoltage();
    // send request
    auto stateItem = state.getStateItem(index);
    JsonDocument doc;
    doc["from"] = buildName(index);
    doc["counter"] = stateItem->counter;
    doc["channel"] = stateItem->channel;
    doc["push"] = PUSH_TIME_MS;
    doc["battery"] = voltage;
    sendDocument(doc, stateItem->mac);
    // increase counter and save state
    stateItem->counter++;
    state.saveState();
}

void pairDevice(uint8_t index)
{
    // pair device
    JsonDocument doc;
    StateItem *stateItem = state.getStateItem(index);
    // construct name as MY_NAME + "-" + index
    stateItem->pairPhase = PAIR_REQUEST_SENT;
    stateItem->pairId = random(0, 1000000);
    stateItem->counter = random(10, 10000);
    doc["pairMagic"] = PAIRING_MAGIC;
    doc["counter"] = stateItem->counter;
    doc["from"] = buildName(index);
    doc["id"] = stateItem->pairId;
    sendDocument(doc, broadcastAddress);
    state.saveState();
}

void clearPairedDevice(uint8_t index) {
    StateItem *stateItem = state.getStateItem(index);
    if (stateItem != nullptr)
    {
        stateItem->pairPhase = PAIR_NOT_PAIRED;
        stateItem->pairId = 0;
        stateItem->channel = 0;
        stateItem->peerRegistered = false;
        state.saveState();
    }
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
            sendPress(index);
        }
    }
    else if (durationMs < VERY_LONG_PRESS_MS)
    {
        // pairing attempt
        blickLed(3, 1000, 300);
        pairDevice(index);
    } else {
        // clear paired device
        blickLed(1, 5000, 0);
        clearPairedDevice(index);
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

void onDataReceived(const esp_now_recv_info_t *espNowInfo, const uint8_t *incomingData, int len)
{
    memcpy(espNowMessageBuffer, incomingData, len);
    Serial.print(len);
    Serial.print(" bytes received from mac ");
    Serial.print(mac2string(espNowInfo->src_addr));
    Serial.print(": ");
    logMessageToSerial((byte *)incomingData, len, ENABLE_ENCRYPTION);
    if (ENABLE_ENCRYPTION)
    {
        inPlaceDecrypt(espNowMessageBuffer, len);
    }
    Serial.print("Decrypted ");
    logMessageToSerial(espNowMessageBuffer, len, false);
    // parse decrypted message to json
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, espNowMessageBuffer);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }
    // find pairing entry and update mac
    for (int i = 0; i < NO_OF_BUTTONS; i++)
    {
        StateItem *stateItem = state.getStateItem(i);
        if (stateItem != nullptr && stateItem->pairPhase == PAIR_REQUEST_SENT)
        {
            if (doc["pairMagic"] == PAIRING_MAGIC && doc["id"] == stateItem->pairId)
            {
                stateItem->pairPhase = PAIRED;
                stateItem->channel = doc["channel"];
                Serial.print("Found pairing request for index ");
                Serial.print(i);
                Serial.print(" with channel ");
                Serial.println(stateItem->channel);
                memcpy(stateItem->mac, espNowInfo->src_addr, 6);
                registerPeersFromState();
                state.saveState();
                blickLed(3, 100, 100);
                return;
            }
        }
    }
}

bool registerPeer(uint8_t *mac)
{
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    memcpy(peerInfo.peer_addr, mac, 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return false;
    }
    return true;
}

void registerPeersFromState()
{
    // register peers from state
    for (int i = 0; i < NO_OF_BUTTONS; i++)
    {
        StateItem *stateItem = state.getStateItem(i);
        if (stateItem != nullptr && stateItem->pairPhase == PAIRED && !stateItem->peerRegistered)
        {
            Serial.print("Registering peer ");
            Serial.print(i);
            Serial.print(" from state with mac: ");
            Serial.println(mac2string(stateItem->mac));
            auto result = registerPeer(stateItem->mac);
            stateItem->peerRegistered = result;
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
    registerPeer(broadcastAddress);
    registerPeersFromState();
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceived);
    Serial.println("ESP-NOW init done");
}