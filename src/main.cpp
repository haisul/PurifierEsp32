#include "HmiConnect.h"
#include "funcControl.h"
#include "littlefsfun.h"
#include "loggerESP.h"
#include "rgbled.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <QueueList.h>
#include <WiFi.h>
#include <cstdlib>
QueueList messageQueue;
LoggerESP logger;
rgbLed Led;
/////////////////////////////////////////////
String chipid = String(ESP.getEfuseMac());
const String serialNum = "serialNum:$iep" + chipid;
/////////////////////////////////////////////
#include <MQTTClient.h>
#include <Ticker.h>
#include <WiFiClientSecure.h>
MQTTClient mqttClient(1024);
// TimerHandle_t mqttTimer;
WiFiClientSecure secureClient;
uint8_t wifiReconnectCount = 0;
// uint8_t mqttReconnectCount = 0;
bool isWifiDisconnectCreated = false;
bool isWifiSmartConfigCreated = false;
bool isPaire = false;

// const char *ca_cert;
//////////////////////////////////////////////
// #include <AsyncElegantOTA.h>
// #include <ESPAsyncWebServer.h>
//  AsyncWebServer server(80);
//////////////////////////////////////////////
#include <PMserial.h>
HardwareSerial pmsSerial(1);
SerialPM pms(PMS5003, 5, 18);
//////////////////////////////////////////////
#define LEDpin 2
//////////////////////////////////////////////
const uint8_t powerSignal = 34;
const uint8_t powerSupply = 25;
//////////////////////////////////////////////
HmiConnect HMI(Serial2, 9600);
funcControl function;
//////////////////////////////////////////////
bool isTaskCreated = false;

#define MQTT_HOST "ya0e11c3.ala.us-east-1.emqxsl.com"
#define MQTT_PORT 8883
#define MQTT_CLIENT_ID "mqtt_esp32"
#define MQTT_USERNAME "LJ_IEP"
#define MQTT_PASSWORD "33456789"

String paireTopic = "";
String topicApp;
String topicEsp, topicPms, topicTimer;

void smartConfig(void *pvParam);
void MqttSendMessage(String, String);
void connectToWifi();
void loadTopic();

void task(void *pvParameters) {
    isTaskCreated = true;
    String From = ((String *)pvParameters)[0];
    String func = ((String *)pvParameters)[1];
    String set = ((String *)pvParameters)[2];
    String state = ((String *)pvParameters)[3];

    function.commend(func, set, state);
    if (From != "HMIDev") {
        if (From == "HMI")
            function.refresh("MQTT");
        if (From == "MQTT")
            function.refresh("HMI");
    }
    delete[] ((String *)pvParameters);
    isTaskCreated = false;
    vTaskDelete(NULL);
}

void msgProcess(String From, String msg) {
    Serial.printf("\n");
    uint8_t index1 = msg.indexOf('_');
    uint8_t index2 = msg.indexOf(':');
    String func = msg.substring(0, index1), set = "", state = "";

    if (index2 >= 0) {
        set = msg.substring(index1 + 1, index2);
        state = msg.substring(index2 + 1);
    } else {
        set = msg.substring(index1 + 1);
    }

    Serial.printf("From:%s\nfunc:%s\nset:%s\nstate:%s\n", From.c_str(), func.c_str(), set.c_str(), state.c_str());
    if (!isTaskCreated) {
        String *params = new String[5]{From, func, set, state};
        xTaskCreatePinnedToCore(task, "task", 4096, (void *)params, 2, NULL, 0); // 1176 bytes
    }
}

void HmiCallback(String HMI_msg) {
    String From = "HMI";
    if (HMI_msg.startsWith("!")) {
        HMI_msg.remove(0, 1);
        From = "HMIDev";
    } else if (HMI_msg == "refresh") {
        function.refresh("HMI");
    }
    msgProcess(From, HMI_msg);
}

// local Setting
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void MCUcommender(String target) {
    Serial.printf("target: %s", target);
    if (target == "HMI" || target == "") {
        String temp = "\"" + HMI.escapeJson(function.getInitialJson()) + "\"";
        HMI.sendMessage("AdminJson.Json.txt=" + temp);
    }
    if (target == "MQTT" || target == "")
        MqttSendMessage(topicEsp, function.getInitialJson());

    if (target == "wifion" || target == "wifiInfo") {
        isWifiDisconnectCreated = false;
        connectToWifi();
    } else if (target == "wifioff") {
        isWifiDisconnectCreated = true;
        WiFi.disconnect(true);
    } else if (target == "wifiConfig") {
        if (!isWifiSmartConfigCreated)
            xTaskCreatePinnedToCore(smartConfig, "smartConfig", 1024, NULL, 1, NULL, 0);
    }
}

