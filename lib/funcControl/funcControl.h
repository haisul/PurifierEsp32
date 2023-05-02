#ifndef __FUNCCONTROL_
#define __FUNCCONTROL_
#include <Arduino.h>
#include <ESP32Time.h>

typedef enum {
    func_mode_pur = 0,
    func_mode_fog,
    func_mode_uvc,
    func_mode_all,
    modeAuto,
    modeSleep,
    modeManual,
    mysystem,
    Failed
} function_mode_types;

#define PUR func_mode_pur
#define FOG func_mode_fog
#define UVC func_mode_uvc
#define ALL func_mode_all
#define AUTO modeAuto
#define SLEEP modeSleep
#define MANUAL modeManual
#define SYSTEM mysystem

#define ON true
#define OFF false

#define COUNT_ON true
#define COUNT_OFF false

/*#define FUNC_CALLBACK_SIGNATURE std::function<void(String, bool)> func_callback*/
#define MCU_COMMENDER_SIGNATURE std::function<void(String)> MCUcommender

class funcControl {
private:
        const byte fogpump = 12;
    const byte fogfan = 14;
    const byte fogMachine = 32;
    /// pin25、26、27改為pin26、27輸出訊號至L298N ///
    // byte ecin = 25;
    // byte ecout = 26;
    // byte ecswitch = 27;
    const byte ecin = 26;
    const byte ecout = 27;
    const byte uvLamp = 22;
    const byte purifier = 33;
    const byte PWMpin = 4;
    const byte PWMchannel = 1;
    const byte resolution = 8;
    short _freq = 3000;
    short _dutycycle = 30;
    short _dust = 0;

    /*FUNC_CALLBACK_SIGNATURE;*/
    MCU_COMMENDER_SIGNATURE;
    function_mode_types nowPurMode = AUTO;
    byte manualDutycycle = 30;

    typedef struct function_status {
        String name;
        bool state = false;
        bool countState = false;
        bool startingCount = false;
        int time = 1800;
        unsigned int countTime = 1800;
        int endEpoch;

    } status;

public:
    ESP32Time rtc;
    int epoch = rtc.getEpoch();
    String SSID = "";
    String PASSWORD = "";
    bool _modeAuto = true, _modeSleep = false, _modeManual = false;

    funcControl();

    void purMode(function_mode_types);
    void getDustVal(int dustVal);
    void getDutycycle(int dutycyclePer);
    void getSetTime(int);
    void mapDutycycle(function_mode_types);

    void funcState(function_mode_types, bool, bool publish = false);
    void countSet(function_mode_types, bool);
    void timeSet(function_mode_types, int);
    void startCountTime(function_mode_types);

    void saveToJson();
    void initialJson();
    void refresh(String target = "");
    void MachineInitial();
    void wifiSaveToJson(String, String, String, String);
    void initialWifi();
    String getWifiInfo();

    void devMode(String, bool);
    String getInitialJson();
    funcControl &setMCUcommender(MCU_COMMENDER_SIGNATURE);

    void commend(String, String set = "", String stete = "");
    void systemCommend(String, String);
    void reset();

    status all, pur, fog, uvc;
};

function_mode_types convert(String);

#endif