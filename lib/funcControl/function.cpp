#include "function.h"
ESP32Time rtc;

Function::Function(String name) {
    this->name = name;
}

void Function::power(bool status) {
    state = status;
    if (state && countState) {
        endTime = rtc.getEpoch() + time;
        startingCount = true;
        logger.i("%s startingCount", name.c_str());
    } else {
        startingCount = false;
    }
    logger.i("%s:%s", name.c_str(), state ? "ON" : "OFF");
}

void Function::count(bool status) {
    countState = status;
    if (state && countState) {
        endTime = rtc.getEpoch() + time;
        startingCount = true;
        logger.i("%s startingCount", name.c_str());
    } else {
        startingCount = false;
    }
    logger.i("%s COUNT:%s", name.c_str(), state ? "ON" : "OFF");
}

void Function::setTime(uint32_t time) {
    this->time = time;
    logger.i("%s TIME:%d", name.c_str(), time);
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

    countState = doc["countState"];
    time = doc["time"];
}

bool Function::getState() { return state; }

uint16_t Function::getEndTime() { return endTime; }

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

Purifier::Purifier(String name) : Function(name) {
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
    xTaskCreatePinnedToCore(purPowerControl, "purPowerControl", 2048, (void *)params, 1, NULL, 0);
    logger.i("%s:%s", name.c_str(), state ? "ON" : "OFF");
}

void Purifier::purPowerControl(void *pvParam) {
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
        logger.i("%s MODE:AUTO", name.c_str());
        break;

    case sleepMode:
        dutycycle = 40;
        logger.i("%s MODE:SLEEP", name.c_str());
        break;

    case manualMode:
        dutycycle = manualDutycycle * 2.55;
        logger.i("%s MODE:MANUAL", name.c_str());
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
    if (modeState == autoMode)
        setMode(AutoMode);
}

void Purifier::setDuty(uint16_t val) {
    manualDutycycle = val;
    setMode(ManualMode);
    logger.i("%s DUTY:%s", name.c_str(), val);
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

FogMachine::FogMachine(String name) : Function(name) {
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
    xTaskCreatePinnedToCore(fogPowerControl, "fogPowerControl", 2048, (void *)params, 1, NULL, 0);
    logger.i("%s:%s", name.c_str(), state ? "ON" : "OFF");
}

void FogMachine::fogPowerControl(void *pvParam) {
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

UvcLamp::UvcLamp(String name) : Function(name) {
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
    xTaskCreatePinnedToCore(uvcPowerControl, "uvcPowerControl", 2048, (void *)params, 1, NULL, 0);
    logger.i("%s:%s", name.c_str(), state ? "ON" : "OFF");
}

void UvcLamp::uvcPowerControl(void *pvParam) {
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
