#ifndef QUEUE_MQTT_CLIENT_H
#define QUEUE_MQTT_CLIENT_H

#include "MQTTClient.h"
#include "littlefsfun.h"
#include "loggerESP.h"
#include <ArduinoJson.h>
#include <QueueList.h>
#include <WiFiClientSecure.h>

#include "Pairing.h"

#define MQTT_CALLBACK_SIGNATURE std::function<void(String & topic, String & payload)> MQTT_callback
#define MQTT_ONCONNECT_SIGNATURE std::function<void()> MQTT_onConnect
#define RESET_SIGNATURE std::function<void()> MQTT_reset

class QueueMQTTClient {
private:
    typedef enum {
        qos0 = 0,
        qos1,
        qos2
    } Qos;

    const char *MQTT_HOST = "ya0e11c3.ala.us-east-1.emqxsl.com";
    const uint16_t MQTT_PORT = 8883;
    const char *MQTT_CLIENT_ID = "mqtt_esp32";
    const char *MQTT_USERNAME = "LJ_IEP";
    const char *MQTT_PASSWORD = "33456789";

    Pairing *pairing;
    MQTTClient mqttClient = MQTTClient(1024);
    WiFiClientSecure secureClient;
    QueueList QoS0_Queue, QoS1_Queue, QoS2_Queue;

    MQTT_CALLBACK_SIGNATURE;
    MQTT_ONCONNECT_SIGNATURE;
    RESET_SIGNATURE;

    uint32_t lastMsg = millis();

    String _serialNum;
    String _ssid;
    String topicApp;
    String topicEsp, topicPms, topicTimer, topicWifi;

    TaskHandle_t taskHandle1;

    bool loadCertFile();

    static void taskFunctionLoop(void *pvParam);
    void mqttLoop();

    void pairingResult(bool);
    void saveTopic(String);

public:
    QueueMQTTClient(String, String);
    ~QueueMQTTClient();

    void connectToMqtt();
    void sendMessage(const String targetTopic, const String payload, int qos);
    QueueMQTTClient &mqttCallback(MQTT_CALLBACK_SIGNATURE);
    QueueMQTTClient &onMqttConnect(MQTT_ONCONNECT_SIGNATURE);
    QueueMQTTClient &reset(RESET_SIGNATURE);
    void subscribe(String &topic, int qos);
    void unSubscribe(String &topic);
    bool pairingMsg(String, String);
    bool loadTopic();

    String getTopicApp();
    String getTopicWifi();
    void Esp(String);
    void Pms(String);
    void Timer(String);
};

#endif