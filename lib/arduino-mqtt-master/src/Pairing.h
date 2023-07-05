#ifndef PAIRING_H
#define PAIRING_H

#include "loggerESP.h"
#include <ArduinoJson.h>

class QueueMQTTClient;

#define RESULT_SIGNATURE std::function<void(String)> result

class Pairing {
private:
    RESULT_SIGNATURE;

    QueueMQTTClient *mqttClient;
    String serialNum;
    String pairingTopic;
    String userId;

    uint16_t pairingTime;
    uint16_t counter = 0;

    String topicStr;

    bool onPairing = false;
    bool pairingSuccess = false;

    TaskHandle_t pairingTimerHandle;
    static void pairingTimer(void *pvParam);

public:
    Pairing(QueueMQTTClient *, String, String, uint16_t);
    ~Pairing();
    void test();
    bool start();
    bool recievePairingMessage(String);
    void pairingMassage(String &topic, String &payload);

    Pairing &resultCallback(RESULT_SIGNATURE);
};

#endif