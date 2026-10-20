#include "WebServer.h"
#include "Config.h"
#include "WifiManager.h"
#include <ESP8266WebServer.h>

namespace {
ESP8266WebServer server(80);
void handleRoot() {
    // Temporary diagnostic page. Admin authentication and full UI
    // must be implemented before exposing the controller to a network.
    String page = F("<!doctype html><html><head><meta charset='utf-8'>"
                    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                    "<title>Smart Lights</title></head><body>"
                    "<h1>Smart Lights</h1><p>Web UI is not implemented yet.</p>");
    if (Config::isReady()) {
        page += F("<p>Active relays: ");
        page += String(Config::get().relayCount);
        page += F("</p>");
    }
    if (WifiManager::inSetupMode()) page += F("<p>Setup access point is active.</p>");
    page += F("</body></html>");
    server.send(200, "text/html; charset=utf-8", page);
}
}

namespace WebServerModule {
void begin() {
    server.on("/", HTTP_GET, handleRoot);
    server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
    server.begin();
    Serial.println(F("[Web] HTTP server started"));
}
void loop() {
    server.handleClient();
}
}
