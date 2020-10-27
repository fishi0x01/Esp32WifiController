#include "Esp32WifiController.h"

Esp32WifiController wifiController;

const int PUSHBUTTON_PIN = 15;
char* BLUETOOTH_NAME = "MyESP32";

void setup() { wifiController.begin(BLUETOOTH_NAME, PUSHBUTTON_PIN); }

void loop() {}
