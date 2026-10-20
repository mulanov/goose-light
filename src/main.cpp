#include <Arduino.h>

#include "Config.h"
#include "Relay.h"
#include "WifiManager.h"
#include "TimeManager.h"
#include "WebServer.h"
#include "Radio433.h"
#include "Rules.h"
#include "Mqtt.h"

void setup() {
    Serial.begin(115200);
    delay(50);
    Serial.println();
    Serial.println(F("\n[SmartLights] Boot"));

    // Initialize shift-register GPIOs and drive all outputs to OFF first.
    // If storage initialization fails, we leave the hardware in this safe state.
    Relay::begin();

    if (!Config::begin()) {
        Serial.println(F("[SmartLights] FATAL: configuration storage unavailable"));
        Relay::allOff();
        return;
    }

    // Apply per-relay startupOn settings now that configuration is loaded.
    Relay::begin();
    WifiManager::begin();
    TimeManager::begin();
    WebServerModule::begin();
    Radio433::begin();
    Rules::begin();
    Mqtt::begin();
}

void loop() {
    if (!Config::isReady()) {
        delay(10);
        return;
    }

    Config::loop();
    WifiManager::loop();
    TimeManager::loop();
    WebServerModule::loop();
    Radio433::loop();
    Rules::loop();
    Mqtt::loop();
    Relay::loop();
}
