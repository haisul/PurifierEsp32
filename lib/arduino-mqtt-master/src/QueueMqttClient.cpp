#include "QueueMqttClient.h"

QueueMQTTClient::QueueMQTTClient() {
    mqttClient = MQTTClient(1024);
    mqttClient.setOptions(30, true, 2000);
}

void QueueMQTTClient::setCaFile(WiFiClientSecure secureClient) {
    this->secureClient = secureClient;
}

void QueueMQTTClient::connectToMqtt(String SSID, String serialNum) {
    mqttClient.begin(MQTT_HOST, MQTT_PORT, secureClient);
    while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        vTaskDelay(250);
    }
    logger.i("MQTT is connected!");

    // xTaskCreatePinnedToCore((TaskFunction_t)&QueueMQTTClient::taskFunction, "mqttLoop", 8192, this, 1, NULL, 0);
    // xTaskCreatePinnedToCore((TaskFunction_t)(&QueueMQTTClient::mqttLoop), "mqttLoop", 8192, static_cast<void*>(this), 1, NULL, 0);
    xTaskCreatePinnedToCore(taskFunction, "mqttLoopk", 8192, NULL, 1, NULL, 0);
    if (!loadTopic()) {
        mqttClient.onMessage([this](String &topic, String &payload) {
            this->paireMassage(topic, payload);
        });
        this->paireTopic = paireTopic;
        this->serialNum = serialNum;
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
        Serial.printf("deserializeJson() failed: %s", error.c_str());
        return false;
    }

    topicApp = doc["topicApp"].as<String>();
    topicEsp = doc["topicEsp"].as<String>();
    topicPms = doc["topicPms"].as<String>();
    topicTimer = doc["topicTimer"].as<String>();

    mqttClient.subscribe(topicApp);
    return true;
}

bool QueueMQTTClient::pairing(String step, String userId) {
    if (step == "step1") {
        logger.i("subscribe %s : %s", paireTopic.c_str(), mqttClient.subscribe(paireTopic, 2) ? "true" : "false");
        while (!mqttClient.publish(paireTopic, serialNum, false, 2)) {
            vTaskDelay(250);
        }
        deleteTopic = true;
        return true;
    } else if (step == "step2" && userId != "") {
        mqttClient.publish(paireTopic, "connected", false, 2);

        serialNum.replace("serialNum:", "");
        userId.replace("userID:", "");
        topicApp = userId + "/" + serialNum + "/app";
        topicEsp = userId + "/" + serialNum + "/esp";
        topicPms = userId + "/" + serialNum + "/pms";
        topicTimer = userId + "/" + serialNum + "/timer";
        logger.e("subscribe %s : %s", topicApp.c_str(), mqttClient.subscribe(topicApp) ? "true" : "false");

        StaticJsonDocument<384> j_topic;
        j_topic["topicApp"] = userId + "/" + serialNum + "/app";
        j_topic["topicEsp"] = userId + "/" + serialNum + "/esp";
        j_topic["topicPms"] = userId + "/" + serialNum + "/pms";
        j_topic["topicTimer"] = userId + "/" + serialNum + "/timer";

        String topicStr;
        serializeJson(j_topic, topicStr);
        writeFile2(LittleFS, "/initial/topic.txt", topicStr.c_str());
        // function.refresh("MQTT");
        mqttClient.unsubscribe(paireTopic);
        return true;
    } else
        return false;
}

void QueueMQTTClient::sendMessage(const String targetTopic, const String payload, int qos) {
    if (payload != nullptr || payload != "") {
        String msg = "#" + payload;
        switch (qos) {
        case 0:
            QoS0_Queue.addToQueue(msg);
            break;
        case 1:
            QoS1_Queue.addToQueue(msg);
            break;
        case 2:
            QoS2_Queue.addToQueue(msg);
            break;
        }
    }
}

void QueueMQTTClient::taskFunction(void *pvParam) {
    QueueMQTTClient *instance = static_cast<QueueMQTTClient *>(pvParam);
    instance->mqttLoop();
}

void QueueMQTTClient::mqttLoop() {
    while (1) {
        if (!mqttClient.loop()) {
            logger.e("lastError:%d", mqttClient.lastError());
        }

        if (!mqttClient.connected()) {
            while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
                Serial.printf(".");
                vTaskDelay(500);
            }
            logger.i("MQTT is connected!");
        }
        if (WiFi.status() != WL_CONNECTED) {
            Serial.printf("mqttClient.disconnect");
            mqttClient.disconnect();
            break;
        }

        if (!QoS0_Queue.isQueueEmpty()) {
            String message = QoS0_Queue.getFromQueue();
            bool result = mqttClient.publish("test", message, false, 0);
            logger.w("QoS0:%s", result ? "true" : "false");
        }

        if (millis() - lastMsg > 1000) {
            if (!QoS1_Queue.isQueueEmpty()) {
                String message = QoS1_Queue.getFromQueue();
                bool result = mqttClient.publish("test", message, false, 1);
                logger.w("QoS1:%s", result ? "true" : "false");
            }
            if (!QoS2_Queue.isQueueEmpty()) {
                String message = QoS2_Queue.getFromQueue();
                bool result = mqttClient.publish("test", message, false, 2);
                logger.w("QoS2:%s", result ? "true" : "false");
            }
            lastMsg = millis();
        }
        vTaskDelay(10);
    }
    vTaskDelete(NULL);
}

QueueMQTTClient &QueueMQTTClient::mqttCallback(MQTT_CALLBACK_SIGNATURE) {
    this->MQTT_callback = MQTT_callback;
    return *this;
};

void QueueMQTTClient::paireMassage(String &topic, String &payload) {
    if (payload != nullptr && payload != "") {
        String mqttMsg = "";
        mqttMsg = payload;
        if (topic == paireTopic && payload.startsWith("userID:")) {
            logger.e("pairing...");
            pairing("step2", mqttMsg);
        }
    }
}