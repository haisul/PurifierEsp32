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
    if (loadWifiData())
        getSSID(wifiIndex);
    if (ssid != nullptr && password != nullptr) {
        logger.iL("SSID: %s PASSWORD: %s", ssid.c_str(), password.c_str());
        WiFi.begin(ssid.c_str(), password.c_str());
        logger.iL("Wifi connecting");
    } else {
        xTaskCreatePinnedToCore(smartConfig, "smartConfig", 3000, NULL, 1, NULL, 0);
    }
}

void wifiController::getSSID(int index) {

    int listLengh = j_wifiList.size();

    ssid = j_wifiList[index]["SSID"].as<String>();
    password = j_wifiList[index]["PASSWORD"].as<String>();

    if (wifiIndex < listLengh) {
        wifiIndex++;
        wifiIndex = wifiIndex % listLengh;
    }
}

void wifiController::WiFiEvent(WiFiEvent_t event) {
    logger.iL("[WiFi-event] event: %d", event);
    switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        logger.iL("Wifi Connected!");
        instance->onWifiConnected();
        instance->saveInfo(WiFi.SSID(), WiFi.psk(), WiFi.localIP().toString(), String(WiFi.RSSI()));
        instance->addWifi(WiFi.SSID(), WiFi.psk());
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
    logger.iL("SmartConfig Connecting...");
    while (millis() - smartConfigTime < 59 * 1000) {

        if (WiFi.isConnected()) {
            logger.iL("SmartConfig Done!");
            break;
        }
        vTaskDelay(500);
    }
    if (!WiFi.isConnected())
        logger.eL("SmartConfig Failed!");

    WiFi.stopSmartConfig();
    logger.iL("SmartConfig end!");
    vTaskDelete(NULL);
}

void wifiController::wifiReconnect(void *parameter) {
    logger.eL("Wifi Reconnected!");
    uint8_t wifiReconnectCount = 0;
    uint16_t delaytime = 3000;
    while (1) {
        if (WiFi.isConnected())
            break;
        if (wifiReconnectCount > 5)
            instance->getSSID(instance->wifiIndex);
        if (wifiReconnectCount == 50) {
            instance->wifiIndex = 0;
            WiFi.mode(WIFI_OFF);
            logger.iL("Wifi OFF!");
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

void wifiController::saveInfo(String ssid, String password, String ip, String rssi) {
    this->ssid = ssid;
    this->password = password;
    this->ip = ip;
    this->rssi = rssi;
}

void wifiController::addWifi(String SSID, String PASS) {
    String wifiStr;
    bool saved = false;

    int listLengh = j_wifiList.size();
    if (listLengh < 10) {
        for (int i = 0; i <= listLengh; i++) {
            if (j_wifiList[i]["SSID"] == SSID) {
                if (j_wifiList[i]["PASSWORD"] == PASS) {
                    logger.iL("Wifi data is same");
                    return;
                } else {
                    j_wifi["SSID"] = SSID;
                    j_wifi["PASSWORD"] = PASS;
                    j_wifiList[i] = j_wifi;
                    saved = true;
                    break;
                }
            }
        }
        if (!saved) {
            j_wifi["SSID"] = SSID;
            j_wifi["PASSWORD"] = PASS;
            j_wifiList[listLengh] = j_wifi;
        }

        serializeJson(j_wifiList, wifiStr);
        logger.iL(wifiStr.c_str());
        writeFile2(LittleFS, "/wifi/wifi.txt", wifiStr.c_str());
        logger.iL("Wifi data is saved");
    } else {
        logger.eL("Wifi data over the max size");
    }
}

void wifiController::removeWifi(String SSID) {
    String wifiStr;
    int listLengh = j_wifiList.size();
    for (int i = 0; i < listLengh; i++) {
        if (j_wifiList[i]["SSID"] == SSID) {
            j_wifiList.remove(i);
            break;
        }
    }
    logger.iL(wifiStr.c_str());
    serializeJson(j_wifiList, wifiStr);
    writeFile2(LittleFS, "/wifi/wifi.txt", wifiStr.c_str());
    logger.iL("Wifi data is saved");
}

bool wifiController::loadWifiData() {
    if (!initLittleFS()) {
        logger.eL("load wifi data Failed!");
        return false;
    } else {
        String readWifiStr = readFile(LittleFS, "/wifi/wifi.txt");
        DeserializationError error = deserializeJson(j_wifiList, readWifiStr);
        if (error) {
            logger.eL("deserialize wifi data Failed: %s", error.c_str());
            return false;
        }
        logger.wL(readWifiStr.c_str());
        logger.iL("WIFI data load complete");
        return true;
    }
}

void wifiController::convertWifi(String readWifiStr) {
    DeserializationError error = deserializeJson(j_wifi, readWifiStr);
    if (error) {
        logger.eL("deserialize wifi data Failed: %s", error.c_str());
        return;
    }
    String ssid = j_wifi["SSID"];
    String pass = j_wifi["PASSWORD"];
    addWifi(ssid, pass);
}

String wifiController::getInfo() {
    String wifiInfoStr;
    StaticJsonDocument<192> wifiInfo;

    wifiInfo["SSID"] = ssid;
    wifiInfo["PASSWORD"] = password;
    wifiInfo["IP"] = ip;
    wifiInfo["RSSI"] = rssi;

    serializeJson(wifiInfo, wifiInfoStr);

    return wifiInfoStr;
}
