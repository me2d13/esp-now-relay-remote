#pragma once

#include <Arduino.h>
#include "config.h"

typedef enum {
    PAIR_NOT_PAIRED = 0,
    PAIR_REQUEST_SENT = 1,
    PAIRED = 2
} pair_phase_t;


struct StateItem
{
    uint8_t mac[6];
    pair_phase_t pairPhase;
    uint32_t counter;
    uint32_t pairId;
    uint8_t channel;
    bool peerRegistered; // not persisted
};

class PersState
{
private:
    StateItem stateItems[NO_OF_BUTTONS];
public:
    void saveState();
    void loadState();
    StateItem *getStateItem(int index) {
        return &stateItems[index];
    }
};

extern PersState state;