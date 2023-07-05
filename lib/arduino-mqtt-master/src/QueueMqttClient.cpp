#include "QueueMqttClient.h"

QueueMQTTClient::QueueMQTTClient(String ssid, String serialNum) : _ssid(ssid), _serialNum(serialNum) {}
QueueMQTTClient::~QueueMQTTClient() {
    mqttClient.~MQTTClient();
    if (taskHandle1 != NULL) {
        vTaskDelete(taskHandle1);
        logger.wL("mqttLoop() is deleted!");
    }
}

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
                vTaskDelay(250);
                return true;
            } else {
                logger.eL("CertFile load failed");
            }
        }
        caFile.close();
    }
    return false;
}

void QueueMQTTClient::connectToMqtt() {
    if (loadCertFile()) {
        logger.iL("MQTT connecting...");
        mqttClient.setOptions(10, true, 10000);
        mqttClient.begin(MQTT_HOST, MQTT_PORT, secureClient);
        while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
            vTaskDelay(250);
        }
        logger.iL("connected broker:%s", MQTT_HOST);
        vTaskDelay(1000);
        QueueMQTTClient *params = this;
        xTaskCreatePinnedToCore(taskFunctionLoop, "mqttLoop", 8192, (void *)params, 1, &taskHandle1, 1);
    }

    if (!loadTopic()) {
        createPairingFunction();
    } else {
        mqttClient.onMessage(MQTT_callback);
        MQTT_onConnect();
    }
}

bool QueueMQTTClient::loadTopic() {
    if (!initLittleFS()) {
        logger.eL("initial function data Failed!");
        return false;
    }

    String readTopicStr = readFile(LittleFS, "/topic/topic.txt");

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

    if (topicApp != nullptr && topicEsp != nullptr && topicPms != nullptr && topicTimer != nullptr && topicWifi != nullptr)
        logger.iL("\n%s\n%s\n%s\n%s\n%s", topicApp.c_str(), topicEsp.c_str(), topicPms.c_str(), topicTimer.c_str(), topicWifi.c_str());
    else {
        logger.eL("initial faild");
        return false;
    }

    subscribe(topicApp, 2);
    vTaskDelay(1000);
    subscribe(topicWifi, 2);
    vTaskDelay(1000);
    /*if (!subscribe(topicApp, 2) || !subscribe(topicWifi, 2))
        return false;*/

    mqttClient.setWill(topicEsp.c_str(), "#disEsp", false, 2);
    vTaskDelay(500);
    return true;
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

void QueueMQTTClient::taskFunctionLoop(void *pvParam) {
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
            int i = 0;
            logger.eL("lastError:%d", mqttClient.lastError());
            while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
                if (i < 60 * 4)
                    i++;
                else
                    break;
                vTaskDelay(250);
            }
            if (pairing != nullptr) {
                delete pairing;
                pairing = nullptr;
                removeTopic();
                createPairingFunction();
                logger.iL("MQqtt Pairing is Restart!");
            } else {
                logger.iL("MQqtt is ReConnected!");
                if (loadTopic()) {
                    MQTT_onConnect();
                    uint16_t lastPacketID = mqttClient.lastPacketID();
                    mqttClient.prepareDuplicate(lastPacketID);
                } else {
                    break;
                }
            }
        }
        if (WiFi.status() != WL_CONNECTED) {
            logger.eL("Wifi disconnected");
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
    delete this;
}

/////////////////////////CallBack Set/////////////////////////
//////////////////////////////////////////////////////////////

QueueMQTTClient &QueueMQTTClient::mqttCallback(MQTT_CALLBACK_SIGNATURE) {
    this->MQTT_callback = MQTT_callback;
    return *this;
};

QueueMQTTClient &QueueMQTTClient::onMqttConnect(MQTT_ONCONNECT_SIGNATURE) {
    this->MQTT_onConnect = MQTT_onConnect;
    return *this;
};

QueueMQTTClient &QueueMQTTClient::reset(RESET_SIGNATURE) {
    this->MQTT_reset = MQTT_reset;
    return *this;
};

String QueueMQTTClient::getTopicApp() {
    return topicApp;
}

String QueueMQTTClient::getTopicWifi() {
    return topicWifi;
}

/////////////////////////////API/////////////////////////////
/////////////////////////////////////////////////////////////

void QueueMQTTClient::Esp(String msg) {
    sendMessage(topicEsp, msg, 2);
}

void QueueMQTTClient::Pms(String msg) {
    sendMessage(topicPms, msg, 0);
}
void QueueMQTTClient::Timer(String msg) {
    sendMessage(topicTimer, msg, 2);
}
bool QueueMQTTClient::subscribe(String topic, int qos) {
    bool result = mqttClient.subscribe(topic, qos);
    if (result) {
        logger.iL("Subscribe %s Success", topic.c_str());
    } else {
        logger.eL("Subscribe %s Faild", topic.c_str());
    }
    return result;
}
bool QueueMQTTClient::unSubscribe(String topic) {
    bool result = mqttClient.unsubscribe(topic);
    if (result) {
        logger.iL("unSubscribe %s Success", topic.c_str());
    } else {
        logger.eL("unSubscribe %s Faild", topic.c_str());
    }
    return result;
}

/////////////////////////Pairing Use/////////////////////////
/////////////////////////////////////////////////////////////

void QueueMQTTClient::createPairingFunction() {
    if (pairing == nullptr) {
        pairing = new Pairing(this, _serialNum, _ssid, 60);
        pairing->resultCallback([this](String result) {
            this->pairingResult(result);
        });
        mqttClient.onMessage([this](String &topic, String &payload) {
            pairing->pairingMassage(topic, payload);
        });
        pairing->start();
    }
}

void QueueMQTTClient::pairingResult(String topicStr) {
    saveTopic(topicStr);
    if (loadTopic()) {
        mqttClient.onMessage(MQTT_callback);
        sendMessage(_ssid, "connected", 2);
        MQTT_onConnect();
    } else {
        removeTopic();
        logger.eL("Pairing Faild!");
    }
    delete pairing;
    pairing = nullptr;
}

void QueueMQTTClient::saveTopic(String topicStr) {
    writeFile2(LittleFS, "/topic/topic.txt", topicStr.c_str());
}

void QueueMQTTClient::removeTopic() {
    deleteFile(LittleFS, "/topic/topic.txt");
    deleteFile(LittleFS, "/wifi/wifi.txt");
}