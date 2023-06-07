#include "wifiController.h"

SemaphoreHandle_t wifiController::reconnectMutex;
wifiController *wifiController::instance = nullptr;

wifiController::wifiController() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.onEvent(WiFiEvent);
    instance = this;

    reconnectMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(reconnectMutex);
}

void wifiController::connect() {
    loadFromJson();
    if (ssid != nullptr && password != nullptr) {
        logger.i("SSID: %s\nPASSWORD: %s", ssid.c_str(), password.c_str());
        WiFi.begin(ssid.c_str(), password.c_str());
    } else {
        xTaskCreatePinnedToCore(smartConfig, "smartConfig", 3000, NULL, 1, NULL, 0);
    }
}

void wifiController::WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        logger.i("Wifi is Connected!");
        instance->onWifiConnected();
        instance->saveToJson(WiFi.SSID(), WiFi.psk(), WiFi.localIP().toString(), String(WiFi.RSSI()));
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:

        if (xSemaphoreTake(reconnectMutex, (TickType_t)10) == pdTRUE) {
            xTaskCreatePinnedToCore(wifiReconnect, "wifiReconnect", 3000, NULL, 1, NULL, 0);
        }
        break;
    }
}

void wifiController::smartConfig(void *parameter) {
    int smartConfigTime = millis();
    WiFi.beginSmartConfig();
    logger.i("SmartConfig start!");
    while (millis() - smartConfigTime < 59 * 1000) {

        if (WiFi.isConnected()) {
            logger.i("smartConfig Done!");
            break;
        }
        vTaskDelay(500);
    }
    if (!WiFi.isConnected())
        logger.e("smartConfig Failed!");

    WiFi.stopSmartConfig();
    logger.i("SmartConfig end!");
    vTaskDelete(NULL);
}

void wifiController::wifiReconnect(void *parameter) {
    logger.e("Wifi is disconnected!");
    uint8_t wifiReconnectCount = 0;
    uint16_t delaytime = 3000;
    while (1) {
        if (WiFi.isConnected())
            break;
        if (wifiReconnectCount > 5)
            delaytime = 60000;
        if (wifiReconnectCount == 10) {
            WiFi.mode(WIFI_OFF);
            logger.e("Wifi OFF!");
        }
        wifiReconnectCount++;
        instance->connect();
        vTaskDelay(delaytime);
    }
    xSemaphoreGive(instance->reconnectMutex);
    vTaskDelete(NULL);
}

wifiController &wifiController::setWiFiConnectedCallback(ONWIFICONNECTED_SIGNATURE) {
    this->onWifiConnected = onWifiConnected;
    return *this;
};

void wifiController::saveToJson(String ssid, String password, String ip, String rssi) {
    this->ssid = ssid;
    this->password = password;
    this->ip = ip;
    this->rssi = rssi;

    StaticJsonDocument<192> j_wifi;
    String wifiStr;

    j_wifi["SSID"] = ssid;
    j_wifi["PASSWORD"] = password;
    j_wifi["IP"] = ip;
    j_wifi["RSSI"] = rssi;

    serializeJson(j_wifi, wifiStr);
    writeFile2(LittleFS, "/wifi/wifi.txt", wifiStr.c_str());
    logger.i("WIFI SSID PASSWORD SAVED!");
}

bool wifiController::loadFromJson() {
    if (!initLittleFS()) {
        logger.e("initial LittleFS Failed!");
        return false;
    } else {
        String readWifiStr = readFile(LittleFS, "/wifi/wifi.txt");
        StaticJsonDocument<192> wifi;
        DeserializationError error = deserializeJson(wifi, readWifiStr);
        if (error) {
            logger.e("deserialize WIFI Json() failed: %s", error.c_str());
            return false;
        }
        ssid = wifi["SSID"].as<String>();
        password = wifi["PASSWORD"].as<String>();
        logger.i("WIFI initial complete\n");
        return true;
    }
}

String wifiController::getInfo() {
    String readWifiInfoStr = readFile(LittleFS, "/wifi/wifi.txt");
    return readWifiInfoStr;
}
