#include "Pairing.h"
#include "QueueMqttClient.h"
#include <exception>

Pairing::Pairing(QueueMQTTClient *client, String serialNum, String topic, uint16_t time) : mqttClient(client), serialNum(serialNum), pairingTopic(topic), pairingTime(time) {
}

Pairing::~Pairing() {
    if (pairingTimerHandle != NULL) {
        vTaskDelete(pairingTimerHandle);
        logger.wL("pairingTimer() is deleted!");
    }
    logger.wL("Pairing is deleted!");
}

void Pairing::test() {
    try {
        mqttClient->subscribe("000", 0);
        mqttClient->subscribe("111", 1);
        mqttClient->subscribe("222", 2);
    } catch (const std::exception &e) {
        Serial.println("Exception occurred: " + String(e.what()));
    }
}

bool Pairing::start() {
    Pairing *params = this;
    xTaskCreatePinnedToCore(pairingTimer, "pairingTimer", 4096, (void *)params, 1, &pairingTimerHandle, 1);
    mqttClient->subscribe(pairingTopic, 2);
    mqttClient->sendMessage(pairingTopic, serialNum, 2);
    return true;
}

bool Pairing::recievePairingMessage(String userId) {
    if (!initLittleFS()) {
        logger.eL("initial function data Failed!");
        return false;
    }
    logger.iL("onPairing");
    serialNum.replace("serialNum:", "");
    userId.replace("userID:", "");
    StaticJsonDocument<512> j_topic;
    j_topic["topicApp"] = userId + "/" + serialNum + "/app";
    j_topic["topicEsp"] = userId + "/" + serialNum + "/esp";
    j_topic["topicPms"] = userId + "/" + serialNum + "/pms";
    j_topic["topicTimer"] = userId + "/" + serialNum + "/timer";
    j_topic["topicWifi"] = userId + "/" + serialNum + "/wifi";

    serializeJson(j_topic, topicStr);
    vTaskDelay(500);
    return true;
}

void Pairing::pairingTimer(void *pvParam) {
    Pairing *instance = static_cast<Pairing *>(pvParam);
    while (1) {
        instance->counter++;
        Serial.println(instance->counter);
        if (instance->pairingSuccess) {
            logger.iL("paire Success");
            instance->result(instance->topicStr);
            break;
        }
        if (instance->counter > instance->pairingTime) {
            logger.eL("paire faild");
            break;
        }
        vTaskDelay(1000);
    }
    vTaskDelete(NULL);
}

void Pairing::pairingMassage(String &topic, String &payload) {
    if (!onPairing && topic == pairingTopic && payload.startsWith("userID:")) {
        onPairing = true;
        String mqttMsg = payload;
        counter = 0;
        pairingSuccess = recievePairingMessage(mqttMsg);
    }
}

Pairing &Pairing::resultCallback(RESULT_SIGNATURE) {
    this->result = result;
    return *this;
};
