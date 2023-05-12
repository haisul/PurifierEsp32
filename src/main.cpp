#include "HmiConnect.h"
#include "funcControl.h"
#include "littlefsfun.h"
#include "loggerESP.h"
#include "rgbled.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstdlib>
#include <wifiController.h>

wifiController wifi;
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

/*void task(void *pvParameters) {
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
}*/

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

    logger.i("From:%s\nfunc:%s\nset:%s\nstate:%s\n", From.c_str(), func.c_str(), set.c_str(), state.c_str());
    /*if (!isTaskCreated) {
        String *params = new String[5]{From, func, set, state};
        xTaskCreatePinnedToCore(task, "task", 4096, (void *)params, 2, NULL, 0); // 1176 bytes
    }*/
    function.commend(func, set, state);
    if (From != "HMIDev") {
        if (From == "HMI")
            function.refresh("MQTT");
        if (From == "MQTT")
            function.refresh("HMI");
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
        wifi.connect();
    } else if (target == "wifioff") {
        isWifiDisconnectCreated = true;
        WiFi.disconnect(true);
    }
}
String timerStrBuf;
void clock(void *pvParam) {
    int32_t allTimeBuf, purTimeBuf, fogTimeBuf, uvcTimeBuf;
    StaticJsonDocument<384> j_timer;
    String timerStr;
    uint32_t curTime = millis();

    while (1) {
        if (function.all.countStart()) {
            allTimeBuf = function.all.getCountTime();
            if (allTimeBuf < 0) {
                function.commend("all", "state", "off");
            } else {
                HMI.sendMessage("settime.all_countTime.val=" + String(allTimeBuf));
                HMI.sendMessage("AdminDeveloper.n1.val=" + String(allTimeBuf));
            }
        }
        if (function.pur.countStart() && !function.all.getState()) {
            purTimeBuf = function.pur.getCountTime();
            if (purTimeBuf < 0) {
                function.commend("pur", "state", "off");
            } else {
                HMI.sendMessage("settime.pur_countTime.val=" + String(purTimeBuf));
                HMI.sendMessage("AdminDeveloper.n2.val=" + String(purTimeBuf));
            }
        }
        if (function.fog.countStart() && !function.all.getState()) {
            fogTimeBuf = function.fog.getCountTime();
            if (fogTimeBuf < 0) {
                function.commend("fog", "state", "off");

            } else {
                HMI.sendMessage("settime.fog_countTime.val=" + String(fogTimeBuf));
                HMI.sendMessage("AdminDeveloper.n3.val=" + String(fogTimeBuf));
            }
        }
        if (function.uvc.countStart() && !function.all.getState()) {
            uvcTimeBuf = function.uvc.getCountTime();
            if (uvcTimeBuf < 0) {
                function.commend("uvc", "state", "off");

            } else {
                HMI.sendMessage("settime.uvc_countTime.val=" + String(uvcTimeBuf));
                HMI.sendMessage("AdminDeveloper.n4.val=" + String(uvcTimeBuf));
            }
        }
        if (function.MachineCountStart() && millis() - curTime >= 30000) {
            j_timer["allCountTime"] = allTimeBuf;
            j_timer["purCountTime"] = purTimeBuf;
            j_timer["fogCountTime"] = fogTimeBuf;
            j_timer["uvcCountTime"] = uvcTimeBuf;

            serializeJson(j_timer, timerStr);
            mqttClient.sendMessage(mqttClient.getTopicTimer(), timerStr, 0);
            logger.i(timerStr.c_str());
            timerStrBuf = timerStr;
            timerStr = "";
            curTime = millis();
        }

        vTaskDelay(1000);
    }
}
String pmsStrBuf;
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
        else
            dustValStr = (uint16_t)dustValAvg;

        tempValStr = String(pms.n5p0 / 10);
        rhumValStr = String(pms.n10p0 / 10) + "." + String((pms.n10p0 - (pms.n10p0 / 10) * 10));

        //if (abs(dustValAvg - dustValTemp) / dustValTemp >= 0.1) {
            function.pur.setDust((uint16_t)dustValAvg);

            HMI.sendMessage("home.pm25.txt=\"" + dustValStr + "\"");
            HMI.sendMessage("home.temp.txt=\"" + tempValStr + "\"");
            HMI.sendMessage("home.rhum.txt=\"" + rhumValStr + "\"");

            j_pms["pm25"] = rhumValStr;
            j_pms["temp"] = tempValStr;
            j_pms["rhum"] = rhumValStr;

            serializeJson(j_pms, pmsStr);
            mqttClient.sendMessage(mqttClient.getTopicPms(), pmsStr, 0);
            pmsStrBuf = pmsStr;
            pmsStr = "";
        //}
        dustValTemp = (uint16_t)dustValAvg;
        vTaskDelay(1000);
    }
}

void local(void *pvParam) {
    vTaskDelay(1000);
    function.commend("all", "state", "off");
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
                function.MachineReset();
            } else {
                msgProcess("MQTT", mqttMsg);
                logger.w("Topic:%s\nMsg:%s", topic.c_str(), mqttMsg.c_str());
            }
        }
    }
}

// WIFI Setting
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void onWifiConnected() {
    mqttClient.connectToMqtt(WiFi.SSID(), serialNum);
    HMI.sendMessage("setwifi.wifiInfo.txt=\"" + HMI.escapeJson(wifi.getInfo()) + "\"");
    HMI.sendMessage("AdminSerial.serialTemp.txt=\"" + HMI.escapeJson(wifi.getInfo()) + "\"");
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

/*void OTAServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "The ESP32.");
    });
    AsyncElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    Serial.printf("OTA Server Start!\n");
}*/

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
                    function.commend("all", "state", "off");
                    digitalWrite(powerSupply, LOW);
                }
                jumpTime = millis();
            }
        }

        if (function.MachineState()) {
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
    Serial.begin(115200);

    pinMode(LEDpin, OUTPUT);
    pmsSerial.begin(9600, SERIAL_8E1, 5, 18);
    pms.init();
    ///////////////////////////////////////

    pinMode(powerSignal, INPUT_PULLDOWN);
    pinMode(powerSupply, OUTPUT);
    ///////////////////////////////////////
    wifi.setWiFiConnectedCallback(onWifiConnected);
    wifi.connect();

    mqttClient.mqttCallback(mqttCallback);
    ///////////////////////////////////////

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
            if (msgBuffer == "reset") {
                delay(1000);
                function.MachineReset();
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
            } else if (msgBuffer == "print") {
                logger.i("The quick brown fox jumps over the lazy dog.\nThis is a well-known sentence that is often used to demonstrate typing or font styles. However, it is more than just a typing exercise. The sentence contains every letter of the English alphabet, making it a pangram. Pangrams are a fun way to test your typing skills, and they can also be used in design and typography to showcase different typefaces. In addition to pangrams, there are many other interesting linguistic phenomena in the English language, such as palindromes, anagrams, and tongue twisters");
            } else if (msgBuffer == "json") {
                mqttClient.sendMessage(mqttClient.getTopicPms(), pmsStrBuf, 0);
                mqttClient.sendMessage(mqttClient.getTopicTimer(), timerStrBuf, 0);
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