#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "state.h"
#include "tools.h"

#define PERS_STATE_FILE "/state.json"

PersState state;

void PersState::loadState()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("Failed to mount file system");
        return;
    }
    Serial.println("File system mounted");
    if (!LittleFS.exists(PERS_STATE_FILE))
    {
        Serial.println("State file does not exist, not loaded");
        return;
    }
    Serial.println("State file exists");

    File file = LittleFS.open(PERS_STATE_FILE, "r");
    if (!file)
    {
        Serial.println("Failed to open state file for reading");
        return;
    }
    JsonDocument stateDocument;
    DeserializationError error = deserializeJson(stateDocument, file);
    if (error)
    {
        Serial.println("Failed to read state file");
        file.close();
        return;
    }
    // read state items
    for (int i = 0; i < NO_OF_BUTTONS; i++)
    {
        StateItem *stateItem = &stateItems[i];
        stateItem->counter = stateDocument["stateItems"][i]["counter"];
        stateItem->pairPhase = stateDocument["stateItems"][i]["pairPhase"];
        stateItem->pairId = stateDocument["stateItems"][i]["pairId"];
        stateItem->channel = stateDocument["stateItems"][i]["channel"];
        const char *macStr = stateDocument["stateItems"][i]["mac"];
        sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &stateItem->mac[0], &stateItem->mac[1], &stateItem->mac[2], &stateItem->mac[3], &stateItem->mac[4], &stateItem->mac[5]);
    }
    file.close();
}

void PersState::saveState()
{
    File file = LittleFS.open(PERS_STATE_FILE, "w");
    if (!file)
    {
        Serial.println("Failed to open state file for writing");
        return;
    }
    JsonDocument stateDocument;
    // write state items
    JsonArray items = stateDocument["stateItems"].to<JsonArray>();
    for (int i = 0; i < NO_OF_BUTTONS; i++)
    {
        StateItem *stateItem = &stateItems[i];
        JsonObject stateItemJson = items.add<JsonObject>();
        stateItemJson["counter"] = stateItem->counter;
        stateItemJson["pairPhase"] = stateItem->pairPhase;
        stateItemJson["pairId"] = stateItem->pairId;
        stateItemJson["channel"] = stateItem->channel;
        stateItemJson["mac"] = mac2string(stateItem->mac).c_str();
    }
    serializeJson(stateDocument, file);
    file.close();
}