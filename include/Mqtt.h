#pragma once

#include <stdint.h>

namespace Mqtt {
    void begin();
    void loop();
    void publishRelayState(uint8_t relayIndex, bool on);
}
