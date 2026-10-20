#include "WebServer.h"

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include "Config.h"
#include "Relay.h"
#include "WifiManager.h"

namespace {
ESP8266WebServer server(80);

void sendJson(int code, const String& body) {
    server.sendHeader("Cache-Control", "no-store");
    server.send(code, "application/json; charset=utf-8", body);
}

void appendJsonString(String& out, const char* value) {
    out += '"';

    if (value) {
        while (*value) {
            const char c = *value++;

            switch (c) {
                case '"':  out += F("\\\""); break;
                case '\\': out += F("\\\\"); break;
                case '\n': out += F("\\n"); break;
                case '\r': out += F("\\r"); break;
                case '\t': out += F("\\t"); break;
                default:
                    if (static_cast<uint8_t>(c) >= 0x20) {
                        out += c;
                    }
                    break;
            }
        }
    }

    out += '"';
}

bool requireAdmin() {
    if (!Config::isReady()) {
        server.send(503, "text/plain; charset=utf-8",
                    "Configuration unavailable");
        return false;
    }

    const char* password = Config::get().adminPassword;

    if (!password[0] ||
        !server.authenticate("admin", password)) {
        server.requestAuthentication(
            BASIC_AUTH, "goose-light administration");
        return false;
    }

    return true;
}

void sendFile(const char* path, const char* contentType) {
    if (!LittleFS.exists(path)) {
        server.send(404, "text/plain; charset=utf-8",
                    "Web file not found. Upload LittleFS data.");
        return;
    }

    File file = LittleFS.open(path, "r");

    if (!file) {
        server.send(500, "text/plain; charset=utf-8",
                    "Cannot open web file");
        return;
    }

    server.sendHeader("Cache-Control", "no-cache");
    server.streamFile(file, contentType);
    file.close();
}

void handleRoot() {
    sendFile("/index.html", "text/html; charset=utf-8");
}

void handleAdminPage() {
    if (!requireAdmin()) return;
    sendFile("/admin.html", "text/html; charset=utf-8");
}

void handleStatus() {
    if (!Config::isReady()) {
        sendJson(503, "{\"error\":\"config_unavailable\"}");
        return;
    }

    const AppConfig& cfg = Config::get();

    String json;
    json.reserve(1800);

    json += F("{\"relayCount\":");
    json += String(cfg.relayCount);
    json += F(",\"wifiConnected\":");
    json += WifiManager::connected() ? F("true") : F("false");
    json += F(",\"setupMode\":");
    json += WifiManager::inSetupMode() ? F("true") : F("false");
    json += F(",\"relays\":[");

    for (uint8_t i = 0; i < cfg.relayCount; ++i) {
        if (i) json += ',';

        json += F("{\"index\":");
        json += String(i);
        json += F(",\"name\":");
        appendJsonString(json, cfg.relays[i].name);
        json += F(",\"comment\":");
        appendJsonString(json, cfg.relays[i].comment);
        json += F(",\"on\":");
        json += Relay::get(i) ? F("true") : F("false");
        json += '}';
    }

    json += F("]}");
    sendJson(200, json);
}

void handleRelayCommand() {
    if (!Config::isReady()) {
        sendJson(503, "{\"error\":\"config_unavailable\"}");
        return;
    }

    if (!server.hasArg("index") || !server.hasArg("action")) {
        sendJson(400,
                 "{\"error\":\"index_and_action_required\"}");
        return;
    }

    const String indexArg = server.arg("index");
    const String action = server.arg("action");

    // Reject empty, signed and non-numeric indices.
    if (!indexArg.length()) {
        sendJson(400, "{\"error\":\"invalid_index\"}");
        return;
    }

    for (size_t i = 0; i < indexArg.length(); ++i) {
        if (indexArg[i] < '0' || indexArg[i] > '9') {
            sendJson(400, "{\"error\":\"invalid_index\"}");
            return;
        }
    }

    const long index = indexArg.toInt();

    if (index < 0 || index >= Config::get().relayCount ||
        index >= MAX_RELAY) {
        sendJson(400, "{\"error\":\"index_out_of_range\"}");
        return;
    }

    bool ok = false;

    if (action == "on") {
        ok = Relay::set(static_cast<uint8_t>(index), true);
    } else if (action == "off") {
        ok = Relay::set(static_cast<uint8_t>(index), false);
    } else if (action == "toggle") {
        ok = Relay::toggle(static_cast<uint8_t>(index));
    } else {
        sendJson(400, "{\"error\":\"invalid_action\"}");
        return;
    }

    if (!ok) {
        sendJson(500, "{\"error\":\"relay_command_failed\"}");
        return;
    }

    String response = F("{\"index\":");
    response += String(index);
    response += F(",\"on\":");
    response += Relay::get(static_cast<uint8_t>(index))
                    ? F("true")
                    : F("false");
    response += '}';

    sendJson(200, response);
}

void handleAdminConfigGet() {
    if (!requireAdmin()) return;

    const AppConfig& cfg = Config::get();

    String json;
    json.reserve(3000);

    json += F("{\"relayCount\":");
    json += String(cfg.relayCount);
    json += F(",\"relays\":[");

    for (uint8_t i = 0; i < MAX_RELAY; ++i) {
        if (i) json += ',';

        json += F("{\"index\":");
        json += String(i);
        json += F(",\"name\":");
        appendJsonString(json, cfg.relays[i].name);
        json += F(",\"comment\":");
        appendJsonString(json, cfg.relays[i].comment);
        json += F(",\"startupOn\":");
        json += cfg.relays[i].startupOn ? F("true") : F("false");
        json += '}';
    }

    json += F("]}");
    sendJson(200, json);
}

bool parseUnsignedArg(const String& value, uint8_t& result) {
    if (!value.length()) return false;

    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] < '0' || value[i] > '9') return false;
    }

    const long n = value.toInt();
    if (n < 1 || n > MAX_RELAY) return false;

    result = static_cast<uint8_t>(n);
    return true;
}

