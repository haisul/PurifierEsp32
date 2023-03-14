#include "HmiConnect.h"
#include "funcControl.h"
#include <Arduino.h>
#include <WiFi.h>
/////////////////////////////////////////////
#include <AsyncMqttClient.h>
#include <Ticker.h>
AsyncMqttClient mqttClient;
TimerHandle_t mqttTimer;
// TimerHandle_t wifiTimer;
int wifiReconnectCount = 0;
int mqttReconnectCount = 0;
bool isWifiDisconnectCreated = false;
bool isWifiSmartConfigCreated = false;
//////////////////////////////////////////////
#include <AsyncElegantOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);
//////////////////////////////////////////////
#include <PMserial.h>
HardwareSerial pmsSerial(1);
SerialPM pms(PMS5003, 5, 18);
//////////////////////////////////////////////
#define LEDpin 2
//////////////////////////////////////////////
HmiConnect HMI(Serial2, 9600);
funcControl function;
//////////////////////////////////////////////
bool isTaskCreated = false;
const char *ssid = "";
const char *pass = "";

#define MQTT_HOST "broker.emqx.io"
#define MQTT_PORT 1883
#define MQTT_USERNAME "LJ"
#define MQTT_PASSWORD "33456789"

const char *toAppFunction = "LJ/ex2252/EspCommend/Function";
const char *toAppFunctionSensor = "LJ/ex2252/EspCommend/Function/Seneor";
const char *toAppFunctionCount = "LJ/ex2252/EspCommend/Function/Count";
const char *toAppFunctionFunc = "LJ/ex2252/EspCommend/Function/Func";
const char *toAppSystem = "LJ/ex2252/EspCommend/System";
const char *fromAppFunction = "LJ/ex2252/AppCommend/Function";
const char *fromAppSystem = "LJ/ex2252/AppCommend/System";

void smartConfig(void *pvParam);
void MqttSendMessage(const char *, String);
void connectToWifi();

void task(void *pvParameters) {
    isTaskCreated = true;
    String From = ((String *)pvParameters)[0];
    String func = ((String *)pvParameters)[1];
    String set = ((String *)pvParameters)[2];
    String state = ((String *)pvParameters)[3];
    String topic = ((String *)pvParameters)[4];

    function.commend(func, set, state);
    if (From != "HMIDev") {
        if (From == "HMI" || topic == "LJ/test")
            function.refresh("MQTT");
        if (From == "MQTT")
            function.refresh("HMI");
    }
    delete[] ((String *)pvParameters);
    isTaskCreated = false;
    vTaskDelete(NULL);
}

