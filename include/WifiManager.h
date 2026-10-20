#pragma once
namespace WifiManager {
    void begin();
    void loop();
    void reconfigure();
    bool connected();
    bool inSetupMode();
}
