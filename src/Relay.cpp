#include "Relay.h"
#include "Config.h"

// ASSUMED Wemos D1 mini wiring; change to match your board.
// D5/GPIO14 = shift clock, D6/GPIO12 = serial data, D7/GPIO13 = latch.
constexpr uint8_t PIN_SR_CLOCK = D5;
constexpr uint8_t PIN_SR_DATA  = D6;
constexpr uint8_t PIN_SR_LATCH = D7;

// Set true if your relay board is active-low.
constexpr bool RELAY_ACTIVE_LOW = false;

namespace {
uint32_t relayBits = 0;

void writeRegisters() {
    digitalWrite(PIN_SR_LATCH, LOW);
    // Send highest register/bit first. Verify bit-to-relay order on the actual board.
    for (int8_t i = MAX_RELAY - 1; i >= 0; --i) {
        const bool logicalOn = (relayBits & (1UL << i)) != 0;
        const bool outputHigh = RELAY_ACTIVE_LOW ? !logicalOn : logicalOn;
        digitalWrite(PIN_SR_CLOCK, LOW);
        digitalWrite(PIN_SR_DATA, outputHigh ? HIGH : LOW);
        digitalWrite(PIN_SR_CLOCK, HIGH);
    }
    digitalWrite(PIN_SR_LATCH, LOW);
    digitalWrite(PIN_SR_LATCH, HIGH);
}
}

namespace Relay {
void begin() {
    pinMode(PIN_SR_CLOCK, OUTPUT);
    pinMode(PIN_SR_DATA, OUTPUT);
    pinMode(PIN_SR_LATCH, OUTPUT);

    digitalWrite(PIN_SR_CLOCK, LOW);
    digitalWrite(PIN_SR_DATA, LOW);
    digitalWrite(PIN_SR_LATCH, LOW);

    relayBits = 0;
    writeRegisters();

    if (!Config::isReady()) return;
    const AppConfig& cfg = Config::get();
    for (uint8_t i = 0; i < cfg.relayCount; ++i) {
        if (cfg.relays[i].startupOn) relayBits |= (1UL << i);
    }
    writeRegisters();
}

void loop() {}

bool set(uint8_t index, bool on) {
    if (!Config::isReady() || index >= Config::get().relayCount || index >= MAX_RELAY) return false;
    if (on) relayBits |= (1UL << index);
    else relayBits &= ~(1UL << index);
    writeRegisters();
    return true;
}

bool toggle(uint8_t index) {
    if (index >= MAX_RELAY || !Config::isReady() || index >= Config::get().relayCount) return false;
    return set(index, !get(index));
}

bool get(uint8_t index) {
    if (index >= MAX_RELAY) return false;
    return (relayBits & (1UL << index)) != 0;
}

void allOff() {
    relayBits = 0;
    writeRegisters();
}
}
