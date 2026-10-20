#include "WifiManager.h"
#include "Config.h"
#include <ESP8266WiFi.h>

namespace {
enum class State : uint8_t { Trying, AccessPoint, Connected };
State state = State::Trying;
uint8_t profileIndex = 0;
uint32_t attemptStarted = 0;
uint32_t lastRetry = 0;
bool hasProfiles() {
    const auto& c = Config::get();
    return c.wifi[0].ssid[0] || c.wifi[1].ssid[0];
}
void startProfile(uint8_t index) {
    auto& p = Config::get().wifi[index];
    WiFi.mode(WIFI_STA);
    WiFi.hostname(Config::get().stationName);
    WiFi.begin(p.ssid, p.password);
    attemptStarted = millis();
    state = State::Trying;
}
void startSetupAP() {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    // Change this password before deployment.
    WiFi.softAP("GooseLight-Setup", "setup1234");
    state = State::AccessPoint;
    Serial.println(F("[WiFi] Setup AP: GooseLight-Setup"));
    Serial.println(F("[WiFi] AP password: setup1234"));
}
}

namespace WifiManager {
void begin() {
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    if (!Config::isReady() || !hasProfiles()) {
        startSetupAP();
        return;
    }
    profileIndex = Config::get().wifi[0].ssid[0] ? 0 : 1;
    startProfile(profileIndex);
}
void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (state != State::Connected) {
            state = State::Connected;
            Serial.print(F("[WiFi] Connected, IP="));
            Serial.println(WiFi.localIP());
        }
        return;
    }

    if (state == State::Trying && millis() - attemptStarted > 12000) {
        const uint8_t other = profileIndex == 0 ? 1 : 0;
        if (Config::isReady() && Config::get().wifi[other].ssid[0]) {
            profileIndex = other;
            startProfile(profileIndex);
        } else {
            startSetupAP();
        }
    } else if (state == State::Connected && millis() - lastRetry > 15000) {
        lastRetry = millis();
        if (hasProfiles()) {
            profileIndex = profileIndex == 0 ? 1 : 0;
            if (!Config::get().wifi[profileIndex].ssid[0]) profileIndex = profileIndex == 0 ? 1 : 0;
            if (Config::get().wifi[profileIndex].ssid[0]) startProfile(profileIndex);
        }
    } else if (state == State::AccessPoint && hasProfiles() && millis() - lastRetry > 30000) {
        lastRetry = millis();
        profileIndex = Config::get().wifi[0].ssid[0] ? 0 : 1;
        startProfile(profileIndex);
    }
}
bool connected() { return WiFi.status() == WL_CONNECTED; }
bool inSetupMode() { return state == State::AccessPoint; }
}
