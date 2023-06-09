#include "function.h"
ESP32Time rtc;

SemaphoreHandle_t Purifier::purPowerControlMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t FogMachine::fogPowerControlMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t UvcLamp::uvcPowerControlMutex = xSemaphoreCreateMutex();

Function::Function(String name) {
    this->name = name;
}

void Function::power(bool status) {
    state = status;
    if (state && countState) {
        endTime = rtc.getEpoch() + time;
        startingCount = true;
        logger.iL("%s startingCount", name.c_str());
    } else
        startingCount = false;
    logger.iL("%s:%s", name.c_str(), state ? "ON" : "OFF");
}

void Function::count(bool status) {
    countState = status;
    if (state && countState) {
        endTime = rtc.getEpoch() + time;
        startingCount = true;
        logger.iL("%s startingCount", name.c_str());
    } else
        startingCount = false;
    logger.iL("%s COUNT:%s", name.c_str(), countState ? "ON" : "OFF");
}

void Function::setTime(uint32_t time) {
    this->time = time;
    this->countTime = time;
    endTime = rtc.getEpoch() + time;
    logger.iL("%s TIME:%d", name.c_str(), time);
}

bool Function::countStart() { return startingCount; }

JsonVariant Function::getVariable() {
    j_initial.clear();
    j_initial["state"] = state;
    j_initial["countState"] = countState;
    j_initial["time"] = time;

    return j_initial.as<JsonVariant>();
}

void Function::setVariable(JsonVariant j_initial) {
    countState = j_initial["countState"];
    time = j_initial["time"];
}

bool Function::getState() { return state; }

bool Function::getCountState() { return countState; }

int32_t Function::getTime() { return time; }

int32_t Function::getCountTime() {
    if (startingCount) {
        countTime = endTime - rtc.getEpoch();
        if (countTime < 0)
            startingCount = false;
    }
    return countTime;
}

bool Function::inAction() { return isInAction; }

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

Purifier::Purifier(String name) : Function(name) {
    pinMode(purifier, OUTPUT); // Relay2-1 purifier Fan
    pinMode(PWMpin, OUTPUT);   // purifier fan PWMpin
    digitalWrite(purifier, LOW);
    ledcAttachPin(PWMpin, PWMchannel);
    ledcSetup(PWMchannel, freq, resolution);

    purPowerControlMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(purPowerControlMutex);
}

void Purifier::power(bool status) {
    if (xSemaphoreTake(purPowerControlMutex, (TickType_t)10) == pdTRUE) {
        state = status;
        setMode(modeState);
        if (state && countState) {
            endTime = rtc.getEpoch() + time;
            startingCount = true;
        } else
            startingCount = false;
        Purifier *params = this;
        xTaskCreatePinnedToCore(purPowerControl, "purPowerControl", 2048, (void *)params, 1, &taskHandle, 0);
        // UBaseType_t stackUsage = uxTaskGetStackHighWaterMark(taskHandle);
        // size_t stackUsageBytes = stackUsage * sizeof(StackType_t);
        // Serial.printf("Task stack usage: %u bytes\n", stackUsageBytes);
    }
}

void Purifier::purPowerControl(void *pvParam) {
    Purifier *instance = static_cast<Purifier *>(pvParam);
    instance->isInAction = true;
    if (instance->state) {
        vTaskDelay(500);
        digitalWrite(instance->purifier, HIGH);
    } else {
        vTaskDelay(500);
        digitalWrite(instance->purifier, LOW);
    }
    logger.iL("PUR:%s", instance->state ? "ON" : "OFF");
    instance->isInAction = false;
    xSemaphoreGive(instance->purPowerControlMutex);
    vTaskDelete(NULL);
}

