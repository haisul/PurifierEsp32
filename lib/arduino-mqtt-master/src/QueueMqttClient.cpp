#include "QueueMqttClient.h"

QueueMQTTClient::QueueMQTTClient() {}

bool QueueMQTTClient::loadCertFile() {
    if (!initLittleFS()) {
        logger.eL("Load CertFile Failed!");
    } else {
        File caFile = LittleFS.open("/emqxsl-ca.crt", "r");
        if (!caFile) {
            logger.e("Failed to open CertFile");
        } else {
            secureClient.flush();
            if (secureClient.loadCACert(caFile, caFile.size())) {
                logger.iL("CertFile load success");
                caFile.close();
                return true;
            } else {
                logger.eL("CertFile load failed");
            }
        }
        caFile.close();
    }

    return false;
}

void QueueMQTTClient::connectToMqtt(String SSID, String serialNum) {
    this->paireTopic = SSID;
    this->serialNum = serialNum;

    if (loadCertFile()) {
        mqttClient.setOptions(10, false, 2000);
        logger.iL("MQTT connecting...");
        mqttClient.begin(MQTT_HOST, MQTT_PORT, secureClient);
        while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
            vTaskDelay(250);
        }
        logger.iL("MQTT is connected");
    }

    if (!loadTopic()) {
        mqttClient.onMessage([this](String &topic, String &payload) {
            this->paireMassage(topic, payload);
        });
        pairingSuccess = false;
        QueueMQTTClient *params = this;
        xTaskCreatePinnedToCore(taskFunction2, "pairingTimer", 1024, (void *)params, 1, NULL, 1);
        pairing("step1", "");
    } else {
        mqttClient.onMessage(MQTT_callback);
    }

    QueueMQTTClient *params = this;
    xTaskCreatePinnedToCore(taskFunction, "mqttLoop", 8192, (void *)params, 1, NULL, 1);
}

bool QueueMQTTClient::loadTopic() {
    String readTopicStr = readFile(LittleFS, "/initial/topic.txt");

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, readTopicStr);

    if (error) {
        logger.eL("Deserialize Topic failed: %s", error.c_str());
        return false;
    }

    topicApp = doc["topicApp"].as<String>();
    topicEsp = doc["topicEsp"].as<String>();
    topicPms = doc["topicPms"].as<String>();
    topicTimer = doc["topicTimer"].as<String>();
    topicWifi = doc["topicWifi"].as<String>();

    logger.iL("\n%s\n%s\n%s\n%s", topicApp.c_str(), topicEsp.c_str(), topicPms.c_str(), topicTimer.c_str(), topicWifi.c_str());

    subscribe(topicApp, 2);
    subscribe(topicWifi, 2);

    mqttClient.setWill(topicEsp.c_str(), "#disEsp", false, 2);

    if (MQTT_onConnect) {
        MQTT_onConnect(); // 調用回調函數
    }
    return true;
}

bool QueueMQTTClient::pairing(String step, String userId) {

    if (step == "step1") {
        subscribe(paireTopic, 2);
        sendMessage(paireTopic, serialNum, 2);
        deleteTopic = true;
        return true;
    } else if (step == "step2" && userId != "") {
        serialNum.replace("serialNum:", "");
        userId.replace("userID:", "");
        topicApp = userId + "/" + serialNum + "/app";
        topicEsp = userId + "/" + serialNum + "/esp";
        topicPms = userId + "/" + serialNum + "/pms";
        topicTimer = userId + "/" + serialNum + "/timer";
        topicWifi = userId + "/" + serialNum + "/wifi";

        subscribe(topicApp, 2);
        subscribe(topicWifi, 2);

        StaticJsonDocument<384> j_topic;
        j_topic["topicApp"] = userId + "/" + serialNum + "/app";
        j_topic["topicEsp"] = userId + "/" + serialNum + "/esp";
        j_topic["topicPms"] = userId + "/" + serialNum + "/pms";
        j_topic["topicTimer"] = userId + "/" + serialNum + "/timer";
        j_topic["topicWifi"] = userId + "/" + serialNum + "/wifi";

        String topicStr;
        serializeJson(j_topic, topicStr);
        writeFile2(LittleFS, "/initial/topic.txt", topicStr.c_str());
        sendMessage(paireTopic, "connected", 2);
        while (!deleteTopic) {
            vTaskDelay(250);
        }

        unSubscribe(paireTopic);

        mqttClient.setWill(topicEsp.c_str(), "#disEsp", false, 2);
        mqttClient.onMessage(MQTT_callback);
        return true;
    } else
        return false;
}

void QueueMQTTClient::sendMessage(const String targetTopic, const String payload, int qos) {
    if (mqttClient.connected()) {
        if (payload != nullptr || payload != "") {
            String msg[] = {targetTopic, payload};
            switch (qos) {
            case 0:
                QoS0_Queue.addQueue(msg, 2);
                break;
            case 1:
                QoS1_Queue.addQueue(msg, 2);
                break;
            case 2:
                QoS2_Queue.addQueue(msg, 2);
                break;
            }
        }
    }
}

void QueueMQTTClient::subscribe(String &topic, int qos) {
    bool result = mqttClient.subscribe(topic, qos);
    if (result) {
        logger.iL("Subscribe %s Success", topic.c_str());
    } else {
        logger.eL("Subscribe %s Faild", topic.c_str());
    }
}

