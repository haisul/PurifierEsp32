#ifndef PAIRING_H
#define PAIRING_H

#include "littlefsfun.h"
#include "loggerESP.h"
#include <ArduinoJson.h>
// #include "QueueMqttClient.h"
class QueueMQTTClient;

#define RESULT_SIGNATURE std::function<void(bool)> result

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

    TaskHandle_t taskHandle;
    static void taskFunction(void *pvParam);
    void pairingTimer();

public:
    Pairing(QueueMQTTClient *, String, String, uint16_t);
    ~Pairing();

    bool start();
    bool recievePairingMessage(String);
    void pairingMassage(String &topic, String &payload);
    String getTopic();

    Pairing &resultCallback(RESULT_SIGNATURE);
};

#endif