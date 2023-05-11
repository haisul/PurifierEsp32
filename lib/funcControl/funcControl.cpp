#include "funcControl.h"

funcControl::funcControl() {
    funcState("all", false);
}

void funcControl::commend(String function, String set, String state) {
    bool status;
    int val;
    if (state == "on" || state == "true")
        status = true;
    else if (state == "off" || state == "false")
        status = false;
    else if (state != "")
        val = atoi(state.c_str());

    if (set == "state")
        funcState(function, status);
    else if (set == "count")
        countState(function, status);
    else if (set == "time")
        timeSet(function, val);
    else if (set.startsWith("mode"))
        changePurMode(set);
    else if (set == "speed")
        changePurSpeed(val);

    saveToJson();

    if (set.startsWith("step")) {
        set.remove(0, 4);
        devMode(function, set, status);
    }
    if (function == "system")
        systemCommend(set, state);
}

void funcControl::systemCommend(String set, String state) {
    if (set == "reboot")
        ESP.restart();
    if (set == "reset")
        MachineReset();
    if (set == "wifi") {
        if (state == "on")
            MCUcommender("wifion");
        else if (state == "off")
            MCUcommender("wifioff");
    }
    if (set == "wifiConfig")
        MCUcommender(set);
    /*if (set == "wifiInfo") {
        int index = state.indexOf('/');
        SSID = state.substring(0, index);
        PASSWORD = state.substring(index + 1);
        MCUcommender(set);
    }*/
}

void funcControl::funcState(String function, bool status) {
    if (function == "all")
        all.power(status);
    if (function == "all" || function == "pur")
        pur.power(status);
    if (function == "all" || function == "fog")
        fog.power(status);
    if (function == "all" || function == "uvc")
        uvc.power(status);
}

void funcControl::countState(String function, bool status) {
    if (function == "all")
        all.count(status);
    if (function == "pur")
        pur.count(status);
    if (function == "fog")
        fog.count(status);
    if (function == "uvc")
        uvc.count(status);
}

void funcControl::timeSet(String function, uint32_t time) {
    if (function == "all")
        all.setTime(time);
    if (function == "pur")
        pur.setTime(time);
    if (function == "fog")
        fog.setTime(time);
    if (function == "uvc")
        uvc.setTime(time);
}

void funcControl::changePurMode(String mode) {
    if (mode == "modeAuto")
        pur.setMode(AutoMode);
    if (mode == "modeSleep")
        pur.setMode(SleepMode);
    if (mode == "modeManual")
        pur.setMode(ManualMode);
}

void funcControl::changePurSpeed(uint8_t speed) {
    pur.setDuty(speed * 2.55);
}

void funcControl::devMode(String function, String set, bool state) {
    if (function == "fog") {
        fog.increment(set, state);
    } else if (function == "uvc") {
        uvc.increment(set, state);
    }
}

funcControl &funcControl::setMCUcommender(MCU_COMMENDER_SIGNATURE) {
    this->MCUcommender = MCUcommender;
    return *this;
};

void funcControl::refresh(String target) {
    MCUcommender(target);
}

void funcControl::saveToJson() {
    StaticJsonDocument<512> j_initial;

    j_initial["all"] = all.getVariable();
    j_initial["pur"] = pur.getVariable();
    j_initial["fog"] = fog.getVariable();
    j_initial["uvc"] = uvc.getVariable();
    String initialStr;
    serializeJson(j_initial, initialStr);
    writeFile2(LittleFS, "/initial/initial.txt", initialStr.c_str());
    logger.i("saveToJson Complete!");
}

/*void funcControl::saveToJson() {
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
}*/

void funcControl::initialJson() {
    String readinitialStr = readFile(LittleFS, "/initial/initial.txt");
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, readinitialStr);

    if (error) {
        logger.e("deserialize Function Json() failed: %s", error.c_str());
        return;
    }

    all.setVariable(doc["all"]);
    pur.setVariable(doc["pur"]);
    fog.setVariable(doc["fog"]);
    uvc.setVariable(doc["uvc"]);

    logger.i("load From Json Complete!");
}

/*void funcControl::initialJson() {

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
}*/

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

void funcControl::MachineReset() {
    deleteFile(LittleFS, "/initial/initial.txt");
    deleteFile(LittleFS, "/initial/topic.txt");
    deleteFile(LittleFS, "/wifi/wifi.txt");
    ESP.restart();
}

bool funcControl::MachineState() {
    return all.getState() || pur.getState() || fog.getState() || uvc.getState();
}

bool funcControl::MachineCountStart() {
    return all.countStart() || pur.countStart() || fog.countStart() || uvc.countStart();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
