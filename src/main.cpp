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

LoggerESP logger;
rgbLed Led;
/////////////////////////////////////////////
String chipid = String(ESP.getEfuseMac());
const String serialNum = "serialNum:$iep" + chipid;
/////////////////////////////////////////////
#include "QueueMqttClient.h"
#include <Ticker.h>
QueueMQTTClient mqttClient;
uint8_t wifiReconnectCount = 0;
bool isWifiDisconnectCreated = false;
bool isWifiSmartConfigCreated = false;
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

void smartConfig(void *pvParam);
void connectToWifi();

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
        mqttClient.sendMessage(mqttClient.getTopicEsp(), function.getInitialJson(), 0);

    if (target == "wifion" || target == "wifiInfo") {
        isWifiDisconnectCreated = false;
        connectToWifi();
    } else if (target == "wifioff") {
        isWifiDisconnectCreated = true;
        WiFi.disconnect(true);
    } else if (target == "wifiConfig") {
        if (!isWifiSmartConfigCreated)
            xTaskCreatePinnedToCore(smartConfig, "smartConfig", 3000, NULL, 1, NULL, 0);
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
            mqttClient.sendMessage(mqttClient.getTopicTimer(), timerStr, 0);
            logger.i(timerStr.c_str());
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
            mqttClient.sendMessage(mqttClient.getTopicPms(), pmsStr, 0);
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

void mqttCallback(String &topic, String &payload) {
    if (payload != nullptr && payload != "") {
        String mqttMsg = payload;
        if (topic == mqttClient.getTopicApp()) {
            if (mqttMsg == "delete") {
                // mqttClient.disconnect();
                function.reset();
            } else
                // msgProcess("MQTT", mqttMsg);
                logger.w("Topic:%s\n Msg:%s", topic.c_str(), mqttMsg.c_str());
        }
    }
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
        xTaskCreatePinnedToCore(smartConfig, "smartConfig", 3000, NULL, 1, NULL, 0);
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
            break;
        }
        vTaskDelay(100);
    }

    WiFi.stopSmartConfig();
    Serial.println("SmartConfig end!");
    isWifiSmartConfigCreated = false;
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
        mqttClient.connectToMqtt(WiFi.SSID(), serialNum);
        //  paireTopic = WiFi.SSID();
        //   OTAServer();
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

void setup() {
    // Serial.begin(115200);
    pinMode(LEDpin, OUTPUT);
    pmsSerial.begin(9600, SERIAL_8E1, 5, 18);
    pms.init();
    ///////////////////////////////////////
    pinMode(powerSignal, INPUT_PULLDOWN);
    pinMode(powerSupply, OUTPUT);
    ///////////////////////////////////////
    WiFi.mode(WIFI_AP_STA);
    WiFi.onEvent(WiFiEvent);

    mqttClient.mqttCallback(mqttCallback);
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
void loop() {

    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\n') {
            msgBuffer.trim();
            if (msgBuffer == "smartConfig") {
                delay(1000);
                if (!isWifiSmartConfigCreated)
                    xTaskCreatePinnedToCore(smartConfig, "smartConfig", 3000, NULL, 1, NULL, 0);
            } else if (msgBuffer == "reset") {
                delay(1000);
                function.reset();
            } else if (msgBuffer.startsWith("commend:")) {
                msgBuffer.replace("commend:", "");
                msgProcess("HMI", msgBuffer);
            } else if (msgBuffer == "QOS0") {
                mqttClient.sendMessage("test", "testMsg:QoS0", 0);
            } else if (msgBuffer == "QOS1") {
                mqttClient.sendMessage("test", "testMsg:QoS1", 1);
            } else if (msgBuffer == "QOS2") {
                mqttClient.sendMessage("test", "testMsg:QoS2", 2);
            } else if (msgBuffer.startsWith("!")) {
                msgBuffer.remove(0, 1);
                HmiCallback(msgBuffer);
            }
            msgBuffer = "";
            break;
        } else
            msgBuffer += c;
    }
    // Led.solidColour(BLUE);
    // Led.pulse(BLUE, FAST);
    // Led.circleTwoColours(WHITE, BLUE, FASTEST, FORWARD);
    Led.circleRainbow3(MEDIUM, FORWARD);
}