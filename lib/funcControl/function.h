#ifndef __FUNCTION__
#define __FUNCTION__
#include "loggerESP.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32Time.h>

typedef enum {
    autoMode = 0,
    sleepMode,
    manualMode
} MODE;

#define AutoMode autoMode
#define SleepMode sleepMode
#define ManualMode manualMode

extern ESP32Time rtc;

class Function {
private:
protected:
    String name;
    bool state = false;
    bool countState = false;
    bool startingCount = false;
    uint32_t time = 1800;
    int32_t countTime;
    int32_t endTime;

public:
    Function(String);
    virtual void power(bool);
    virtual void setVariable(JsonVariant);
    virtual JsonVariant getVariable();
    void count(bool);
    void setTime(uint32_t);
    bool countStart();
    bool getState();
    int32_t getCountTime();
};

//////////////////////////////////////////////////
//////////////////////////////////////////////////

class Purifier : public Function {

private:
    const uint8_t purifier = 33;
    const uint8_t PWMpin = 4;
    const uint8_t PWMchannel = 1;
    const uint8_t resolution = 8;

    uint16_t freq = 3000;
    uint8_t dutycycle = 30;
    uint16_t dustVal = 0;
    uint8_t manualDutycycle = 30;
    MODE modeState = autoMode;

    static SemaphoreHandle_t purPowerControlMutex;
    static void purPowerControl(void *pvParam);

public:
    Purifier(String);
    void power(bool) override;
    void setMode(MODE);
    void setDust(uint16_t);
    void setDuty(uint8_t);
    void setVariable(JsonVariant) override;
    JsonVariant getVariable() override;
};

//////////////////////////////////////////////////
//////////////////////////////////////////////////

class FogMachine : public Function {
private:
    const uint8_t fogpump = 12;
    const uint8_t fogfan = 14;
    const uint8_t fogMachine = 32;

    static SemaphoreHandle_t fogPowerControlMutex;
    static void fogPowerControl(void *pvParam);

public:
    FogMachine(String);
    void power(bool) override;
    void increment(String, bool);
};

//////////////////////////////////////////////////
//////////////////////////////////////////////////

class UvcLamp : public Function {
private:
    const uint8_t ecin = 26;
    const uint8_t ecout = 27;
    const uint8_t uvLamp = 22;

    static SemaphoreHandle_t uvcPowerControlMutex;
    static void uvcPowerControl(void *pvParam);

public:
    UvcLamp(String);
    void power(bool) override;
    void increment(String, bool);
};

#endif