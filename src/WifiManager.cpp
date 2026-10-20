#include "WifiManager.h"
#include "Config.h"
#include <ESP8266WiFi.h>

namespace {
enum class State : uint8_t { Trying, AccessPoint, Connected };
State state = State::Trying;
uint8_t profileIndex = 0;
uint32_t attemptStarted = 0;
uint32_t lastRetry = 0;
bool reconfigurePending = false;
uint32_t reconfigureAt = 0;

bool hasProfiles() {
    const auto& c = Config::get();
    return c.wifi[0].ssid[0] || c.wifi[1].ssid[0];
}

int8_t nextConfiguredProfile(uint8_t current) {
    const auto& c = Config::get();
    const uint8_t other = current == 0 ? 1 : 0;
    if (c.wifi[other].ssid[0]) return static_cast<int8_t>(other);
    if (c.wifi[current].ssid[0]) return static_cast<int8_t>(current);
    if (c.wifi[0].ssid[0]) return 0;
    if (c.wifi[1].ssid[0]) return 1;
    return -1;
}

void startProfile(uint8_t index) {
    auto& p = Config::get().wifi[index];
    if (!p.ssid[0]) return;
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.hostname(Config::get().stationName);
    WiFi.begin(p.ssid, p.password);
    profileIndex = index;
    attemptStarted = millis();
    state = State::Trying;
    Serial.print(F("[WiFi] Trying profile "));
    Serial.print(index + 1);
    Serial.print(F(": "));
    Serial.println(p.ssid);
}

void startSetupAP() {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP("GooseLight-Setup", "setup1234");
    state = State::AccessPoint;
    Serial.println(F("[WiFi] Setup AP: GooseLight-Setup"));
    Serial.print(F("[WiFi] AP IP: "));
    Serial.println(WiFi.softAPIP());
    Serial.println(F("[WiFi] AP password: setup1234"));
}

void applyWifiConfig() {
    reconfigurePending = false;
    if (!Config::isReady() || !hasProfiles()) {
        startSetupAP();
        return;
    }
    const auto& c = Config::get();
    profileIndex = c.wifi[0].ssid[0] ? 0 : 1;
    startProfile(profileIndex);
}
}

namespace WifiManager {
void begin() {
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    if (!Config::isReady() || !hasProfiles()) {
        startSetupAP();
        return;
    }
    const auto& c = Config::get();
    profileIndex = c.wifi[0].ssid[0] ? 0 : 1;
    startProfile(profileIndex);
}

void reconfigure() {
    // Let the HTTP response reach the browser before changing network mode.
    reconfigurePending = true;
    reconfigureAt = millis() + 1500;
}

void loop() {
    if (reconfigurePending && static_cast<int32_t>(millis() - reconfigureAt) >= 0) {
        applyWifiConfig();
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (state != State::Connected) {
            state = State::Connected;
            Serial.print(F("[WiFi] Connected, IP="));
            Serial.println(WiFi.localIP());
        }
        return;
    }

    if (state == State::Trying && millis() - attemptStarted > 12000) {
        // If any SSID is configured, keep trying configured networks instead
        // of silently switching to the setup AP.
        const int8_t next = nextConfiguredProfile(profileIndex);
        if (next >= 0) {
            if (static_cast<uint8_t>(next) != profileIndex) {
                startProfile(static_cast<uint8_t>(next));
            } else {
                startProfile(static_cast<uint8_t>(next));
            }
        } else {
            startSetupAP();
        }
    } else if (state == State::Connected && millis() - lastRetry > 15000) {
        lastRetry = millis();
        const int8_t next = nextConfiguredProfile(profileIndex);
        if (next >= 0) startProfile(static_cast<uint8_t>(next));
    } else if (state == State::AccessPoint && hasProfiles() && millis() - lastRetry > 30000) {
        lastRetry = millis();
        const auto& c = Config::get();
        startProfile(c.wifi[0].ssid[0] ? 0 : 1);
    }
}

bool connected() { return WiFi.status() == WL_CONNECTED; }
bool inSetupMode() { return state == State::AccessPoint; }
}