void copyArg(char* dst, size_t size, const String& value) {
    if (!size) return;

    size_t n = value.length();
    if (n >= size) n = size - 1;

    memcpy(dst, value.c_str(), n);
    dst[n] = '\0';
}

void handleAdminConfigPost() {
    if (!requireAdmin()) return;

    if (!server.hasArg("relayCount")) {
        sendJson(400, "{\"error\":\"relayCount_required\"}");
        return;
    }

    uint8_t newCount = 0;
    if (!parseUnsignedArg(server.arg("relayCount"), newCount)) {
        sendJson(400, "{\"error\":\"invalid_relayCount\"}");
        return;
    }

    AppConfig& cfg = Config::get();

    // Validate all submitted relay fields before changing anything.
    for (uint8_t i = 0; i < newCount; ++i) {
        const String nameKey = "name" + String(i);
        const String commentKey = "comment" + String(i);
        const String startupKey = "startup" + String(i);

        if (!server.hasArg(nameKey) ||
            !server.hasArg(commentKey) ||
            !server.hasArg(startupKey)) {
            sendJson(400, "{\"error\":\"missing_relay_fields\"}");
            return;
        }

        if (server.arg(nameKey).length() >= RELAY_NAME_SIZE ||
            server.arg(commentKey).length() >= RELAY_COMMENT_SIZE) {
            sendJson(400, "{\"error\":\"field_too_long\"}");
            return;
        }

        const String startup = server.arg(startupKey);
        if (startup != "0" && startup != "1") {
            sendJson(400, "{\"error\":\"invalid_startup_state\"}");
            return;
        }
    }

    // Turn off outputs that are being removed from the active range.
    for (uint8_t i = newCount; i < cfg.relayCount; ++i) {
        Relay::set(i, false);
    }

    for (uint8_t i = 0; i < newCount; ++i) {
        copyArg(cfg.relays[i].name, RELAY_NAME_SIZE,
                server.arg("name" + String(i)));
        copyArg(cfg.relays[i].comment, RELAY_COMMENT_SIZE,
                server.arg("comment" + String(i)));

        cfg.relays[i].startupOn =
            server.arg("startup" + String(i)) == "1";
    }

    cfg.relayCount = newCount;

    if (!Config::saveNow()) {
        sendJson(500, "{\"error\":\"config_save_failed\"}");
        return;
    }

    sendJson(200, "{\"ok\":true}");
}

void handleNotFound() {
    if (server.uri().startsWith("/api/")) {
        sendJson(404, "{\"error\":\"not_found\"}");
    } else {
        server.send(404, "text/plain; charset=utf-8",
                    "Not found");
    }
}
} // namespace

namespace WebServerModule {

void begin() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/admin", HTTP_GET, handleAdminPage);

    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/relay", HTTP_POST, handleRelayCommand);

    server.on("/api/admin/config", HTTP_GET, handleAdminConfigGet);
    server.on("/api/admin/config", HTTP_POST, handleAdminConfigPost);

    server.on("/style.css", HTTP_GET, []() {
        sendFile("/style.css", "text/css; charset=utf-8");
    });

    server.on("/app.js", HTTP_GET, []() {
        sendFile("/app.js", "application/javascript; charset=utf-8");
    });

    server.on("/admin.js", HTTP_GET, []() {
        sendFile("/admin.js", "application/javascript; charset=utf-8");
    });

    server.onNotFound(handleNotFound);
    server.begin();

    Serial.println(F("[Web] HTTP server started on port 80"));
    Serial.print(F("[Web] Open: http://"));
    Serial.println(WiFi.softAPIP());
}

void loop() {
    server.handleClient();
}

} // namespace WebServerModule