void Purifier::setMode(MODE mode) {
    modeState = mode;
    switch (mode) {
    case autoMode:
        dutycycle = 255 / 4 + (255 * 3 / 4) * dustVal / 1000;
        logger.iL("%s MODE:AUTO DUTY:%d", name.c_str(), dutycycle);
        break;

    case sleepMode:
        dutycycle = 50;
        logger.iL("%s MODE:SLEEP DUTY:%d", name.c_str(), dutycycle);
        break;

    case manualMode:
        dutycycle = manualDutycycle;
        logger.iL("%s MODE:MANUAL DUTY:%d", name.c_str(), dutycycle);
        break;
    }

    if (dutycycle > 225)
        dutycycle = 225;
    else if (dutycycle < 50)
        dutycycle = 50;
    ledcWrite(PWMchannel, dutycycle);
}

void Purifier::setDust(uint16_t val) {
    dustVal = val;
    if (modeState == AutoMode)
        setMode(AutoMode);
}

void Purifier::setDuty(uint8_t val) {
    speed = val;
    manualDutycycle = val * 2.55;
    if (modeState == ManualMode)
        setMode(ManualMode);
}

JsonVariant Purifier::getVariable() {
    j_initial.clear();
    j_initial["state"] = state;
    j_initial["countState"] = countState;
    j_initial["time"] = time;
    j_initial["mode"] = modeState;
    j_initial["speed"] = speed;
    return j_initial.as<JsonVariant>();
}

void Purifier::setVariable(JsonVariant j_initial) {
    countState = j_initial["countState"];
    time = j_initial["time"];
    modeState = j_initial["mode"];
}

void Purifier::maxPur() {
    ledcWrite(PWMchannel, 225);
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

FogMachine::FogMachine(String name) : Function(name) {
    pinMode(fogpump, OUTPUT);    // Relay1-8 fog Pump
    pinMode(fogfan, OUTPUT);     // Relay1-7 fog Fan
    pinMode(fogMachine, OUTPUT); // Relay2-2 fog Machine

    fogPowerControlMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(fogPowerControlMutex);
}

void FogMachine::power(bool status) {
    if (xSemaphoreTake(fogPowerControlMutex, (TickType_t)10) == pdTRUE) {
        state = status;
        if (state && countState) {
            endTime = rtc.getEpoch() + time;
            startingCount = true;
        } else
            startingCount = false;
        FogMachine *params = this;
        xTaskCreatePinnedToCore(fogPowerControl, "fogPowerControl", 2048, (void *)params, 1, NULL, 0);
    }
}

void FogMachine::fogPowerControl(void *pvParam) {
    FogMachine *instance = static_cast<FogMachine *>(pvParam);
    instance->isInAction = true;
    if (instance->state) {
        digitalWrite(instance->fogMachine, HIGH);
        vTaskDelay(500);
        digitalWrite(instance->fogpump, HIGH);
        digitalWrite(instance->fogfan, HIGH);
    } else {
        digitalWrite(instance->fogMachine, LOW);
        digitalWrite(instance->fogpump, LOW);
        vTaskDelay(2000);
        digitalWrite(instance->fogfan, LOW);
    }
    logger.iL("FOG:%s", instance->state ? "ON" : "OFF");
    instance->isInAction = false;
    xSemaphoreGive(instance->fogPowerControlMutex);
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

    uvcPowerControlMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(uvcPowerControlMutex);
}

void UvcLamp::power(bool status) {
    if (xSemaphoreTake(uvcPowerControlMutex, (TickType_t)10) == pdTRUE) {
        state = status;
        if (state && countState) {
            endTime = rtc.getEpoch() + time;
            startingCount = true;
        } else
            startingCount = false;
        UvcLamp *params = this;
        xTaskCreatePinnedToCore(uvcPowerControl, "uvcPowerControl", 2048, (void *)params, 1, NULL, 0);
    }
}

void UvcLamp::uvcPowerControl(void *pvParam) {
    UvcLamp *instance = static_cast<UvcLamp *>(pvParam);
    instance->isInAction = true;
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
    logger.iL("UVC:%s", instance->state ? "ON" : "OFF");
    instance->isInAction = false;
    xSemaphoreGive(instance->uvcPowerControlMutex);
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
