#pragma once
#include <Arduino.h>

namespace Rules {
    void begin();
    void loop();
    void onRadioCode(const String& code);
    bool runRadioCommand(uint8_t commandIndex);
}
