#pragma once
#include <Arduino.h>

namespace Relay {
    void begin();
    void loop();
    bool set(uint8_t index, bool on);
    bool toggle(uint8_t index);
    bool get(uint8_t index);
    void allOff();
}
