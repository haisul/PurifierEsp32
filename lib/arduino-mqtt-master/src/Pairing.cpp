#include "Pairing.h"
#include "QueueMqttClient.h"

Pairing::Pairing(QueueMQTTClient *client, String serialNum, String topic, uint16_t time) : mqttClient(client), serialNum(serialNum), pairingTopic(topic), pairingTime(time) {
}

Pairing::~Pairing() {
    if (taskHandle != NULL) {
        vTaskDelete(taskHandle);
        logger.wL("pairingTimer() is deleted!");
    }
    logger.wL("Pairing is deleted!");
}

bool Pairing::start() {
    Pairing *params = this;
    xTaskCreatePinnedToCore(taskFunction, "pairingTimer", 2048, (void *)params, 1, &taskHandle, 1);
    // FIXME:這裡會斷線
    mqttClient->subscribe(pairingTopic, 2);
    // mqttClient->sendMessage(pairingTopic, serialNum, 2);
    mqttClient->pairingMsg(pairingTopic, serialNum);
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

    /*writeFile2(LittleFS, "/topic/topic.txt", topicStr.c_str());
    logger.iL("Topic data is saved");
    String readTopicStr = readFile(LittleFS, "/topic/topic.txt");
    logger.iL("Topic data:\n%s", readTopicStr.c_str());*/
    vTaskDelay(1000);
    return true;
}

void Pairing::taskFunction(void *pvParam) {
    Pairing *instance = static_cast<Pairing *>(pvParam);
    instance->pairingTimer();
    vTaskDelete(NULL);
}

void Pairing::pairingTimer() {
    while (1) {
        counter++;
        Serial.println(counter);
        if (pairingSuccess) {
            logger.iL("paire Success");
            result(true);
            break;
        }
        if (counter > pairingTime) {
            logger.eL("paire faild");
            result(false);
            break;
        }
        vTaskDelay(1000);
    }
}

void Pairing::pairingMassage(String &topic, String &payload) {
    if (!onPairing && topic == pairingTopic && payload.startsWith("userID:")) {
        onPairing = true;
        mqttClient->unSubscribe(pairingTopic);
        // mqttClient->sendMessage(pairingTopic, "connected", 2);
        while (!mqttClient->pairingMsg(pairingTopic, "connected")) {
            vTaskDelay(200);
        }
        String mqttMsg = payload;
        counter = 0;
        bool result = recievePairingMessage(mqttMsg);
        pairingSuccess = result;
    }
}

String Pairing::getTopic() {
    return topicStr;
}

Pairing &Pairing::resultCallback(RESULT_SIGNATURE) {
    this->result = result;
    return *this;
};
