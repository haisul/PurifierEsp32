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

    uint8_t wifiIndex = 0;

    static SemaphoreHandle_t reconnectMutex;

    StaticJsonDocument<96> j_wifi;
    StaticJsonDocument<960> j_wifiList;

    ONWIFICONNECTED_SIGNATURE;
    static wifiController *instance;
    static void WiFiEvent(WiFiEvent_t);
    void saveInfo(String, String, String, String);
    bool loadWifiData();

    static void smartConfig(void *);
    static void wifiReconnect(void *);
    void getSSID(int);

public:
    wifiController();
    void addWifi(String SSID, String PASS);
    void removeWifi(String SSID);
    void convertWifi(String);
    void connect();
    String getInfo();
    wifiController &setWiFiConnectedCallback(ONWIFICONNECTED_SIGNATURE);
};
extern wifiController *instance;

#endif