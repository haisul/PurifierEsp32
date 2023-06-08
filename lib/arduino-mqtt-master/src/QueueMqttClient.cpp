#include "QueueMqttClient.h"

QueueMQTTClient::QueueMQTTClient() {}

bool QueueMQTTClient::loadCertFile() {
    if (!initLittleFS()) {
        logger.e("initial LittleFS Failed!");
    } else {
        File caFile = LittleFS.open("/emqxsl-ca.crt", "r");
        if (!caFile) {
            logger.e("Failed to open file");
        } else {
            secureClient.flush();
            if (secureClient.loadCACert(caFile, caFile.size())) {
                logger.i("CA cert load success");
                caFile.close();
                return true;
            } else {
                logger.e("CA cert load failed");
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
        mqttClient.begin(MQTT_HOST, MQTT_PORT, secureClient);
        while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
            vTaskDelay(250);
        }
        logger.i("MQTT is connected!");
    }
    QueueMQTTClient *params = this;
    xTaskCreatePinnedToCore(taskFunction, "mqttLoop", 8192, (void *)params, 1, NULL, 1);

    if (!loadTopic()) {
        mqttClient.onMessage([this](String &topic, String &payload) {
            this->paireMassage(topic, payload);
        });
        pairing("step1", "");
    } else {
        mqttClient.onMessage(MQTT_callback);
    }
}

bool QueueMQTTClient::loadTopic() {
    String readTopicStr = readFile(LittleFS, "/initial/topic.txt");

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, readTopicStr);

    if (error) {
        logger.e("deserializeJson() failed: %s", error.c_str());
        return false;
    }

    topicApp = doc["topicApp"].as<String>();
    topicEsp = doc["topicEsp"].as<String>();
    topicPms = doc["topicPms"].as<String>();
    topicTimer = doc["topicTimer"].as<String>();

    logger.w("subscribe %s : %s", topicApp.c_str(), mqttClient.subscribe(topicApp, 2) ? "true" : "false");
    mqttClient.setWill(topicEsp.c_str(), "#disEsp", false, 2);
    logger.i("%s\n%s\n%s\n%s", topicApp.c_str(), topicEsp.c_str(), topicPms.c_str(), topicTimer.c_str());
    if (MQTT_onConnect) {
        MQTT_onConnect(); // 調用回調函數
    }
    return true;
}

bool QueueMQTTClient::pairing(String step, String userId) {
    if (step == "step1") {
        logger.w("subscribe %s : %s", paireTopic.c_str(), mqttClient.subscribe(paireTopic, 2) ? "true" : "false");
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
        logger.w("subscribe %s : %s", topicApp.c_str(), mqttClient.subscribe(topicApp, 2) ? "true" : "false");

        StaticJsonDocument<384> j_topic;
        j_topic["topicApp"] = userId + "/" + serialNum + "/app";
        j_topic["topicEsp"] = userId + "/" + serialNum + "/esp";
        j_topic["topicPms"] = userId + "/" + serialNum + "/pms";
        j_topic["topicTimer"] = userId + "/" + serialNum + "/timer";

        String topicStr;
        serializeJson(j_topic, topicStr);
        writeFile2(LittleFS, "/initial/topic.txt", topicStr.c_str());
        sendMessage(paireTopic, "connected", 2);
        while (!deleteTopic) {
            vTaskDelay(250);
        }
        logger.w("unsubscribe %s : %s", paireTopic.c_str(), mqttClient.unsubscribe(paireTopic) ? "true" : "false");
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
    mqttClient.subscribe(topic, qos);
}

void QueueMQTTClient::taskFunction(void *pvParam) {
    QueueMQTTClient *instance = static_cast<QueueMQTTClient *>(pvParam);
    instance->mqttLoop();
    vTaskDelete(NULL);
}

void QueueMQTTClient::mqttLoop() {
    String topic;
    String message;
    bool result;

    while (1) {

        if (!mqttClient.connected()) {
            logger.e("lastError:%d", mqttClient.lastError());
            while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
                vTaskDelay(250);
            }
            if (MQTT_onConnect) {
                MQTT_onConnect(); // 調用回調函數
            }
            logger.w("MQTT is ReConnected!");
            uint16_t lastPacketID = mqttClient.lastPacketID();
            mqttClient.prepareDuplicate(lastPacketID);
        }
        if (WiFi.status() != WL_CONNECTED) {
            Serial.printf("mqttClient.disconnect");
            mqttClient.disconnect();
            break;
        }

        if (!QoS0_Queue.isQueueEmpty()) {
            topic = QoS0_Queue.getQueue();
            message = QoS0_Queue.getQueue();
            result = mqttClient.publish(topic, message, false, 0);
            if (result) {
                logger.i("QoS0\nMsg:%s\nSend Msg Successes!", message.c_str());
            } else {
                logger.e("QoS0\nMsg:%s\nSend Msg Faild!", message.c_str());
            }
        }

        if (millis() - lastMsg > 1000) {
            if (!QoS1_Queue.isQueueEmpty()) {
                topic = QoS1_Queue.getQueue();
                message = QoS1_Queue.getQueue();
                result = mqttClient.publish(topic, message, false, 1);
                if (result) {
                    logger.i("QoS1\nMsg:%s\nSend Msg Successes!", message.c_str());
                } else {
                    logger.e("QoS1\nMsg:%s\nSend Msg Faild!", message.c_str());
                }
            }
            if (!QoS2_Queue.isQueueEmpty()) {
                topic = QoS2_Queue.getQueue();
                message = QoS2_Queue.getQueue();
                result = mqttClient.publish(topic, message, false, 2);
                if (result) {
                    logger.i("QoS2\nMsg:%s\nSend Msg Successes!", message.c_str());
                } else {
                    logger.e("QoS2\nMsg:%s\nSend Msg Faild!", message.c_str());
                }
            }
            lastMsg = millis();
        }
        mqttClient.loop();
        vTaskDelay(20);
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

void QueueMQTTClient::paireMassage(String &topic, String &payload) {
    if (payload != nullptr && payload != "") {
        String mqttMsg = "";
        mqttMsg = payload;
        if (topic == paireTopic && payload.startsWith("userID:") && !isPairing) {
            isPairing = true;
            logger.i("pairing...");
            pairing("step2", mqttMsg);
        }
    }
}

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