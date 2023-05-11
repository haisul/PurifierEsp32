#include "function.h"

Function::Function() {}

void Function::power(bool status) {
    state = status;
    if (state && countState) {
        countStart();
    }
}

void Function::count(bool status) {
    countState = status;
    if (state && countState) {
        countStart();
    }
}

void Function::setTime(uint32_t time) {
    this->time = time;
}

void Function::countStart() {
    endEpoch = rtc.getEpoch() + time;
}

///////////////////////////////////////////////////////////////////////

Purifier::Purifier() : Function() {
    pinMode(purifier, OUTPUT); // Relay2-1 purifier Fan
    pinMode(PWMpin, OUTPUT);   // purifier fan PWMpin
    digitalWrite(purifier, LOW);
    ledcAttachPin(PWMpin, PWMchannel);
    ledcSetup(PWMchannel, freq, resolution);
}

void Purifier::power(bool status) {
    if (status) {
        vTaskDelay(500);
        digitalWrite(purifier, HIGH);
    } else {
        vTaskDelay(500);
        digitalWrite(purifier, LOW);
    }
    state = status;
    if (state && countState) {
        countStart();
    }
}

void Purifier::changeMode(MODE mode) {
    modeState = mode;
    switch (mode) {
    case autoMode:
        dutycycle = 255 / 4 + (255 * 3 / 4) * dustVal / 500;
        break;

    case sleepMode:
        dutycycle = 40;
        break;

    case manualMode:
        dutycycle = manualDutycycle * 2.55;
        break;
    }

    if (dutycycle > 225)
        dutycycle = 225;
    else if (dutycycle < 40)
        dutycycle = 40;
    ledcWrite(PWMchannel, dutycycle);
}

void Purifier::getDust(uint16_t val) {
    dustVal = val;
    changeMode(autoMode);
}

FogMachine::FogMachine() : Function() {
    pinMode(fogpump, OUTPUT);    // Relay1-8 fog Pump
    pinMode(fogfan, OUTPUT);     // Relay1-7 fog Fan
    pinMode(fogMachine, OUTPUT); // Relay2-2 fog Machine
}

void FogMachine::power(bool status) {
    if (status) {
        digitalWrite(fogMachine, HIGH);
        vTaskDelay(500);
        digitalWrite(fogpump, HIGH);
        digitalWrite(fogfan, HIGH);
    } else {
        digitalWrite(fogMachine, LOW);
        vTaskDelay(2000);
        digitalWrite(fogpump, LOW);
        digitalWrite(fogfan, LOW);
    }
    state = status;
    if (state && countState) {
        countStart();
    }
}

UvcLamp::UvcLamp() : Function() {
    pinMode(ecin, OUTPUT);   // L298NA-IN1 ec Direction
    pinMode(ecout, OUTPUT);  // L298NA-IN2 ec Direction
    pinMode(uvLamp, OUTPUT); // Relay1-1 uv Lamp
}

void UvcLamp::power(bool status) {
    if (status) {
        digitalWrite(ecin, LOW);
        digitalWrite(ecout, LOW);
        digitalWrite(uvLamp, HIGH);
        digitalWrite(ecout, HIGH);
        vTaskDelay(6000);
        digitalWrite(ecout, LOW);
    } else {
        digitalWrite(ecin, LOW);
        digitalWrite(ecout, LOW);
        digitalWrite(uvLamp, LOW);
        digitalWrite(ecin, HIGH);
        vTaskDelay(6000);
        digitalWrite(ecin, LOW);
    }
    state = status;
    if (state && countState) {
        countStart();
    }
}

///////////////////////////////////////////////////////////////////////

/*
class A {
private:
    String a;

protected:
    int i = 0;
    int j = 0;
    int k = 0;

public:
    A();
    virtual void fun();
    void fun2();
};

class B : public A {
    private:
    int l=0;
    int m=0;
    int n=0;
    public:
    B():A(){};
    void fun()override{};
    void test();

}*/
