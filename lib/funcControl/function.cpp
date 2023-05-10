#include "function.h"

Purifier::Purifier() {
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

void Purifier::count(bool status) {
    countState = status;
    if (state && countState) {
        countStart();
    }
}

void Purifier::setTime(uint32_t time) {
    this->time = time;
}

void Purifier::countStart() {
    endEpoch = rtc.getEpoch() + time;
}