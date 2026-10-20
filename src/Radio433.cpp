#include "Radio433.h"
#include "Rules.h"
#include <SoftwareSerial.h>

// ASSUMED wiring: receiver TX -> D1 (GPIO5 / software RX).
// Adjust after confirming the R1A-M5N baud rate and frame format.
constexpr uint8_t RADIO_RX_PIN = D1;
constexpr uint8_t RADIO_TX_PIN = D2;
constexpr uint32_t RADIO_BAUD = 9600;
SoftwareSerial radioSerial(RADIO_RX_PIN, RADIO_TX_PIN);

namespace {
String lineBuffer;
}

namespace Radio433 {
void begin() {
    radioSerial.begin(RADIO_BAUD);
    lineBuffer.reserve(40);
}
void loop() {
    while (radioSerial.available()) {
        const char ch = static_cast<char>(radioSerial.read());
        if (ch == '\r') continue;
        if (ch == '\n') {
            if (lineBuffer.length()) {
                Serial.print(F("[433] "));
                Serial.println(lineBuffer);
                Rules::onRadioCode(lineBuffer);
                lineBuffer = "";
            }
        } else if (lineBuffer.length() < RADIO_CODE_SIZE - 1) {
            lineBuffer += ch;
        } else {
            // Overlong frame: discard to next newline.
            lineBuffer = "";
        }
    }
}
}