void msgProcess(String From, String msg, String topic = "") {
    Serial.printf("\n");
    int index1 = msg.indexOf('_');
    int index2 = msg.indexOf(':');
    String func = msg.substring(0, index1), set = "", state = "";

    if (index2 >= 0) {
        set = msg.substring(index1 + 1, index2);
        state = msg.substring(index2 + 1);
    } else {
        set = msg.substring(index1 + 1);
    }

    Serial.printf("From:%s | func:%s | set:%s | state:%s\n", From.c_str(), func.c_str(), set.c_str(), state.c_str());
    if (!isTaskCreated) {
        String *params = new String[5]{From, func, set, state, topic};
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
    Serial.printf("target: %s\n", target);
    if (target == "HMI" || target == "") {
        String temp = "\"" + HMI.escapeJson(function.getInitialJson()) + "\"";
        HMI.sendMessage("AdminJson.Json.txt=" + temp);
    }
    if (target == "MQTT" || target == "")
        MqttSendMessage(toAppFunctionFunc, function.getInitialJson());

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
    String timeBuff;
    while (1) {
        if (function.all.startingCount) {
            timeBuff = String(function.all.endEpoch - function.rtc.getEpoch());
            HMI.sendMessage("settime.all_countTime.val=" + timeBuff);
            HMI.sendMessage("AdminDeveloper.n0.val=" + timeBuff);
            MqttSendMessage(toAppFunctionCount, "all_countTime:" + timeBuff);
            if (timeBuff == "0")
                function.funcState(ALL, OFF, true);
        }
        if (function.pur.startingCount && !function.all.startingCount) {
            timeBuff = String(function.pur.endEpoch - function.rtc.getEpoch());
            HMI.sendMessage("settime.pur_countTime.val=" + timeBuff);
            HMI.sendMessage("AdminDeveloper.n1.val=" + timeBuff);
            MqttSendMessage(toAppFunctionCount, "pur_countTime:" + timeBuff);
            if (timeBuff == "0")
                function.funcState(PUR, OFF, true);
        }
        if (function.fog.startingCount && !function.all.startingCount) {
            timeBuff = String(function.fog.endEpoch - function.rtc.getEpoch());
            HMI.sendMessage("settime.fog_countTime.val=" + timeBuff);
            HMI.sendMessage("AdminDeveloper.n3.val=" + timeBuff);
            MqttSendMessage(toAppFunctionCount, "fog_countTime:" + timeBuff);
            if (timeBuff == "0")
                function.funcState(FOG, OFF, true);
        }
        if (function.uvc.startingCount && !function.all.startingCount) {
            timeBuff = String(function.uvc.endEpoch - function.rtc.getEpoch());
            HMI.sendMessage("settime.uvc_countTime.val=" + timeBuff);
            HMI.sendMessage("AdminDeveloper.n4.val=" + timeBuff);
            MqttSendMessage(toAppFunctionCount, "uvc_countTime:" + timeBuff);
            if (timeBuff == "0")
                function.funcState(UVC, OFF, true);
        }
        vTaskDelay(1000);
    }
}

void dust(void *pvParam) {
    String dustVal = "0", tempVal = "0", rhumVal = "0";
    while (1) {
        pms.read();
        dustVal = pms.pm25;
        if (pms.pm25 >= 999)
            dustVal = "999";
        else {
            dustVal = pms.pm25;
            if (function._modeAuto)
                function.getDustVal(pms.pm25);
        }
        tempVal = String(pms.n5p0 / 10);
        rhumVal = String(pms.n10p0 / 10) + "." + String((pms.n10p0 - (pms.n10p0 / 10) * 10));

        if (pms.pm25 > 0) {
            HMI.sendMessage("home.pm25.txt=\"" + dustVal + "\"");
            HMI.sendMessage("home.temp.txt=\"" + tempVal + "\"");
            HMI.sendMessage("home.rhum.txt=\"" + rhumVal + "\"");
            MqttSendMessage(toAppFunctionSensor, "pms_dustVal:" + dustVal);
            MqttSendMessage(toAppFunctionSensor, "pms_tempVal:" + tempVal);
            MqttSendMessage(toAppFunctionSensor, "pms_rhumVal:" + rhumVal);
        }
        vTaskDelay(3000);
    }
}

void local(void *pvParam) {
    vTaskDelay(2000);
    function_mode_types func[4] = {ALL, PUR, FOG, UVC};
    for (int i = 0; i < 4; i++)
        function.funcState(func[i], OFF);
    while (1) {
        HMI.loop();
        vTaskDelay(50);
    }
}

// MQTT Setting
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void connectToMqtt() {
    Serial.println("Connecting to MQTT...");
    mqttClient.connect();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.println("Disconnected from MQTT.");
    if (WiFi.isConnected()) {
        xTimerStart(mqttTimer, 0);
    }
}

void onMqttConnect(bool sessionPresent) {
    Serial.printf("Connected to MQTT.\n");
    mqttClient.subscribe(fromAppFunction, 2);
    mqttClient.subscribe(fromAppSystem, 2);
    mqttClient.subscribe("LJ/test", 2);
    MqttSendMessage(toAppSystem, "onESP32");
    MqttSendMessage(toAppFunctionFunc, function.getInitialJson());
    mqttReconnectCount = 0;
}

void MqttCallback(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    String mqtt_msg = "";
    for (int i = 0; i < len; i++) {
        mqtt_msg += (char)payload[i];
    }
    if (mqtt_msg == "onFlutter") {
        MqttSendMessage(toAppSystem, "esp32 receive");
        function.refresh("MQTT");
        MqttSendMessage(toAppSystem, "complete");
    } else
        msgProcess("MQTT", mqtt_msg, topic);
}

void MqttSendMessage(const char *target_topic, String msg) {
    msg = "#" + msg;
    if (target_topic == toAppSystem || target_topic == toAppFunctionFunc)
        mqttClient.publish(target_topic, 2, false, msg.c_str());
    else
        mqttClient.publish(target_topic, 0, false, msg.c_str());
}

// WIFI Setting
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void connectToWifi() {
    if (function.SSID != "" && function.PASSWORD != "") {
        Serial.printf("SSID: %s  PASSWORD: %s\n", function.SSID, function.PASSWORD);
        WiFi.begin(function.SSID.c_str(), function.PASSWORD.c_str());
        Serial.printf("connect wifi:%d\n", wifiReconnectCount);
    } else if (!isWifiSmartConfigCreated)
        xTaskCreatePinnedToCore(smartConfig, "smartConfig", 1024, NULL, 1, NULL, 0);
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
    isWifiSmartConfigCreated = true;
    WiFi.beginSmartConfig();
    vTaskDelay(120 * 1000);
    WiFi.stopSmartConfig();
    isWifiSmartConfigCreated = false;
    vTaskDelete(NULL);
}

void OTAServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "The ESP32.");
    });
    AsyncElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    Serial.printf("OTA Server Start!\n");
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("\n[WiFi-event] event: %d\n", event);
    switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        function.wifiSaveToJson(WiFi.SSID(), WiFi.psk(), WiFi.localIP().toString(), String(WiFi.RSSI()));
        ////////////////////////////////////////////////////////////////////////////////////////////
        HMI.sendMessage("setwifi.wifiInfo.txt=\"" + HMI.escapeJson(function.getWifiInfo()) + "\"");
        HMI.sendMessage("AdminSerial.serialTemp.txt=\"" + HMI.escapeJson(function.getWifiInfo()) + "\"");
        Serial.printf("%s\n", function.getWifiInfo().c_str());
        connectToMqtt();
        OTAServer();
        WiFi.stopSmartConfig();
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

void setup() {
    Serial.begin(115200);
    pinMode(LEDpin, OUTPUT);
    pmsSerial.begin(9600, SERIAL_8E1, 5, 18);
    pms.init();
    ///////////////////////////////////////
    WiFi.mode(WIFI_AP_STA);
    WiFi.onEvent(WiFiEvent);
    // WiFi.beginSmartConfig();
    mqttTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(4000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
    // wifiTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(4000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

    mqttClient.setCredentials(MQTT_USERNAME, MQTT_PASSWORD);
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
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
}

void loop() {
}