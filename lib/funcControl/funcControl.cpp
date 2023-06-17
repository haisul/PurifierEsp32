#include "funcControl.h"

funcControl::funcControl() : all("ALL"), pur("PUR"), fog("FOG"), uvc("UVC") {
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
}

void funcControl::funcState(String function, bool status) {

    if (function == "all" || function == "pur" && status != pur.getState())
        pur.power(status);
    if (function == "all" || function == "fog" && status != fog.getState())
        fog.power(status);
    if (function == "all" || function == "uvc" && status != uvc.getState())
        uvc.power(status);
    if (function == "all" && status != all.getState()) {
        all.power(status);
        pur.maxPur();
    }
}

void funcControl::countState(String function, bool status) {
    if (function == "all" && status != all.getCountState())
        all.count(status);
    if (function == "pur" && status != pur.getCountState())
        pur.count(status);
    if (function == "fog" && status != fog.getCountState())
        fog.count(status);
    if (function == "uvc" && status != uvc.getCountState())
        uvc.count(status);
}

void funcControl::timeSet(String function, uint32_t time) {
    if (function == "all" && time != all.getTime())
        all.setTime(time);
    if (function == "pur" && time != pur.getTime())
        pur.setTime(time);
    if (function == "fog" && time != fog.getTime())
        fog.setTime(time);
    if (function == "uvc" && time != uvc.getTime())
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
    pur.setDuty(speed);
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
    logger.iL("Save function data Success");
}

void funcControl::MachineInitial() {
    if (!initLittleFS()) {
        logger.eL("initial function data Failed!");
        return;
    }

    String readinitialStr = readFile(LittleFS, "/initial/initial.txt");
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, readinitialStr);

    if (error) {
        logger.eL("deserialize Function data failed: %s", error.c_str());
        return;
    }

    all.setVariable(doc["all"]);
    pur.setVariable(doc["pur"]);
    fog.setVariable(doc["fog"]);
    uvc.setVariable(doc["uvc"]);

    logger.iL("Load function data Complete!");
}

String funcControl::getInitialJson() {
    String readinitialStr = readFile(LittleFS, "/initial/initial.txt");
    return readinitialStr;
}

void funcControl::MachineReset() {
    deleteFile(LittleFS, "/initial/initial.txt");
    deleteFile(LittleFS, "/topic/topic.txt");
    deleteFile(LittleFS, "/wifi/wifi.txt");
    ESP.restart();
}

bool funcControl::MachineState() {
    return all.getState() || pur.getState() || fog.getState() || uvc.getState();
}

bool funcControl::MachineCountStart() {
    return all.countStart() || pur.countStart() || fog.countStart() || uvc.countStart();
}

bool funcControl::MachineAction() {
    return all.inAction() || pur.inAction() || fog.inAction() || uvc.inAction();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
