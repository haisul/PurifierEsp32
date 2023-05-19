#include "HmiConnect.h"
#include "funcControl.h"
#include "littlefsfun.h"
#include "loggerESP.h"
#include "rgbled.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstdlib>
#include <esp_intr_alloc.h>
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
// AsyncWebServer server(80);
//////////////////////////////////////////////
#include <PMserial.h>
HardwareSerial pmsSerial(1);
SerialPM pms(PMS5003, 5, 18);
//////////////////////////////////////////////
#define LEDpin 2
//////////////////////////////////////////////

static unsigned long lastInterruptTime = millis();
bool enableIntterrupt = false;

const int buttonPin = 34;
volatile bool buttonState = false;
volatile bool buttonPressed = false;
unsigned long buttonStartTime = 0;

const uint8_t powerSupply = 25;
//////////////////////////////////////////////
HmiConnect HMI(Serial2, 9600);
funcControl function;
//////////////////////////////////////////////
bool isTaskCreated = false;

int LedColor = 0;

bool isMsgProcessRunning = false;

bool machineRunning = false;

void buttonInterruptHandler();

void msgProcess(String From, String msg) {
    isMsgProcessRunning = true;
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

    function.commend(func, set, state);
    if (From != "HMIDev") {
        if (From == "HMI")
            function.refresh("MQTT");
        if (From == "MQTT")
            function.refresh("HMI");
    }
    isMsgProcessRunning = false;
}

void HmiCallback(String HMI_msg) {
    String From = "HMI";
    if (HMI_msg.startsWith("!")) {
        HMI_msg.remove(0, 1);
        From = "HMIDev";
    } else if (HMI_msg == "refresh") {
        function.refresh("HMI");
    }
    if (!isMsgProcessRunning)
        msgProcess(From, HMI_msg);
}

// local Setting
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void MCUcommender(String target) {
    Serial.printf("target: %s", target);
    if (target == "HMI" || target == "") {
        HMI.sendMessage("c" + function.getInitialJson());
    }

    if (target == "MQTT" || target == "")
        mqttClient.sendMessage(mqttClient.getTopicEsp(), function.getInitialJson(), 0);
}

void clock(void *pvParam) {
    int32_t allTimeBuf = function.all.getTime();
    int32_t purTimeBuf = function.pur.getTime();
    int32_t fogTimeBuf = function.fog.getTime();
    int32_t uvcTimeBuf = function.uvc.getTime();
    StaticJsonDocument<384> j_timer;
    String timerStr;
    uint32_t curTime = millis();

    while (1) {
        if (machineRunning) {
            if (function.all.countStart()) {
                allTimeBuf = function.all.getCountTime();
                if (allTimeBuf <= 0) {
                    function.commend("all", "state", "off");
                    allTimeBuf = function.all.getTime();
                }
            }
            if (function.pur.countStart() && !function.all.getState()) {
                purTimeBuf = function.pur.getCountTime();
                if (purTimeBuf <= 0) {
                    function.commend("pur", "state", "off");
                    purTimeBuf = function.pur.getTime();
                }
            }
            if (function.fog.countStart() && !function.all.getState()) {
                fogTimeBuf = function.fog.getCountTime();
                if (fogTimeBuf <= 0) {
                    function.commend("fog", "state", "off");
                    fogTimeBuf = function.fog.getTime();
                }
            }
            if (function.uvc.countStart() && !function.all.getState()) {
                uvcTimeBuf = function.uvc.getCountTime();
                if (uvcTimeBuf <= 0) {
                    function.commend("uvc", "state", "off");
                    uvcTimeBuf = function.uvc.getTime();
                }
            }
            if (function.MachineCountStart() && millis() - curTime >= 30000) {
                j_timer["allCountTime"] = allTimeBuf;
                j_timer["purCountTime"] = purTimeBuf;
                j_timer["fogCountTime"] = fogTimeBuf;
                j_timer["uvcCountTime"] = uvcTimeBuf;

                serializeJson(j_timer, timerStr);
                mqttClient.sendMessage(mqttClient.getTopicTimer(), timerStr, 0);
                HMI.sendMessage("t" + timerStr);
                logger.i(timerStr.c_str());
                timerStr = "";
                curTime = millis();
            }
        }
        vTaskDelay(1000);
    }
}

