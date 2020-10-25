#include "Esp32WifiController.h"

#include "BluetoothSerial.h"
#include "Preferences.h"

BluetoothSerial SerialBT;
Preferences prefs;
TaskHandle_t loopTask = NULL;

const char* LOG_TAG = "WIFICTRL";

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
    log_d("Stored %s at %s", wifiSsid, wifiSsidStorageKey);
    log_d("Stored %s at %s", wifiPassword, wifiPasswordStorageKey);
}

void readWifiCredentials()
{
    // open storage in R mode
    prefs.begin(storageNamespace, true);
    wifiPassword = prefs.getString(wifiPasswordStorageKey);
    wifiSsid = prefs.getString(wifiSsidStorageKey);
    prefs.end();
    log_d("Read %s from %s", wifiSsid, wifiSsidStorageKey);
    log_d("Read %s from %s", wifiPassword, wifiPasswordStorageKey);
}

void connectWifi()
{
    if (wifiPassword.length() == 0 || wifiSsid.length() == 0) {
        return;
    }
    log_d("Initiate WiFi.begin(%s, %s)", wifiSsid, wifiPassword);
    char ssidBuf[wifiSsid.length() + 1];
    char passBuf[wifiPassword.length() + 1];
    wifiSsid.toCharArray(ssidBuf, wifiSsid.length());
    wifiPassword.toCharArray(passBuf, wifiPassword.length());
    int status = WiFi.begin(ssidBuf, passBuf);
    int tries = 3;
    while (status != WL_CONNECTED && tries > 0) {
        log_d("WiFi.status() == %u", status);
        vTaskDelay(4000);
        status = WiFi.status();
        if (status != WL_IDLE_STATUS && status != WL_CONNECTED) {
            log_d("Re-try WiFi.begin()");
            WiFi.begin(ssidBuf, passBuf);
        }
        tries--;
    }
    log_d("WiFi.begin() finished with WiFi.status() == %u", status);
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
            log_d("Initiate SerialBT.end()");
            SerialBT.end();
        }

        if (digitalRead(pushbuttonPin) == HIGH && btActivityCountdown == 0) {
            log_d("Initiate SerialBT.begin('%s')", bluetoothName);
            SerialBT.begin(bluetoothName);
            btActivityCountdown = 30;
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
    log_i("Started Wifi Controller Task");
}

void Esp32WifiController::end()
{
    vTaskDelete(loopTask);
    if (SerialBT.isReady()) SerialBT.end();
}

Esp32WifiController::~Esp32WifiController() {}
Esp32WifiController::Esp32WifiController() {}
