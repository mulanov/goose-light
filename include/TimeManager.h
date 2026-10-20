#pragma once
#include <time.h>

namespace TimeManager {
    void begin();
    void loop();
    bool isSynced();
    time_t now();
    bool getLocalTime(tm& result);
}