void clock(void *pvParam) {
    String allTimerBuff, purTimerBuff, fogTimerBuff, uvcTimerBuff;
    StaticJsonDocument<384> j_timer;
    String timerStr;
    while (1) {
        if (function.all.startingCount) {
            allTimerBuff = String(function.all.endEpoch - function.rtc.getEpoch());
            HMI.sendMessage("settime.all_countTime.val=" + allTimerBuff);
            HMI.sendMessage("AdminDeveloper.n0.val=" + allTimerBuff);
            if (allTimerBuff == "0")
                function.funcState(ALL, OFF, true);
        }
        if (function.pur.startingCount && !function.all.startingCount) {
            purTimerBuff = String(function.pur.endEpoch - function.rtc.getEpoch());
            HMI.sendMessage("settime.pur_countTime.val=" + purTimerBuff);
            HMI.sendMessage("AdminDeveloper.n1.val=" + purTimerBuff);
            if (purTimerBuff == "0")
                function.funcState(PUR, OFF, true);
        }
        if (function.fog.startingCount && !function.all.startingCount) {
            fogTimerBuff = String(function.fog.endEpoch - function.rtc.getEpoch());
            HMI.sendMessage("settime.fog_countTime.val=" + fogTimerBuff);
            HMI.sendMessage("AdminDeveloper.n3.val=" + fogTimerBuff);
            if (fogTimerBuff == "0")
                function.funcState(FOG, OFF, true);
        }
        if (function.uvc.startingCount && !function.all.startingCount) {
            uvcTimerBuff = String(function.uvc.endEpoch - function.rtc.getEpoch());
            HMI.sendMessage("settime.uvc_countTime.val=" + uvcTimerBuff);
            HMI.sendMessage("AdminDeveloper.n4.val=" + uvcTimerBuff);
            if (uvcTimerBuff == "0")
                function.funcState(UVC, OFF, true);
        }
        if (function.all.startingCount || function.pur.startingCount || function.fog.startingCount || function.uvc.startingCount) {

            j_timer["allCountTime"] = allTimerBuff;
            j_timer["purCountTime"] = purTimerBuff;
            j_timer["fogCountTime"] = fogTimerBuff;
            j_timer["uvcCountTime"] = uvcTimerBuff;

            serializeJson(j_timer, timerStr);
            MqttSendMessage(topicTimer, timerStr);
            timerStr = "";
        }

        vTaskDelay(30 * 1000);
    }
}

void dust(void *pvParam) {
    String dustValStr = "0", tempValStr = "0", rhumValStr = "0";
    double dustValAvg = 0;
    uint16_t dustValTemp = 0;
    StaticJsonDocument<384> j_pms;
    String pmsStr;
    while (1) {
        pms.read();
        dustValStr = pms.pm25;
        dustValAvg = (pms.pm25 + dustValTemp) / 2;

        if (dustValAvg >= 999)
            dustValStr = "999";
        else {
            dustValStr = (uint16_t)dustValAvg;
            if (function._modeAuto)
                function.getDustVal((uint16_t)dustValAvg);
        }
        tempValStr = String(pms.n5p0 / 10);
        rhumValStr = String(pms.n10p0 / 10) + "." + String((pms.n10p0 - (pms.n10p0 / 10) * 10));

        if (abs(dustValAvg - dustValTemp) / dustValTemp >= 0.1) {
            HMI.sendMessage("home.pm25.txt=\"" + dustValStr + "\"");
            HMI.sendMessage("home.temp.txt=\"" + tempValStr + "\"");
            HMI.sendMessage("home.rhum.txt=\"" + rhumValStr + "\"");

            j_pms["pm25"] = rhumValStr;
            j_pms["temp"] = tempValStr;
            j_pms["rhum"] = rhumValStr;

            serializeJson(j_pms, pmsStr);
            MqttSendMessage(topicPms, pmsStr);
            pmsStr = "";
        }
        dustValTemp = (uint16_t)dustValAvg;
        vTaskDelay(1000);
    }
}