void dust(void *pvParam) {
    String dustValStr = "0", tempValStr = "0", rhumValStr = "0";
    double dustValAvg = 0;
    uint16_t dustValTemp = 0;
    StaticJsonDocument<384> j_pms;
    String pmsStr;
    while (1) {
        if (machineRunning) {
            pms.read();
            dustValStr = pms.pm25;
            dustValAvg = (pms.pm25 + dustValTemp) / 2;

            if (dustValAvg >= 999)
                dustValStr = "999";
            else
                dustValStr = (uint16_t)dustValAvg;

            tempValStr = String(pms.n5p0 / 10);
            rhumValStr = String(pms.n10p0 / 10) + "." + String((pms.n10p0 - (pms.n10p0 / 10) * 10));
            if (dustValAvg < 50 && dustValAvg > 0)
                LedColor = 1;
            else if (dustValAvg < 150 && dustValAvg > 50)
                LedColor = 2;
            else if (dustValAvg > 150)
                LedColor = 3;

            if (abs(dustValAvg - dustValTemp) / dustValTemp >= 0.1) {

                function.pur.setDust((uint16_t)dustValAvg);

                j_pms["pm25"] = dustValStr;
                j_pms["temp"] = tempValStr;
                j_pms["rhum"] = rhumValStr;

                serializeJson(j_pms, pmsStr);
                mqttClient.sendMessage(mqttClient.getTopicPms(), pmsStr, 0);
                HMI.sendMessage("p" + pmsStr);
                pmsStr = "";
            }
            dustValTemp = (uint16_t)dustValAvg;
        }
        vTaskDelay(1000);
    }
}

void local(void *pvParam) {
    vTaskDelay(1000);
    function.commend("all", "state", "off");
    while (1) {
        HMI.loop();

        if (buttonPressed && digitalRead(buttonPin) == HIGH) {
            if (millis() - lastInterruptTime > 2000) {
                buttonState = !buttonState;
                buttonStartTime = millis();
                if (buttonState) {
                    digitalWrite(powerSupply, HIGH);
                    machineRunning = true;
                    logger.e("turnOn");
                } else if (!buttonState) {
                    function.commend("all", "state", "off");
                    digitalWrite(powerSupply, LOW);
                    logger.e("shutDown");
                    LedColor = 0;
                    machineRunning = false;
                }
                buttonPressed = false;
                enableIntterrupt = true;
            }
        } else if (buttonPressed && digitalRead(buttonPin) == LOW) {
            buttonPressed = false;
            attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterruptHandler, RISING);
        }

        if (millis() - buttonStartTime > 10000 && enableIntterrupt) {
            attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterruptHandler, RISING);
            enableIntterrupt = false;
        }

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
            } else if (!isMsgProcessRunning) {
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
    HMI.sendMessage("w" + wifi.getInfo());
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void buttonInterruptHandler() {
    detachInterrupt(digitalPinToInterrupt(buttonPin));
    lastInterruptTime = millis();
    buttonPressed = true;
}

void loop();

void setup() {
    Serial.begin(115200);

    pinMode(LEDpin, OUTPUT);
    pmsSerial.begin(9600, SERIAL_8E1, 5, 18);
    pms.init();
    ///////////////////////////////////////
    pinMode(powerSupply, OUTPUT);
    pinMode(buttonPin, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterruptHandler, RISING);

    ///////////////////////////////////////
    wifi.setWiFiConnectedCallback(onWifiConnected);
    wifi.connect();

    mqttClient.mqttCallback(mqttCallback);
    ///////////////////////////////////////

    HMI.SetCallback(HmiCallback);
    function.setMCUcommender(MCUcommender);
    function.MachineInitial();

    // OTAServer();

    xTaskCreatePinnedToCore(clock, "CLOCK", 4096, NULL, 1, NULL, 0); // 2220 bytes
    xTaskCreatePinnedToCore(dust, "DUST", 4096, NULL, 1, NULL, 0);   // 2384 bytes
    xTaskCreatePinnedToCore(local, "LOCAL", 8192, NULL, 2, NULL, 1); // 5352 bytes
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
            } else if (msgBuffer.startsWith("?")) {
                msgBuffer.remove(0, 1);
                HMI.sendMessage(msgBuffer);
                HMI.sendMessage("c" + function.getInitialJson());
            }
            msgBuffer = "";
            break;
        } else
            msgBuffer += c;
    }

    switch (LedColor) {
    case 0:
        Led.pulse(BLACK, FASTEST);
        break;
    case 1:
        Led.pulse(GREEN, FASTEST);
        break;
    case 2:
        Led.pulse(YELLOW, FASTEST);
        break;
    case 3:
        Led.pulse(RED, FASTEST);
        break;

    default:
        break;
    }

    // Led.solidColour(BLUE);
    // Led.pulse(BLUE, FASTEST);
    // Led.circleTwoColours(WHITE, BLUE, FASTEST, FORWARD);
    // Led.circleRainbow3(MEDIUM, FORWARD);
}