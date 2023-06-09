#ifndef __FUNCCONTROL_
#define __FUNCCONTROL_
#include "littlefsfun.h"
#include "loggerESP.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32Time.h>
#include <function.h>

#define ON true
#define OFF false

#define MCU_COMMENDER_SIGNATURE std::function<void(String)> MCUcommender

class funcControl {
private:
    MCU_COMMENDER_SIGNATURE;

    void funcState(String, bool);
    void countState(String, bool);
    void timeSet(String, uint32_t);
    void changePurMode(String);
    void changePurSpeed(uint8_t);

    void devMode(String, String, bool);
    void systemCommend(String, String);

public:
    Function all;
    Purifier pur;
    FogMachine fog;
    UvcLamp uvc;

    funcControl();
    void commend(String, String = "", String = "");

    void saveToJson();
    void refresh(String target = "");
    void MachineInitial();
    void MachineReset();
    bool MachineState();
    bool MachineCountStart();
    bool MachineAction();

    String getInitialJson();
    funcControl &setMCUcommender(MCU_COMMENDER_SIGNATURE);
};

#endif