void local(void *pvParam) {
    vTaskDelay(2000);
    function_mode_types func[4] = {ALL, PUR, FOG, UVC};
    for (uint8_t i = 0; i < 4; i++)
        function.funcState(func[i], OFF);
    while (1) {
        HMI.loop();
        vTaskDelay(50);
    }
}

// MQTT Setting
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
void onMqttConnect(void *pvParam);
void connectToMqtt(String SSID) {
    mqttClient.begin(MQTT_HOST, MQTT_PORT, secureClient);
    Serial.printf("Connecting to MQTT...");
    while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.printf(".");
        vTaskDelay(500);
    }
    logger.i("MQTT is connected!");

    loadTopic();
    String *params = new String[2]{SSID, serialNum};
    xTaskCreatePinnedToCore(onMqttConnect, "onMqttConnect", 8192, params, 1, NULL, 0);
}

void loadTopic() {
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

bool deleteTopic = false;

bool pairing(String step, String paireTopic = "", String serialNum = "", String userId = "") {
    if (step == "step1") {
        logger.i("subscribe %s : %s", paireTopic.c_str(), mqttClient.subscribe(paireTopic, 2) ? "true" : "false");
        // mqttClient.publish(paireTopic, serialNum, false, 2);
        // logger.i("publish to %s : %s", paireTopic.c_str(),mqttClient.publish(paireTopic, serialNum, false, 2)?"true":"false");
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
        // mqttClient.publish("test", "test", false, 2);
        //  mqttClient.subscribe(topicApp);

        StaticJsonDocument<384> j_topic;
        j_topic["topicApp"] = userId + "/" + serialNum + "/app";
        j_topic["topicEsp"] = userId + "/" + serialNum + "/esp";
        j_topic["topicPms"] = userId + "/" + serialNum + "/pms";
        j_topic["topicTimer"] = userId + "/" + serialNum + "/timer";

        String topicStr;
        serializeJson(j_topic, topicStr);
        writeFile2(LittleFS, "/initial/topic.txt", topicStr.c_str());
        function.refresh("MQTT");
        mqttClient.unsubscribe(paireTopic);
        return true;
    } else
        return false;
}

void onMqttConnect(void *pvParam) {
    String *params = (String *)pvParam;
    String paireTopic = params[0];
    String serialNum = params[1];
    while (1) {
        if (!mqttClient.loop()) {
            // logger.e(mqttClient.lastError());
            logger.e("lastError:%d", mqttClient.lastError());
            // Serial.println(mqttClient.lastError());
            // break;
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

bool isPairing = false;

void MqttCallback(String &topic, String &payload) {
    if (payload != nullptr && payload != "") {
        String mqttMsg = "";
        mqttMsg = payload;
        if (topic == paireTopic && payload.startsWith("userID:") && !isPairing) {
            logger.e("pairing...");
            isPairing = true;
            while (isPaire) {
                vTaskDelay(10);
            };
            pairing("step2", topic, serialNum, mqttMsg);
        }

        if (topic == topicApp) {
            if (mqttMsg == "delete") {
                mqttClient.disconnect();
                function.reset();
            } else
                msgProcess("MQTT", mqttMsg);
        }
        Serial.printf("\ntopic:%s mqttMsg:%s\n", topic.c_str(), mqttMsg.c_str());
        // mqttClient.publish("test", __func__);
    }
    /*if (mqttMsg == "onFlutter") {
        // MqttSendMessage(toAppSystem, "esp32 receive");
        function.refresh("MQTT");
        // MqttSendMessage(toAppSystem, "complete");
    } else
      msgProcess("MQTT", mqttMsg, topic);*/
}

void MqttSendMessage(const String targetTopic, const String payload) {
    if (payload != nullptr || payload != "") {
        Serial.printf(targetTopic.c_str());
        Serial.printf(payload.c_str());
        String msg = "#" + payload;
        // Serial.println(msg);

        mqttClient.publish(targetTopic, msg);
    }
    // mqttClient.publish("test", __func__);

    /*if (target_topic == toAppSystem || target_topic == toAppFunctionFunc)
        mqttClient.publish(target_topic, 2, false, msg.c_str());
    else
        mqttClient.publish(target_topic, 0, false, msg.c_str());*/
}

// WIFI Setting
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void connectToWifi() {
    if (function.SSID != "" && function.PASSWORD != "") {
        Serial.printf("SSID: %s\nPASSWORD: %s", function.SSID, function.PASSWORD);
        WiFi.begin(function.SSID.c_str(), function.PASSWORD.c_str());
        Serial.printf("connect wifi:%d", wifiReconnectCount);
    } else if (!isWifiSmartConfigCreated)
        xTaskCreatePinnedToCore(smartConfig, "smartConfig", 2048, NULL, 1, NULL, 0);
}

void wifiDisconnect(void *pvParam) {
    isWifiDisconnectCreated = true;
    if (wifiReconnectCount < 5)
        vTaskDelay(pdMS_TO_TICKS(3000));
    else
        vTaskDelay(pdMS_TO_TICKS(60000));
    connectToWifi();
    isWifiDisconnectCreated = false;
    vTaskDelete(NULL);
}

void smartConfig(void *pvParam) {
    int smartConfigTime = millis();
    isWifiSmartConfigCreated = true;
    Serial.printf("SmartConfig %s !\n", WiFi.beginSmartConfig() ? "start" : "end");
    while (millis() - smartConfigTime < 120 * 1000) {
        if (WiFi.isConnected()) {
            Serial.println("smartConfigDone");
            isPaire = true;
            break;
        }
        vTaskDelay(100);
    }

    WiFi.stopSmartConfig();
    Serial.println("SmartConfig end!");
    isWifiSmartConfigCreated = false;
    if (!isPaire)
        function.reset();
    vTaskDelete(NULL);
}

/*void OTAServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "The ESP32.");
    });
    AsyncElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    Serial.printf("OTA Server Start!\n");
}*/

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.printf("WiFi connected");
        function.wifiSaveToJson(WiFi.SSID(), WiFi.psk(), WiFi.localIP().toString(), String(WiFi.RSSI()));
        ////////////////////////////////////////////////////////////////////////////////////////////
        HMI.sendMessage("setwifi.wifiInfo.txt=\"" + HMI.escapeJson(function.getWifiInfo()) + "\"");
        HMI.sendMessage("AdminSerial.serialTemp.txt=\"" + HMI.escapeJson(function.getWifiInfo()) + "\"");
        Serial.printf("%s", function.getWifiInfo().c_str());
        connectToMqtt(WiFi.SSID());
        paireTopic = WiFi.SSID();
        // OTAServer();
        wifiReconnectCount = 0;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        if (!isWifiDisconnectCreated) {
            xTaskCreatePinnedToCore(wifiDisconnect, "wifiDisconnect", 8192, NULL, 1, NULL, 0);
            wifiReconnectCount++;
        }
        break;
    }
}

void buttonInterrupt(void *Param) {
    uint16_t buttonState = analogRead(powerSignal);
    bool POWER_ON = false;
    static uint32_t jumpTime = millis();
    static uint32_t currentTime = millis();

    while (1) {
        buttonState = analogRead(powerSignal);
        if (buttonState > 4000) {
            if (millis() - jumpTime > 1000) {
                POWER_ON = !POWER_ON;
                if (POWER_ON) {
                    HMI.sendMessage("sleep=0");
                    HMI.sendMessage("thup=1");
                    digitalWrite(powerSupply, HIGH);
                } else {
                    HMI.sendMessage("thup=0");
                    HMI.sendMessage("sleep=1");
                    if (function.all.state)
                        function.commend("all", "state", "off");
                    if (function.pur.state)
                        function.commend("pur", "state", "off");
                    if (function.fog.state)
                        function.commend("fog", "state", "off");
                    if (function.uvc.state)
                        function.commend("uvc", "state", "off");
                    digitalWrite(powerSupply, LOW);
                }
                jumpTime = millis();
            }
        }

        if (function.all.state || function.pur.state || function.fog.state || function.uvc.state) {
            currentTime = millis();
        }
        if (millis() - currentTime > 300 * 1000 && POWER_ON) {
            POWER_ON = false;
            HMI.sendMessage("sleep=1");
            digitalWrite(powerSupply, LOW);
        }

        vTaskDelay(10);
    }
}

void loadCertFile() {
    if (!initLittleFS()) {
        Serial.printf("initial LittleFS Failed!");
    } else {
        File caFile = LittleFS.open("/emqxsl-ca.crt", "r");
        if (!caFile) {
            Serial.printf("Failed to open file");
        }
        if (secureClient.loadCACert(caFile, caFile.size())) {
            logger.i("CA cert loaded");
        } else {
            logger.e("CA cert load failed");
        }
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(LEDpin, OUTPUT);
    pmsSerial.begin(9600, SERIAL_8E1, 5, 18);
    pms.init();
    ///////////////////////////////////////
    pinMode(powerSignal, INPUT_PULLDOWN);
    pinMode(powerSupply, OUTPUT);
    ///////////////////////////////////////
    WiFi.mode(WIFI_AP_STA);
    WiFi.onEvent(WiFiEvent);

    loadCertFile();
    secureClient.setHandshakeTimeout(2000);
    mqttClient.setOptions(30, true, 2000);
    mqttClient.onMessage(MqttCallback);
    ///////////////////////////////////////
    function.initialWifi();
    connectToWifi();

    HMI.SetCallback(HmiCallback);
    function.setMCUcommender(MCUcommender);
    function.MachineInitial();

    xTaskCreatePinnedToCore(clock, "CLOCK", 4096, NULL, 1, NULL, 0); // 2220 bytes
    xTaskCreatePinnedToCore(dust, "DUST", 4096, NULL, 1, NULL, 0);   // 2384 bytes
    xTaskCreatePinnedToCore(local, "LOCAL", 8192, NULL, 2, NULL, 1); // 5352 bytes
    xTaskCreatePinnedToCore(buttonInterrupt, "buttonTask", 4096, NULL, 1, NULL, 1);
}

String msgBuffer = "";
String testMsg = "#{\" pur.state \":false,\" fog.state \":false,\" uvc.state \":false,\" all.state \":false,\" pur.countState \":true,\" fog.countState \":false,\" uvc.countState \":false,\" all.countState \":false,\" pur.time \":300,\" fog.time \":1800,\" uvc.time \":1800,\" all.time \":1800,\"_modeAuto\":false,\"_modeSleep\":false,\"_modeManua\":true,\" manualDutycycle \":35,\" non \":\" non \"}";
static uint32_t lastMsg = millis();

void loop() {

    while (Serial.available() > 0) {

        char c = Serial.read();
        if (c == '\n') {
            msgBuffer.trim();
            // Serial.printf("Commend: %s\n", msgBuffer);
            if (msgBuffer == "smartConfig") {
                delay(1000);
                if (!isWifiSmartConfigCreated)
                    xTaskCreatePinnedToCore(smartConfig, "smartConfig", 2048, NULL, 1, NULL, 0);
            } else if (msgBuffer == "reset") {
                delay(1000);
                mqttClient.disconnect();
                function.reset();
            } else if (msgBuffer == "subscript") {
                mqttClient.subscribe("test");
            } else if (msgBuffer == "publish") {
                Serial.println(topicEsp);
                mqttClient.publish(topicEsp, testMsg);
            } else if (msgBuffer == "connectMqtt") {
                mqttClient.begin(MQTT_HOST, MQTT_PORT, secureClient);
                while (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
                    Serial.printf(".");
                    vTaskDelay(500);
                }
            } else if (msgBuffer.startsWith("commend:")) {
                msgBuffer.replace("commend:", "");
                msgProcess("HMI", msgBuffer);
            } else if (msgBuffer == "QOS0") {
                messageQueue.addToQueue("testMsg:QoS0");
                /*bool result = mqttClient.publish("test", "testMsg", false, 0);
                logger.w("QoS0:%s", result ? "true" : "false");*/
            } else if (msgBuffer == "QOS1") {
                messageQueue.addToQueue("testMsg:QoS1");
                /*bool result = mqttClient.publish("test", "testMsg", false, 1);
                mqttClient.prepareDuplicate(mqttClient.lastPacketID());
                logger.w("QoS1:%s", result ? "true" : "false");*/
            } else if (msgBuffer == "QOS2") {
                messageQueue.addToQueue("testMsg:QoS2");
                /*bool result = mqttClient.publish("test", "testMsg", false, 2);
                mqttClient.prepareDuplicate(mqttClient.lastPacketID());
                logger.w("QoS2:%s", result ? "true" : "false");*/
            }
            msgBuffer = "";
            break;
        } else
            msgBuffer += c;
    }

    if (millis() - lastMsg > 1000) {
        if (!messageQueue.isQueueEmpty()) {
            String message = messageQueue.getFromQueue();
            bool result = mqttClient.publish("test", message, false, 2);
            logger.w("QoS2:%s", result ? "true" : "false");
        }
        lastMsg = millis();
    }
    // Led.solidColour(BLUE);
    // Led.pulse(BLUE, FAST);
    // Led.circleTwoColours(WHITE, BLUE, FASTEST, FORWARD);
    Led.circleRainbow3(FASTEST, FORWARD);
}