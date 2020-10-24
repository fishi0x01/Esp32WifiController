#include "Esp32WifiController.h"

#include "BluetoothSerial.h"
#include "Preferences.h"

BluetoothSerial SerialBT;
Preferences prefs;
TaskHandle_t loopTask = NULL;

bool btActive = false;
char* bluetoothName = "";
int pushbuttonPin = -1;

String bluetoothMessage = String();

String wifiPassword = "";
String wifiSsid = "";

// Storage
char* wifiPasswordStorageKey = "wifiPass";
char* wifiSsidStorageKey = "wifiSsid";
char* storageNamespace = "esp32_wifi_ctrl";

void persistWifiCredentials()
{
    // open storage in RW mode
    prefs.begin(storageNamespace, false);
    prefs.putString(wifiPasswordStorageKey, wifiPassword);
    prefs.putString(wifiSsidStorageKey, wifiSsid);
    prefs.end();
}

void readWifiCredentials()
{
    // open storage in R mode
    prefs.begin(storageNamespace, true);
    wifiPassword = prefs.getString(wifiPasswordStorageKey);
    wifiSsid = prefs.getString(wifiSsidStorageKey);
    prefs.end();
}

void connectWifi()
{
    if (wifiPassword.length() == 0 || wifiSsid.length() == 0) {
        return;
    }
    Serial.println("Try connecting to Wifi");
    char ssidBuf[wifiSsid.length() + 1];
    char passBuf[wifiPassword.length() + 1];
    wifiSsid.toCharArray(ssidBuf, wifiSsid.length());
    wifiPassword.toCharArray(passBuf, wifiPassword.length());
    int status = WiFi.begin(ssidBuf, passBuf);
    int waitCountdown = 10;
    while (status != WL_CONNECTED && status != WL_CONNECT_FAILED &&
           status != WL_NO_SSID_AVAIL && waitCountdown > 0) {
        vTaskDelay(1000);
        waitCountdown--;
    }
}

void bluetoothProcessMessage()
{
    if (bluetoothMessage.startsWith("ssid ")) {
        wifiSsid = bluetoothMessage.substring(5);
    }
    if (bluetoothMessage.startsWith("password ")) {
        wifiPassword = bluetoothMessage.substring(9);
    }
    // TODO: use .equals()
    if (bluetoothMessage.startsWith("connect")) {
        persistWifiCredentials();
        connectWifi();
    }
    bluetoothMessage = String();
}

void bluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param)
{
    if (event != ESP_SPP_DATA_IND_EVT) return;
    int numMessages = SerialBT.available();
    while (numMessages > 0) {
        char inputChar = SerialBT.read();
        if (inputChar == '\n') {
            bluetoothProcessMessage();
        }
        else {
            // TODO: use char*
            bluetoothMessage += String(inputChar);
        }
        numMessages--;
    }
}

void loop(void* params)
{
    int btActivityCountdown = 0;
    while (1) {
        if (btActivityCountdown > 0) btActivityCountdown--;

        if (btActivityCountdown == 0 && !SerialBT.hasClient() &&
            SerialBT.isReady()) {
            SerialBT.end();
            Serial.println("Ended BT");
        }

        if (digitalRead(pushbuttonPin) == HIGH && btActivityCountdown == 0) {
            Serial.print("Start BT Device: ");
            Serial.println(bluetoothName);
            SerialBT.begin(bluetoothName);
            btActivityCountdown = 30;
            Serial.println("Start BT: Success");
        }
        vTaskDelay(500);
    }
}

void Esp32WifiController::begin(char* btName, int pbPin)
{
    bluetoothName = btName;
    pushbuttonPin = pbPin;
    readWifiCredentials();
    connectWifi();
    SerialBT.register_callback(bluetoothCallback);
    xTaskCreate(loop, "WifiController", 10000, NULL, 1, &loopTask);
    configASSERT(loopTask);
    Serial.println("Started Wifi Controller Task");
}

void Esp32WifiController::end()
{
    vTaskDelete(loopTask);
    if (SerialBT.isReady()) SerialBT.end();
}

Esp32WifiController::~Esp32WifiController() {}
Esp32WifiController::Esp32WifiController() {}
