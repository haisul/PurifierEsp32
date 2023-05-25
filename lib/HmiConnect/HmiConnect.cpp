#include "HmiConnect.h"

HmiConnect::HmiConnect(HardwareSerial HMI, int monitor_speed) : _HMI(HMI), _monitor_speed(monitor_speed) {
    _HMI.begin(_monitor_speed);
    //_HMI.printf("rest\xff\xff\xff");
    while (_HMI.read() >= 0) {
    };
}

HmiConnect &HmiConnect::SetCallback(HMI_CALLBACK_SIGNATURE) {
    this->hmi_callback = hmi_callback;
    return *this;
};

HmiConnect &HmiConnect::SetDeveloperMode(HMI_CALLBACK_SIGNATURE) {
    this->hmi_developer = hmi_developer;
    return *this;
};

void HmiConnect::loop() {

    while (_HMI.available()) {
        char c = _HMI.read();
        if (c == 0x24 && _HMI.available() > 5) {
            msgBuffer = "";
            while (_HMI.available()) {
                char c = _HMI.read();
                if (c == '\n') {
                    msgBuffer.trim();
                    if (millis() - previousMillis < interval && msgBuffer == previousMessage) {
                        sendMessage("***");
                        break;
                    }
                    previousMessage = msgBuffer;
                    Serial.printf("%s\n", msgBuffer.c_str());
                    sendMessage("*");
                    hmi_callback(msgBuffer);
                    break;
                } else
                    msgBuffer += c;
            }
        } else if (c == 0x87) {
            Serial.println("weakup");
            hmi_callback("refresh");
            while (_HMI.available() > 0)
                _HMI.read();
        } else {
            Serial.print(c, HEX);
            Serial.printf(" ");
        }
        previousMillis = millis();
    }
}

void HmiConnect::sendMessage(String HMI_msg) {
    logger.w(HMI_msg.c_str());
    //_HMI.printf("%s\xff\xff\xff", HMI_msg.c_str());
    _HMI.printf("%s", HMI_msg.c_str());
}

void HmiConnect::enable() {
    _HMI.begin(_monitor_speed);
    _HMI.printf("rest\xff\xff\xff");
    while (_HMI.read() >= 0) {
    };
}

void HmiConnect::disable() {
    _HMI.end();
    while (_HMI.read() >= 0) {
    };
}

String formatMessage(String func, String set, String state) {
    String page = "home", val = "0";
    if (state == "on")
        val = "1";
    else if (state == "off")
        val = "0";
    else
        val = state;

    if (set == "speed" || set.startsWith("mode"))
        page = "home";
    else if (set == "state")
        page = "mode"; // HMI上的mode頁面
    else if (set == "count" || set == "time")
        page = "settime";

    return page + "." + func + "_" + set + ".val=" + val;
};

/*String HmiConnect::escapeJson(String jsonString) {
    String escapedString = "";

    for (int i = 0; i < jsonString.length(); i++) {
        char c = jsonString.charAt(i);

        switch (c) {
        case '\"':
            escapedString += "\\\"";
            break;
        case '\\':
            escapedString += "\\\\";
            break;
        case '\b':
            escapedString += "\\b";
            break;
        case '\f':
            escapedString += "\\f";
            break;
        case '\n':
            escapedString += "\\n";
            break;
        case '\r':
            escapedString += "\\r";
            break;
        case '\t':
            escapedString += "\\t";
            break;
        default:
            if (c < 32 || c > 126) {
                escapedString += String("\\u") + String(c, HEX);
            } else {
                escapedString += c;
            }
            break;
        }
    }

    return escapedString;
}*/