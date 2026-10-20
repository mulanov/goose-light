#include "Rules.h"
#include "Config.h"
#include "Relay.h"
#include "TimeManager.h"

// Scenario execution is intentionally a stub in this first project scaffold.
// Next implementation will use a non-blocking action queue (millis(), no delay()).
namespace Rules {
void begin() {}
void loop() {}

void onRadioCode(const String& code) {
    if (!Config::isReady()) return;
    const auto& cfg = Config::get();
    for (uint8_t i = 0; i < MAX_RADIO_COMMANDS; ++i) {
        const auto& cmd = cfg.radioCommands[i];
        if (cmd.enabled && cmd.code[0] && code.equals(cmd.code)) {
            runRadioCommand(i);
            return;
        }
    }
}

bool runRadioCommand(uint8_t commandIndex) {
    if (!Config::isReady() || commandIndex >= MAX_RADIO_COMMANDS) return false;
    // TODO: queue actions without blocking networking or radio reception.
    Serial.printf("[Rules] Command %u matched; executor not implemented yet\n", commandIndex);
    return true;
}
}
