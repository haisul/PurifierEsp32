#ifndef __HMICONNECT_
#define __HMICONNECT_
#include "loggerESP.h"
#include <Arduino.h>

#define HMI_CALLBACK_SIGNATURE std::function<void(String)> hmi_callback
#define HMI_DEVELOPER_SIGNATURE std::function<void(String)> hmi_developer
String formatMessage(String, String, String);

class HmiConnect {
private:
    HardwareSerial _HMI = 0;
    uint16_t _monitor_speed;
    String _HMI_msg;
    String msgBuffer = "";
    HMI_CALLBACK_SIGNATURE;
    HMI_DEVELOPER_SIGNATURE;

    unsigned long previousMillis = 0;
    unsigned long interval = 1000;
    String previousMessage;

public:
    HmiConnect(HardwareSerial, int);
    HmiConnect &SetCallback(HMI_CALLBACK_SIGNATURE);
    void loop();
    void sendMessage(String);
    HmiConnect &SetDeveloperMode(HMI_CALLBACK_SIGNATURE);
    void enable();
    void disable();
    String escapeJson(String);
};

#endif