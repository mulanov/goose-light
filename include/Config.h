#pragma once

#include <Arduino.h>

// Physical limit: three 74HC595 registers = 24 outputs.
constexpr uint8_t MAX_RELAY = 24;
constexpr uint8_t MAX_RADIO_COMMANDS = 64;
constexpr uint8_t MAX_ACTIONS = 16;
constexpr uint8_t MAX_SCHEDULES = 32;
constexpr uint8_t WIFI_PROFILE_COUNT = 2;
constexpr uint8_t NTP_SERVER_COUNT = 2;

constexpr size_t RELAY_NAME_SIZE = 9;       // 8 characters + NUL
constexpr size_t RELAY_COMMENT_SIZE = 96;
constexpr size_t RADIO_NAME_SIZE = 25;
constexpr size_t RADIO_COMMENT_SIZE = 128;
constexpr size_t RADIO_CODE_SIZE = 32;
constexpr size_t WIFI_SSID_SIZE = 33;
constexpr size_t WIFI_PASSWORD_SIZE = 64;
constexpr size_t NTP_SERVER_SIZE = 64;
constexpr size_t MQTT_HOST_SIZE = 64;
constexpr size_t MQTT_USER_SIZE = 32;
constexpr size_t MQTT_PASSWORD_SIZE = 64;
constexpr size_t MQTT_BASE_TOPIC_SIZE = 64;
constexpr size_t ADMIN_PASSWORD_SIZE = 64;
constexpr size_t TIMEZONE_SIZE = 48;

enum class ActionType : uint8_t {
    RelayOn = 0,
    RelayOff = 1,
    RelayToggle = 2,
    Delay = 3
};

struct RelayConfig {
    char name[RELAY_NAME_SIZE];
    char comment[RELAY_COMMENT_SIZE];
    bool startupOn;
};

struct RuleAction {
    ActionType type;
    uint8_t relayIndex;       // 0..23; ignored for Delay
    uint16_t delaySeconds;    // pause duration
};

struct RadioCommandConfig {
    bool enabled;
    char code[RADIO_CODE_SIZE];
    char name[RADIO_NAME_SIZE];
    char comment[RADIO_COMMENT_SIZE];
    uint8_t actionCount;
    RuleAction actions[MAX_ACTIONS];
};

struct WifiProfile {
    char ssid[WIFI_SSID_SIZE];
    char password[WIFI_PASSWORD_SIZE];
};

struct MqttConfig {
    bool enabled;
    char host[MQTT_HOST_SIZE];
    uint16_t port;
    char username[MQTT_USER_SIZE];
    char password[MQTT_PASSWORD_SIZE];
    char baseTopic[MQTT_BASE_TOPIC_SIZE];
};

struct ScheduleConfig {
    bool enabled;
    char name[RADIO_NAME_SIZE];
    uint8_t weekdaysMask;     // bit 0 = Monday ... bit 6 = Sunday
    uint8_t hour;             // 0..23 local time
    uint8_t minute;           // 0..59
    uint8_t actionCount;
    RuleAction actions[MAX_ACTIONS];
};

struct AppConfig {
    uint8_t relayCount;       // active relays, 1..MAX_RELAY
    RelayConfig relays[MAX_RELAY];
    RadioCommandConfig radioCommands[MAX_RADIO_COMMANDS];
    ScheduleConfig schedules[MAX_SCHEDULES];
    WifiProfile wifi[WIFI_PROFILE_COUNT];
    char ntpServers[NTP_SERVER_COUNT][NTP_SERVER_SIZE];
    char timezone[TIMEZONE_SIZE];  // POSIX TZ string
    MqttConfig mqtt;
    char adminPassword[ADMIN_PASSWORD_SIZE];
    uint16_t schemaVersion;
};

namespace Config {
    bool begin();
    void loop();
    AppConfig& get();
    bool saveNow();
    bool resetDefaults();
    bool isReady();
}
