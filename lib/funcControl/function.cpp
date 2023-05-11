#include "function.h"
ESP32Time rtc;

Function::Function() {}

void Function::power(bool status) {
    state = status;
    if (state && countState) {
        endTime = rtc.getEpoch() + time;
        startingCount = true;
    }
}

void Function::count(bool status) {
    countState = status;
    if (state && countState) {
        endTime = rtc.getEpoch() + time;
        startingCount = true;
    }
}

void Function::setTime(uint32_t time) {
    this->time = time;
}

bool Function::countStart() { return startingCount; }

JsonVariant Function::getVariable() {
    StaticJsonDocument<128> j_initial;
    j_initial["state"] = state;
    j_initial["countState"] = countState;
    j_initial["time"] = time;

    return j_initial;
}

void Function::setVariable(JsonVariant j_initial) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, j_initial);

    if (error)
        return;

    // state = doc["state"];
    countState = doc["countState"];
    time = doc["time"];
}

bool Function::getState() { return state; }

uint16_t Function::getEndTime() { return endTime; }

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

Purifier::Purifier() : Function() {
    pinMode(purifier, OUTPUT); // Relay2-1 purifier Fan
    pinMode(PWMpin, OUTPUT);   // purifier fan PWMpin
    digitalWrite(purifier, LOW);
    ledcAttachPin(PWMpin, PWMchannel);
    ledcSetup(PWMchannel, freq, resolution);
}

void Purifier::power(bool status) {
    state = status;
    if (state && countState) {
        endTime = rtc.getEpoch() + time;
        startingCount = true;
    }
    Purifier *params = this;
    xTaskCreatePinnedToCore(powerControl, "powerControl", 2048, (void *)params, 1, NULL, 0);
}

void Purifier::powerControl(void *pvParam) {
    Purifier *instance = static_cast<Purifier *>(pvParam);
    if (instance->state) {
        vTaskDelay(500);
        digitalWrite(instance->purifier, HIGH);
    } else {
        vTaskDelay(500);
        digitalWrite(instance->purifier, LOW);
    }
    vTaskDelete(NULL);
}

void Purifier::setMode(MODE mode) {
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

void Purifier::setDust(uint16_t val) {
    dustVal = val;
    setMode(AutoMode);
}

void Purifier::setDuty(uint16_t val) {
    manualDutycycle = val;
    setMode(ManualMode);
}

JsonVariant Purifier::getVariable() {
    StaticJsonDocument<128> j_initial;
    j_initial["state"] = state;
    j_initial["countState"] = countState;
    j_initial["time"] = time;
    j_initial["mode"] = modeState;

    return j_initial;
}

void Purifier::setVariable(JsonVariant j_initial) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, j_initial);

    if (error)
        return;

    // state = doc["state"];
    countState = doc["countState"];
    time = doc["time"];
    modeState = doc["mode"];
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

FogMachine::FogMachine() : Function() {
    pinMode(fogpump, OUTPUT);    // Relay1-8 fog Pump
    pinMode(fogfan, OUTPUT);     // Relay1-7 fog Fan
    pinMode(fogMachine, OUTPUT); // Relay2-2 fog Machine
}

void FogMachine::power(bool status) {
    state = status;
    if (state && countState) {
        endTime = rtc.getEpoch() + time;
        startingCount = true;
    }
    FogMachine *params = this;
    xTaskCreatePinnedToCore(powerControl, "powerControl", 2048, (void *)params, 1, NULL, 0);
}

void FogMachine::powerControl(void *pvParam) {
    FogMachine *instance = static_cast<FogMachine *>(pvParam);
    if (instance->state) {
        digitalWrite(instance->fogMachine, HIGH);
        vTaskDelay(500);
        digitalWrite(instance->fogpump, HIGH);
        digitalWrite(instance->fogfan, HIGH);
    } else {
        digitalWrite(instance->fogMachine, LOW);
        vTaskDelay(2000);
        digitalWrite(instance->fogpump, LOW);
        digitalWrite(instance->fogfan, LOW);
    }
    vTaskDelete(NULL);
}

void FogMachine::increment(String set, bool status) {
    if (set = "fogMachine") {
        digitalWrite(fogMachine, status);
    } else if (set = "fogpump") {
        digitalWrite(fogpump, status);
    } else if (set = "fogfan") {
        digitalWrite(fogfan, status);
    }
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

UvcLamp::UvcLamp() : Function() {
    pinMode(ecin, OUTPUT);   // L298NA-IN1 ec Direction
    pinMode(ecout, OUTPUT);  // L298NA-IN2 ec Direction
    pinMode(uvLamp, OUTPUT); // Relay1-1 uv Lamp
}

void UvcLamp::power(bool status) {
    state = status;
    if (state && countState) {
        endTime = rtc.getEpoch() + time;
        startingCount = true;
    }
    UvcLamp *params = this;
    xTaskCreatePinnedToCore(powerControl, "powerControl", 2048, (void *)params, 1, NULL, 0);
}

void UvcLamp::powerControl(void *pvParam) {
    UvcLamp *instance = static_cast<UvcLamp *>(pvParam);
    if (instance->state) {
        digitalWrite(instance->ecin, LOW);
        digitalWrite(instance->ecout, LOW);
        digitalWrite(instance->uvLamp, HIGH);
        digitalWrite(instance->ecout, HIGH);
        vTaskDelay(6000);
        digitalWrite(instance->ecout, LOW);
    } else {
        digitalWrite(instance->ecin, LOW);
        digitalWrite(instance->ecout, LOW);
        digitalWrite(instance->uvLamp, LOW);
        digitalWrite(instance->ecin, HIGH);
        vTaskDelay(6000);
        digitalWrite(instance->ecin, LOW);
    }
    vTaskDelete(NULL);
}

void UvcLamp::increment(String set, bool status) {
    if (set = "ec") {
        if (status) {
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
    } else if (set = "light") {
        digitalWrite(uvLamp, status);
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
