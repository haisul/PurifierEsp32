#include "QueueMqttClient.h"

QueueMQTTClient::QueueMQTTClient() {
    mqttClient = MQTTClient(1024);
    mqttClient.setOptions(30, true, 2000);
}

void QueueMQTTClient::setCaFile(WiFiClientSecure *secureClient) {
    this->secureClient = secureClient;
}

void QueueMQTTClient::connectToMqtt(String SSID, String serialNum) {
    mqttClient.begin(MQTT_HOST, MQTT_PORT, *secureClient);
    while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        vTaskDelay(250);
    }
    logger.i("MQTT is connected!");

    loadTopic();
    String *params = new String[2]{SSID, serialNum};
    xTaskCreatePinnedToCore((TaskFunction_t)&QueueMQTTClient::onMqttConnect, "onMqttConnect", 8192, params, 1, NULL, 0);
}

void QueueMQTTClient::loadTopic() {
    String readTopicStr = readFile(LittleFS, "/initial/topic.txt");

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, readTopicStr);

    if (error) {
        Serial.printf("deserializeJson() failed: %s", error.c_str());
        return;
    }

    topicApp = doc["topicApp"].as<String>();
    topicEsp = doc["topicEsp"].as<String>();
    topicPms = doc["topicPms"].as<String>();
    topicTimer = doc["topicTimer"].as<String>();

    mqttClient.subscribe(topicApp);
}

bool QueueMQTTClient::pairing(String step, String paireTopic = "", String serialNum = "", String userId = "") {
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

void QueueMQTTClient::onMqttConnect(void *pvParam) {
    String *params = (String *)pvParam;
    String paireTopic = params[0];
    String serialNum = params[1];
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
            vTaskDelete(NULL);
        }
        if (isPaire) {
            pairing("step1", paireTopic, serialNum, "");
            isPaire = false;
        }
        vTaskDelay(20);
    }
    vTaskDelete(NULL);
}

void QueueMQTTClient::sendMessage(const String targetTopic, const String payload) {
    if (payload != nullptr || payload != "") {
        Serial.printf(targetTopic.c_str());
        Serial.printf(payload.c_str());
        String msg = "#" + payload;
        mqttClient.publish(targetTopic, msg);
    }
}

QueueMQTTClient &QueueMQTTClient::mqttCallback(MQTT_CALLBACK_SIGNATURE) {
    this->MQTT_callback = MQTT_callback;
    mqttClient.onMessage(MQTT_callback);
    return *this;
};