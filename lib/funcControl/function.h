#ifndef __FUNCTION__
#define __FUNCTION__
#include <Arduino.h>
#include <ESP32Time.h>

class Purifier {
private:
    ESP32Time rtc;

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

    String name;
    bool state = false;
    bool countState = false;
    bool startingCount = false;
    uint32_t time = 1800;
    // uint32_t countTime = 1800;
    uint32_t endEpoch;
    uint32_t epoch = rtc.getEpoch();
    MODE modeState = autoMode;

    void countStart();

public:
    Purifier();
    void power(bool);
    void count(bool);
    void setTime(uint32_t);

    void changeMode(MODE);
    void getDust(uint16_t);
};

class FogMachine {
};

class UvcLamp {
};

#endif