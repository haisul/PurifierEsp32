#ifndef __FUNCCONTROLLER_H_
#define __FUNCCONTROLLER_H_
#include "littlefsfun.h"
#include "loggerESP.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define ONWIFICONNECTED_SIGNATURE std::function<void()> onWifiConnected

class wifiController {
private:
    String ssid;
    String password;
    String ip;
    String rssi;

    static SemaphoreHandle_t reconnectMutex;

    ONWIFICONNECTED_SIGNATURE;
    static wifiController *instance;
    static void WiFiEvent(WiFiEvent_t);
    void saveToJson(String, String, String, String);
    bool loadFromJson();

    static void smartConfig(void *);
    static void wifiReconnect(void *);

public:
    wifiController();
    void connect();
    String getInfo();
    wifiController &setWiFiConnectedCallback(ONWIFICONNECTED_SIGNATURE);
};
extern wifiController *instance;

#endif