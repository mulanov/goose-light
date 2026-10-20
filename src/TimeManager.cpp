#include "TimeManager.h"
#include "Config.h"
#include "WifiManager.h"
#include <ESP8266WiFi.h>
#include <time.h>
#include <stdlib.h>

namespace {
bool configured = false;
bool synced = false;
uint32_t lastCheck = 0;
}

namespace TimeManager {
void begin() {
    configured = false;
    synced = false;
}
void loop() {
    if (!WifiManager::connected() || !Config::isReady()) return;

    if (!configured) {
        const auto& c = Config::get();
        setenv("TZ", c.timezone, 1);
        tzset();
        configTime(0, 0, c.ntpServers[0], c.ntpServers[1]);
        configured = true;
        Serial.println(F("[Time] NTP configured"));
    }

    if (millis() - lastCheck >= 1000) {
        lastCheck = millis();
        const time_t t = ::time(nullptr);
        // Reject uninitialized clocks (roughly before 2022).
        synced = t > 1640995200;
    }
}
bool isSynced() { return synced; }
time_t now() { return synced ? ::time(nullptr) : 0; }
bool getLocalTime(tm& result) {
    if (!synced) return false;
    const time_t t = ::time(nullptr);
    localtime_r(&t, &result);
    return true;
}
}
