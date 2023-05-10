#include "funcControl.h"

funcControl::funcControl() {
    pinMode(fogpump, OUTPUT);    // Relay1-8 fog Pump
    pinMode(fogfan, OUTPUT);     // Relay1-7 fog Fan
    pinMode(fogMachine, OUTPUT); // Relay2-2 fog Machine
    pinMode(ecin, OUTPUT);       // L298NA-IN1 ec Direction
    pinMode(ecout, OUTPUT);      // L298NA-IN2 ec Direction
    pinMode(uvLamp, OUTPUT);     // Relay1-1 uv Lamp

    pinMode(purifier, OUTPUT); // Relay2-1 purifier Fan
    pinMode(PWMpin, OUTPUT);   // purifier fan PWMpin

    digitalWrite(fogpump, LOW);
    digitalWrite(fogfan, LOW);
    digitalWrite(fogMachine, LOW);
    digitalWrite(ecin, LOW);
    digitalWrite(ecout, LOW);
    digitalWrite(uvLamp, LOW);

    digitalWrite(purifier, LOW);
    ledcAttachPin(PWMpin, PWMchannel);
    ledcSetup(PWMchannel, _freq, resolution);

    all.name = "all";
    pur.name = "pur";
    fog.name = "fog";
    uvc.name = "uvc";
}

void funcControl::funcState(function_mode_types mode_types, bool state, bool publish) {
    if (state) {
        switch (mode_types) {
        case PUR:
            if (!pur.state) {
                vTaskDelay(500);
                digitalWrite(purifier, HIGH);
                pur.state = true;
                mapDutycycle(nowPurMode);
                logger.i("pur.state:ON");
            }
            break;

        case FOG:
            if (!fog.state) {
                digitalWrite(fogMachine, HIGH);
                vTaskDelay(500);
                digitalWrite(fogpump, HIGH);
                digitalWrite(fogfan, HIGH);
                fog.state = true;
                logger.i("fog.state:ON");
            }
            break;
        case UVC:
            if (!uvc.state) {
                digitalWrite(ecin, LOW);
                digitalWrite(ecout, LOW);
                digitalWrite(uvLamp, HIGH);
                digitalWrite(ecout, HIGH);
                vTaskDelay(6000);
                digitalWrite(ecout, LOW);
                uvc.state = true;
                logger.i("uvc.state:ON");
            }
            break;
        case ALL:
            if (!all.state) {
                funcState(PUR, ON);
                funcState(FOG, ON);
                funcState(UVC, ON);
                all.state = true;
                logger.i("all.state:ON");
            }
            break;

        default:
            logger.e("Message error");
            break;
        }
    } else {
        switch (mode_types) {
        case PUR:
            if (pur.state) {
                vTaskDelay(500);
                digitalWrite(purifier, LOW);
                pur.state = false;
                logger.i("pur.state:OFF");
            }
            break;

        case FOG:
            if (fog.state) {
                digitalWrite(fogMachine, LOW);
                vTaskDelay(2000);
                digitalWrite(fogpump, LOW);
                digitalWrite(fogfan, LOW);
                fog.state = false;
                logger.i("fog.state:OFF");
            }
            break;
        case UVC:
            if (uvc.state) {
                digitalWrite(ecin, LOW);
                digitalWrite(ecout, LOW);
                digitalWrite(uvLamp, LOW);
                digitalWrite(ecin, HIGH);
                vTaskDelay(6000);
                digitalWrite(ecin, LOW);
                uvc.state = false;
                logger.i("uvc.state:OFF");
            }
            break;
        case ALL:
            if (all.state) {
                funcState(PUR, OFF);
                funcState(FOG, OFF);
                funcState(UVC, OFF);
                all.state = false;
                logger.i("all.state:OFF");
            }
            break;

        default:
            logger.e("Message error\n");
            break;
        }
    }
    startCountTime(mode_types);
    if (publish) {
        saveToJson();
        refresh();
    }
}

void funcControl::purMode(function_mode_types mode) {

    switch (mode) {
    case AUTO:
        _modeAuto = true;
        _modeSleep = false;
        _modeManual = false;
        logger.i("AutoMode:ON");
        break;

    case SLEEP:
        _modeAuto = false;
        _modeSleep = true;
        _modeManual = false;
        logger.i("SleepMode:ON");
        break;

    case MANUAL:
        _modeAuto = false;
        _modeSleep = false;
        _modeManual = true;
        logger.i("ManualMode:ON");
        break;

    default:
        return;
    }
    mapDutycycle(mode);
}

void funcControl::mapDutycycle(function_mode_types mode) {
    switch (mode) {
    case AUTO:
        _dutycycle = 255 / 4 + (255 * 3 / 4) * _dust / 500;
        nowPurMode = AUTO;
        break;

    case SLEEP:
        _dutycycle = 40;
        nowPurMode = SLEEP;
        break;

    case MANUAL:
        _dutycycle = manualDutycycle * 2.55;
        nowPurMode = MANUAL;
        break;

    default:
        return;
    }

    if (_dutycycle > 225)
        _dutycycle = 225;
    else if (_dutycycle < 40)
        _dutycycle = 40;
    ledcWrite(PWMchannel, _dutycycle);
    logger.i("pur_speed:%d", _dutycycle);
}

