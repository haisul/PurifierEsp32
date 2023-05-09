#ifndef QUEUE_MQTT_CLIENT_H
#define QUEUE_MQTT_CLIENT_H

#include "MQTTClient.h"
#include "littlefsfun.h"
#include "loggerESP.h"
#include <ArduinoJson.h>
#include <QueueList.h>
#include <WiFiClientSecure.h>

#define MQTT_CALLBACK_SIGNATURE std::function<void(String & topic, String & payload)> MQTT_callback

class QueueMQTTClient {
private:
    LoggerESP logger;

    const char *MQTT_HOST = "ya0e11c3.ala.us-east-1.emqxsl.com";
    const uint8_t MQTT_PORT = 8883;
    const char *MQTT_CLIENT_ID = "mqtt_esp32";
    const char *MQTT_USERNAME = "LJ_IEP";
    const char *MQTT_PASSWORD = "33456789";

    MQTTClient mqttClient;
    WiFiClientSecure *secureClient;

    MQTT_CALLBACK_SIGNATURE;

    bool deleteTopic = false;
    bool isPaire = false;

public:
    String paireTopic;
    String topicApp;
    String topicEsp, topicPms, topicTimer;

    QueueMQTTClient();
    void setCaFile(WiFiClientSecure *secureClient);
    void connectToMqtt(String SSID, String serialNum);
    void loadTopic();
    bool pairing(String step, String paireTopic = "", String serialNum = "", String userId = "");
    void onMqttConnect(void *pvParam);
    void sendMessage(const String targetTopic, const String payload);
    QueueMQTTClient &mqttCallback(MQTT_CALLBACK_SIGNATURE);
};

#endif