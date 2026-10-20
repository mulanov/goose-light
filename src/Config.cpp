#include "Config.h"

#include <LittleFS.h>
#include <SaverFile.h>
#include <cstring>

namespace {
constexpr char CONFIG_PATH[] = "/config.bin";
constexpr uint8_t CONFIG_FORMAT_VERSION = 'A';
constexpr uint8_t SAVE_TIMEOUT_SECONDS = 5;
constexpr uint16_t CONFIG_SCHEMA_VERSION = 1;

AppConfig configData{};

// SaverFile expects fs::FS&, path, data, version, timeout seconds, mode.
SaverFile configSaver(
    LittleFS, CONFIG_PATH, configData,
    CONFIG_FORMAT_VERSION, SAVE_TIMEOUT_SECONDS,
    FileStore::Atomic
);

bool ready = false;

void copyString(char* dst, size_t size, const char* src) {
    if (!size) return;
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

void setDefaults(AppConfig& c) {
    memset(&c, 0, sizeof(c));
    c.relayCount = 8;  // Start with 8 active relays; admin can change 1..24.

    for (uint8_t i = 0; i < MAX_RELAY; ++i) {
        char name[RELAY_NAME_SIZE];
        snprintf(name, sizeof(name), "Relay%02u", static_cast<unsigned>(i + 1));
        copyString(c.relays[i].name, sizeof(c.relays[i].name), name);
        c.relays[i].startupOn = false;
    }

    copyString(c.stationName, sizeof(c.stationName), "GooseLights");

    copyString(c.ntpServers[0], NTP_SERVER_SIZE, "pool.ntp.org");
    copyString(c.ntpServers[1], NTP_SERVER_SIZE, "time.google.com");
    copyString(c.timezone, sizeof(c.timezone),
               "EET-2EEST,M3.5.0/3,M10.5.0/4");

    c.mqtt.enabled = false;
    c.mqtt.port = 1883;
    copyString(c.mqtt.baseTopic, sizeof(c.mqtt.baseTopic), "house/lights");

    // CHANGE THIS before exposing the device to a network.
    copyString(c.adminPassword, sizeof(c.adminPassword), "change-me-now");
    c.schemaVersion = CONFIG_SCHEMA_VERSION;
}

bool validActionList(const RuleAction* actions, uint8_t count) {
    if (count > MAX_ACTIONS) return false;
    for (uint8_t i = 0; i < count; ++i) {
        const RuleAction& a = actions[i];
        if (a.type != ActionType::RelayOn &&
            a.type != ActionType::RelayOff &&
            a.type != ActionType::RelayToggle &&
            a.type != ActionType::Delay) return false;
        if (a.type != ActionType::Delay && a.relayIndex >= MAX_RELAY) return false;
    }
    return true;
}

bool validateConfig(AppConfig& c) {
    if (c.schemaVersion != CONFIG_SCHEMA_VERSION) return false;
    if (c.relayCount < 1 || c.relayCount > MAX_RELAY) return false;

    for (uint8_t i = 0; i < MAX_RELAY; ++i) {
        c.relays[i].name[RELAY_NAME_SIZE - 1] = '\0';
        c.relays[i].comment[RELAY_COMMENT_SIZE - 1] = '\0';
    }

    for (uint8_t i = 0; i < MAX_RADIO_COMMANDS; ++i) {
        auto& cmd = c.radioCommands[i];
        cmd.code[RADIO_CODE_SIZE - 1] = '\0';
        cmd.name[RADIO_NAME_SIZE - 1] = '\0';
        cmd.comment[RADIO_COMMENT_SIZE - 1] = '\0';
        if (!validActionList(cmd.actions, cmd.actionCount)) return false;
    }

    for (uint8_t i = 0; i < MAX_SCHEDULES; ++i) {
        auto& s = c.schedules[i];
        s.name[RADIO_NAME_SIZE - 1] = '\0';
        if (s.hour > 23 || s.minute > 59 ||
            (s.weekdaysMask & 0x80) != 0 ||
            !validActionList(s.actions, s.actionCount)) return false;
    }

    for (uint8_t i = 0; i < WIFI_PROFILE_COUNT; ++i) {
        c.wifi[i].ssid[WIFI_SSID_SIZE - 1] = '\0';
        c.wifi[i].password[WIFI_PASSWORD_SIZE - 1] = '\0';
    }
    for (uint8_t i = 0; i < NTP_SERVER_COUNT; ++i) {
        c.ntpServers[i][NTP_SERVER_SIZE - 1] = '\0';
    }

    c.stationName[STATION_NAME_SIZE - 1] = '\0';
    c.timezone[TIMEZONE_SIZE - 1] = '\0';
    c.adminPassword[ADMIN_PASSWORD_SIZE - 1] = '\0';
    c.mqtt.host[MQTT_HOST_SIZE - 1] = '\0';
    c.mqtt.username[MQTT_USER_SIZE - 1] = '\0';
    c.mqtt.password[MQTT_PASSWORD_SIZE - 1] = '\0';
    c.mqtt.baseTopic[MQTT_BASE_TOPIC_SIZE - 1] = '\0';

    if (c.mqtt.port == 0) return false;
    return true;
}
}  // namespace

namespace Config {

bool begin() {
    ready = false;
    setDefaults(configData);

    if (!LittleFS.begin()) {
        Serial.println(F("[Config] LittleFS mount failed; filesystem not formatted automatically"));
        return false;
    }

    const Saver::Status status = configSaver.beginGrow();
    if (status == Saver::Error) {
        Serial.println(F("[Config] Saver could not read/write config"));
        return false;
    }

    if (!validateConfig(configData)) {
        Serial.println(F("[Config] Invalid/unsupported config; using defaults"));
        setDefaults(configData);
        if (configSaver.write(true) == Saver::Error) {
            Serial.println(F("[Config] Failed to write defaults"));
            return false;
        }
    }

    ready = true;
    Serial.printf("[Config] Ready; config struct size=%u bytes\n",
                  static_cast<unsigned>(sizeof(AppConfig)));
    return true;
}

void loop() {
    if (!ready) return;
    const Saver::Status status = configSaver.tick();
    if (status == Saver::Write) Serial.println(F("[Config] Saved"));
    else if (status == Saver::Error) Serial.println(F("[Config] Save error"));
}

AppConfig& get() {
    return configData;
}

bool saveNow() {
    if (!ready) return false;
    return configSaver.write(true) != Saver::Error;
}

bool resetDefaults() {
    if (!ready) return false;
    setDefaults(configData);
    return configSaver.write(true) != Saver::Error;
}

bool isReady() {
    return ready;
}

}  // namespace Config