void QueueMQTTClient::unSubscribe(String &topic) {
    bool result = mqttClient.unsubscribe(topic);
    if (result) {
        logger.iL("unSubscribe %s Success", topic.c_str());
    } else {
        logger.eL("unSubscribe %s Faild", topic.c_str());
    }
}

void QueueMQTTClient::taskFunction(void *pvParam) {
    QueueMQTTClient *instance = static_cast<QueueMQTTClient *>(pvParam);
    instance->mqttLoop();
    vTaskDelete(NULL);
}

void QueueMQTTClient::mqttLoop() {
    logger.iL("Mqtt Loop begin");
    String topic;
    String message;
    bool result;

    while (1) {

        if (!mqttClient.connected()) {
            logger.eL("lastError:%d", mqttClient.lastError());
            while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
                vTaskDelay(250);
            }
            if (MQTT_onConnect) {
                MQTT_onConnect(); // 調用回調函數
            }
            logger.iL("MQqtt is ReConnected!");
            uint16_t lastPacketID = mqttClient.lastPacketID();
            mqttClient.prepareDuplicate(lastPacketID);
        }
        if (WiFi.status() != WL_CONNECTED) {
            logger.eL("Wifi disconnected");
            mqttClient.disconnect();
            logger.eL("Mqtt disconnected");
            break;
        }

        if (!QoS0_Queue.isQueueEmpty()) {
            topic = QoS0_Queue.getQueue();
            message = QoS0_Queue.getQueue();
            result = mqttClient.publish(topic, message, false, 0);
            if (result) {
                logger.wL("\n[MQTT][Send][Success][QoS0][Topic:%s]\n%s\n", topic.c_str(), message.c_str());
            } else {
                logger.eL("\n[MQTT][Send][Faild][QoS0][Topic:%s]\n%s\n", topic.c_str(), message.c_str());
            }
        }

        if (millis() - lastMsg > 1000) {
            if (!QoS1_Queue.isQueueEmpty()) {
                topic = QoS1_Queue.getQueue();
                message = QoS1_Queue.getQueue();
                result = mqttClient.publish(topic, message, false, 1);
                if (result) {
                    logger.wL("\n[MQTT][Send][Success][QoS0][Topic:%s]\n%s\n", topic.c_str(), message.c_str());
                } else {
                    logger.eL("\n[MQTT][Send][Faild][QoS2][Topic:%s]\n%s\n", topic.c_str(), message.c_str());
                }
            }
            if (!QoS2_Queue.isQueueEmpty()) {
                topic = QoS2_Queue.getQueue();
                message = QoS2_Queue.getQueue();
                result = mqttClient.publish(topic, message, false, 2);
                if (result) {
                    logger.wL("\n[MQTT][Send][Success][QoS2][Topic:%s]\n%s\n", topic.c_str(), message.c_str());
                } else {
                    logger.eL("\n[MQTT][Send][Faild][QoS2][Topic:%s]\n%s\n", topic.c_str(), message.c_str());
                }
            }
            lastMsg = millis();
        }
        mqttClient.loop();
        vTaskDelay(20);
    }
}

void QueueMQTTClient::taskFunction2(void *pvParam) {
    QueueMQTTClient *instance = static_cast<QueueMQTTClient *>(pvParam);
    instance->pairingTimer();
    vTaskDelete(NULL);
}

void QueueMQTTClient::pairingTimer() {
    uint8_t count = 0;
    while (1) {
        count++;
        if (count > 90)
            if (MQTT_pairingFaild)
                MQTT_pairingFaild();
        if (pairingSuccess)
            break;
        vTaskDelay(1000);
    }
    vTaskDelete(NULL);
}

void QueueMQTTClient::paireMassage(String &topic, String &payload) {
    if (payload != nullptr && payload != "") {
        String mqttMsg = "";
        mqttMsg = payload;
        if (topic == paireTopic && payload.startsWith("userID:") && !isPairing) {
            isPairing = true;
            logger.iL("Pairing...");
            bool result = pairing("step2", mqttMsg);
            if (result)
                pairingSuccess = true;
        }
    }
}

QueueMQTTClient &QueueMQTTClient::mqttCallback(MQTT_CALLBACK_SIGNATURE) {
    this->MQTT_callback = MQTT_callback;
    return *this;
};

QueueMQTTClient &QueueMQTTClient::onMqttConnect(MQTT_ONCONNECT_SIGNATURE) {
    this->MQTT_onConnect = MQTT_onConnect;
    return *this;
};

QueueMQTTClient &QueueMQTTClient::mqttPairingFaild(MQTT_PAIRINGFAILD_SIGNATURE) {
    this->MQTT_pairingFaild = MQTT_pairingFaild;
    return *this;
};

String QueueMQTTClient::getTopicEsp() {
    return topicEsp;
}
String QueueMQTTClient::getTopicApp() {
    return topicApp;
}
String QueueMQTTClient::getTopicPms() {
    return topicPms;
}
String QueueMQTTClient::getTopicTimer() {
    return topicTimer;
}
String QueueMQTTClient::getTopicWifi() {
    return topicWifi;
}