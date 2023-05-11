#ifndef __FUNCTION__
#define __FUNCTION__
#include <Arduino.h>
#include <ESP32Time.h>

class Function {
private:
    ESP32Time rtc;

protected:
    String name;
    bool state = false;
    bool countState = false;
    bool startingCount = false;
    uint32_t time = 1800;
    uint32_t endEpoch;
    uint32_t epoch = rtc.getEpoch();

    void countStart();

public:
    Function();
    virtual void power(bool);
    void count(bool);
    void setTime(uint32_t);
};

class Purifier : public Function {
private:
    typedef enum {
        autoMode = 0,
        sleepMode,
        manualMode
    } MODE;

    const uint8_t purifier = 33;
    const uint8_t PWMpin = 4;
    const uint8_t PWMchannel = 1;
    const uint8_t resolution = 8;

    uint16_t freq = 3000;
    uint8_t dutycycle = 30;
    uint16_t dustVal = 0;
    uint8_t manualDutycycle = 30;

    MODE modeState = autoMode;

public:
    Purifier();
    void power(bool) override;
    void changeMode(MODE);
    void getDust(uint16_t);
};

class FogMachine : public Function {
private:
    const uint8_t fogpump = 12;
    const uint8_t fogfan = 14;
    const uint8_t fogMachine = 32;

public:
    FogMachine();
    void power(bool) override;
};

class UvcLamp : public Function {
private:
    const uint8_t ecin = 26;
    const uint8_t ecout = 27;
    const uint8_t uvLamp = 22;

public:
    UvcLamp();
    void power(bool) override;
};

#endif