void funcControl::getDustVal(int dustVal = 0) {
    _dust = dustVal;
    _dutycycle = 255 / 4 + (255 * 3 / 4) * _dust / 500;
    if (_dutycycle > 225)
        _dutycycle = 225;
    else if (_dutycycle < 30)
        _dutycycle = 30;
    ledcWrite(PWMchannel, _dutycycle);
}

void funcControl::getDutycycle(int val) {
    manualDutycycle = val;
    _dutycycle = val * 2.55;
    if (_dutycycle > 225)
        _dutycycle = 225;
    else if (_dutycycle < 30)
        _dutycycle = 30;
    ledcWrite(PWMchannel, _dutycycle);
    logger.i("dutyCycle: %d", _dutycycle);
}

void funcControl::countSet(function_mode_types mode_types, bool state) {
    switch (mode_types) {
    case PUR:
        pur.countState = state;
        logger.i("pur.countState : %s", pur.countState ? "ON" : "OFF");
        break;
    case FOG:
        fog.countState = state;
        logger.i("fog.countState : %s", fog.countState ? "ON" : "OFF");
        break;
    case UVC:
        uvc.countState = state;
        logger.i("uvc.countState : %s", uvc.countState ? "ON" : "OFF");
        break;
    case ALL:
        all.countState = state;
        logger.i("all.countState : %s", all.countState ? "ON" : "OFF");
    default:
        break;
    }
    startCountTime(mode_types);
}

void funcControl::timeSet(function_mode_types mode_types, int time) {
    switch (mode_types) {
    case PUR:
        pur.time = time;
        logger.i("pur.setTime = %d", pur.time);
        break;
    case FOG:
        fog.time = time;
        logger.i("fog.setTime = %d", fog.time);
        break;
    case UVC:
        uvc.time = time;
        logger.i("uvc.setTime = %d", uvc.time);
        break;
    case ALL:
        all.time = time;
        logger.i("all.setTime = %d", all.time);
    default:
        break;
    }
    startCountTime(mode_types);
}

void funcControl::commend(String mode_types, String set, String state) {
    bool status;
    int val;
    if (state == "on" || state == "true")
        status = true;
    else if (state == "off" || state == "false")
        status = false;
    else if (state != "")
        val = atoi(state.c_str());

    if (set == "state")
        funcState(convert(mode_types), status);
    else if (set == "count")
        countSet(convert(mode_types), status);
    else if (set == "time")
        timeSet(convert(mode_types), val);
    else if (set.startsWith("mode"))
        purMode(convert(set));
    else if (set == "speed")
        getDutycycle(val);

    saveToJson();

    if (set.startsWith("step")) {
        set.remove(0, 4);
        devMode(set, status);
    }
    if (mode_types == "system")
        systemCommend(set, state);
}

void funcControl::systemCommend(String set, String state) {
    if (set == "reboot")
        ESP.restart();
    if (set == "reset")
        reset();
    if (set == "wifi") {
        if (state == "on")
            MCUcommender("wifion");
        else if (state == "off")
            MCUcommender("wifioff");
    }
    if (set == "wifiConfig")
        MCUcommender(set);
    if (set == "wifiInfo") {
        int index = state.indexOf('/');
        SSID = state.substring(0, index);
        PASSWORD = state.substring(index + 1);
        MCUcommender(set);
    }
}

void funcControl::startCountTime(function_mode_types mode_types) {
    status *func = nullptr;

    switch (mode_types) {
    case PUR:
        func = &pur;
        break;
    case FOG:
        func = &fog;
        break;
    case UVC:
        func = &uvc;
        break;
    case ALL:
        func = &all;
        break;
    default:
        return;
    }

    if (func->state && func->countState) {
        func->endEpoch = rtc.getEpoch() + func->time;
        func->startingCount = true;
    } else
        func->startingCount = false;
}

funcControl &funcControl::setMCUcommender(MCU_COMMENDER_SIGNATURE) {
    this->MCUcommender = MCUcommender;
    return *this;
};

void funcControl::refresh(String target) {
    MCUcommender(target);
}

void funcControl::saveToJson() {
    StaticJsonDocument<384> j_initial;

    j_initial["pur.state"] = pur.state;
    j_initial["fog.state"] = fog.state;
    j_initial["uvc.state"] = uvc.state;
    j_initial["all.state"] = all.state;
    j_initial["pur.countState"] = pur.countState;
    j_initial["fog.countState"] = fog.countState;
    j_initial["uvc.countState"] = uvc.countState;
    j_initial["all.countState"] = all.countState;
    j_initial["pur.time"] = pur.time;
    j_initial["fog.time"] = fog.time;
    j_initial["uvc.time"] = uvc.time;
    j_initial["all.time"] = all.time;
    j_initial["_modeAuto"] = _modeAuto;
    j_initial["_modeSleep"] = _modeSleep;
    j_initial["_modeManual"] = _modeManual;
    j_initial["manualDutycycle"] = manualDutycycle;
    j_initial["non"] = "non";

    String initialStr;
    serializeJson(j_initial, initialStr);
    writeFile2(LittleFS, "/initial/initial.txt", initialStr.c_str());
}

