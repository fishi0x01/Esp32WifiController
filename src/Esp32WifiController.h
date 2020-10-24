#ifndef __ESP32_WIFI_CONTROLLER_H__
#define __ESP32_WIFI_CONTROLLER_H__
#include "Arduino.h"
#include "WiFi.h"

class Esp32WifiController {
   public:
    Esp32WifiController(void);
    ~Esp32WifiController(void);

    void begin(char*, int);
    void end(void);
};

#endif  // __ESP32_WIFI_CONTROLLER_H__
