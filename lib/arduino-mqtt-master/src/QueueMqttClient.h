#ifndef QUEUE_MQTT_CLIENT_H
#define QUEUE_MQTT_CLIENT_H

#include "MQTTClient.h"
#include "littlefsfun.h"
#include "loggerESP.h"
#include <ArduinoJson.h>
#include <QueueList.h>
#include <WiFiClientSecure.h>

#define MQTT_CALLBACK_SIGNATURE std::function<void(String & topic, String & payload)> MQTT_callback
#define QoS0 qos0
#define QoS1 qos1
#define QoS2 qos2

class QueueMQTTClient {
private:
    LoggerESP logger;

    typedef enum {
        qos0 = 0,
        qos1,
        qos2
    } Qos;

    const char *MQTT_HOST = "ya0e11c3.ala.us-east-1.emqxsl.com";
    const uint8_t MQTT_PORT = 8883;
    const char *MQTT_CLIENT_ID = "mqtt_esp32";
    const char *MQTT_USERNAME = "LJ_IEP";
    const char *MQTT_PASSWORD = "33456789";

    MQTTClient mqttClient;
    WiFiClientSecure *secureClient;
    QueueList QoS0_Queue, QoS1_Queue, QoS2_Queue;

    MQTT_CALLBACK_SIGNATURE;

    bool deleteTopic = false;
    // bool isPaire = false;
    uint32_t lastMsg = millis();

    String serialNum;
    String paireTopic;
    String topicApp;
    String topicEsp, topicPms, topicTimer;

    bool loadTopic();
    void mqttLoop(void *pvParam);

public:
    QueueMQTTClient();
    void setCaFile(WiFiClientSecure *secureClient);
    void connectToMqtt(String SSID, String serialNum);
    bool pairing(String step, String userId = "");
    void onMqttConnect(void *pvParam);
    void pairingConnect(void *pvParam);
    void sendMessage(const String targetTopic, const String payload, Qos qos);
    QueueMQTTClient &mqttCallback(MQTT_CALLBACK_SIGNATURE);
    void paireMassage(String &topic, String &payload);
};

#endif