void funcControl::initialJson() {

    String readinitialStr = readFile(LittleFS, "/initial/initial.txt");

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, readinitialStr);

    if (error) {
        logger.e("deserializeJson() failed: %s", error.c_str());
        return;
    }

    pur.countState = doc["pur.countState"];
    fog.countState = doc["fog.countState"];
    uvc.countState = doc["uvc.countState"];
    all.countState = doc["all.countState"];
    pur.time = doc["pur.time"];
    fog.time = doc["fog.time"];
    uvc.time = doc["uvc.time"];
    all.time = doc["all.time"];
    _modeAuto = doc["_modeAuto"];
    _modeSleep = doc["_modeSleep"];
    _modeManual = doc["_modeManual"];
    manualDutycycle = doc["manualDutycycle"];

    refresh();
    saveToJson();
    logger.i("initial LittleFS Complete!");
}

void funcControl::MachineInitial() {
    if (!initLittleFS()) {
        logger.e("initial LittleFS Failed!");
        return;
    } else {
        initialJson();
    }
}

String funcControl::getInitialJson() {
    String readinitialStr = readFile(LittleFS, "/initial/initial.txt");
    return readinitialStr;
}

void funcControl::reset() {
    deleteFile(LittleFS, "/initial/initial.txt");
    deleteFile(LittleFS, "/initial/topic.txt");
    deleteFile(LittleFS, "/wifi/wifi.txt");
    ESP.restart();
}

function_mode_types convert(String str) {
    if (str == "all")
        return ALL;
    else if (str == "pur")
        return PUR;
    else if (str == "fog")
        return FOG;
    else if (str == "uvc")
        return UVC;
    else if (str == "modeAuto")
        return AUTO;
    else if (str == "modeSleep")
        return SLEEP;
    else if (str == "modeManual")
        return MANUAL;
    else if (str == "system")
        return SYSTEM;
    else
        return Failed;
}

void funcControl::devMode(String set, bool state) {
    if (set == "light") {
        if (state)
            digitalWrite(uvLamp, HIGH);
        else
            digitalWrite(uvLamp, LOW);
    } else if (set == "ec") {
        if (state) {
            digitalWrite(ecin, LOW);
            digitalWrite(ecout, LOW);
            digitalWrite(ecout, HIGH);
            vTaskDelay(200);
            digitalWrite(ecout, LOW);
        } else {
            digitalWrite(ecin, LOW);
            digitalWrite(ecout, LOW);
            digitalWrite(ecin, HIGH);
            vTaskDelay(200);
            digitalWrite(ecin, LOW);
        }
    } else if (set == "fog") {
        if (state)
            digitalWrite(fogMachine, HIGH);
        else
            digitalWrite(fogMachine, LOW);
    } else if (set == "pump") {
        if (state)
            digitalWrite(fogpump, HIGH);
        else
            digitalWrite(fogpump, LOW);
    } else if (set == "fan") {
        if (state)
            digitalWrite(fogfan, HIGH);
        else
            digitalWrite(fogfan, LOW);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void funcControl::wifiSaveToJson(String SSID, String PASSWORD, String IP, String RSSI) {
    StaticJsonDocument<384> j_wifi;
    String wifiStr;

    j_wifi["SSID"] = SSID;
    j_wifi["PASSWORD"] = PASSWORD;
    j_wifi["IP"] = IP;
    j_wifi["RSSI"] = RSSI;

    serializeJson(j_wifi, wifiStr);
    writeFile2(LittleFS, "/wifi/wifi.txt", wifiStr.c_str());
    logger.i("WIFI SSID PASSWORD SAVED!");
}

String funcControl::getWifiInfo() {
    String readWifiInfoStr = readFile(LittleFS, "/wifi/wifi.txt");
    return readWifiInfoStr;
}

void funcControl::initialWifi() {

    if (!initLittleFS()) {
        logger.e("initial LittleFS Failed!");
        return;
    } else {
        String readWifiStr = readFile(LittleFS, "/wifi/wifi.txt");
        StaticJsonDocument<512> wifi;
        DeserializationError error = deserializeJson(wifi, readWifiStr);

        if (error) {
            logger.e("deserialize WIFI Json() failed: %s", error.c_str());
            return;
        }

        String ssid = wifi["SSID"];
        String pass = wifi["PASSWORD"];

        SSID = ssid;
        PASSWORD = pass;
        logger.i("WIFI initial complete\n");
    }
}