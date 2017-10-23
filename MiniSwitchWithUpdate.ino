/*
  To upload through terminal you can use: curl -F "image=@firmware.bin" esp8266-webupdate.local/update
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <MQTT.h>
#include <EEPROM.h>

#include "WemoSwitch.h"
#include "WemoManager.h"
#include "CallbackFunction.h"

#define DEBUG 1

#define CONFIG_VERSION "v01"

// prototypes
boolean connectWifi();
void wemoCB(uint8_t pin, uint8_t level);
void allOn();
void allOff();

const char* host = "esp8266-webupdate";
//const char* ssid = "WLAN1_Guest1";
//const char* password = "Locoto100";
#define AP_SSID "WLAN1_Guest1"
#define AP_PASSWD "Locoto100"

const char* eiocloud_username = "rsalama";
const char* eiocloud_passwd = "Locoto100";

const char* eiocloud_host = "cloud.iot-playground.com";
const int eiocloud_port = 1883;
// MQTT myMqtt("", eiocloud_host, eiocloud_port);

WemoManager wemoManager;

#define NUM_MODULES 6

const int relayPin1 = 4;
const int relayPin2 = 5;
const int ledPin = 2; //BUILTIN_LED;
const int redLED = 16;
const int yellowLED = 14;
const int greenLED = 12;

struct StoreStruct {
  char version[4];
  uint moduleId;
  bool state;
  char ssid[20];
  char passwd[20];
} storage = {
  CONFIG_VERSION,
  2,
  1, // off;
  AP_SSID,
  AP_PASSWD
};

bool stepOk = false;
int buttonState;

boolean result;
String topic("");
String valueStr("");

boolean switchState;
const int buttonPin = 0;  
const int outPin = 2;  
int lastButtonState = LOW; 

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

struct RelayInfo {
	String name;
	int pin;
	uint port;
	boolean reverse;
};

#define NUM_RELAYS 7
RelayInfo relays[NUM_RELAYS] = {
	{ String("office lights"), relayPin1, 90, false },
	{ String("kitchen lights"), relayPin2, 91, false },
	{ String("coffee light"), ledPin, 92, true },
	{ String("red light"), redLED, 93, false },
	{ String("yellow light"), yellowLED, 94, false },
	{ String("green light"), greenLED, 95, false },
	{ String("all light"), -1, 96, false }
};

void setup(void){
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(AP_SSID, AP_PASSWD);

  while(WiFi.waitForConnectResult() != WL_CONNECTED){
    WiFi.begin(AP_SSID, AP_PASSWD);
    Serial.println("WiFi failed, retrying.");
  }

  MDNS.begin(host);

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  IPAddress ip = WiFi.localIP();
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\r\n", host);
  Serial.print("IP address: ");
  Serial.println(ip);

  String clientName;
  uint8_t mac[6];
  clientName += macToStr((unsigned char *) mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
  Serial.print("MQTT client id:");
  Serial.println(clientName);

  /*
  myMqtt.setClientId(clientName.c_str());


  // setup callbacks
  myMqtt.onConnected(myConnectedCb);
  myMqtt.onDisconnected(myDisconnectedCb);
  myMqtt.onPublished(myPublishedCb);
  myMqtt.onData(myDataCb);
  
  //////Serial.println("connect mqtt...");
  myMqtt.setUserPwd(eiocloud_username, eiocloud_passwd);  
  myMqtt.connect();

  delay(500);

  Serial.println("Suscribe: /"+String(storage.moduleId)+ "/Sensor.Parameter1"); 
  myMqtt.subscribe("/"+String(storage.moduleId)+ "/Sensor.Parameter1");
  */
  
  wemoManager.begin();
  // Format: Alexa invocation name, pin, local port no, callback, reverse (HIGH and LOW)
  for (int i = 0; i < NUM_RELAYS; i++) {
	  wemoManager.addDevice(new WemoSwitch(relays[i].name, relays[i].pin, relays[i].port, wemoCB));
	  pinMode(relays[i].pin, OUTPUT);
  }
  delay(10);
  digitalWrite(ledPin, HIGH); // Wemos BUILTIN_LED is active Low, so high is off
  Serial.println("Finished setup()");
}

void loop(void) {
  httpServer.handleClient();
  wemoManager.serverLoop();
}

void wemoCB(uint8_t pin, uint8_t level) {
    Serial.printf("Switch %d turn %d ...\r\n", pin, level);
    digitalWrite(pin, level);
}
/*
void allOn() {
  Serial.print("all lights on ...");
  officeLightsOn();
  kitchenLightsOn();
  ledOn();
  redOn();
  yellowOn();
  greenOn();
}

void allOff() {
  Serial.print("all lights off ...");
  officeLightsOff();
  kitchenLightsOff();
  ledOff();
  redOff();
  yellowOff();
  greenOff();
}
*/

void changeStatus(int ind,int newStatus) {
    Serial.printf("Changing status ind %d new status %d", ind, newStatus);
    Serial.println();
    switch(ind){
      case 1:
        digitalWrite(relayPin1, newStatus);
        break;
      case 2:
        digitalWrite(relayPin2, newStatus);
        break;
      case 3:
        digitalWrite(ledPin, newStatus);
        break;
      case 4:
        digitalWrite(redLED, newStatus);
        break;
      case 5:
        digitalWrite(yellowLED, newStatus);
        break;
      case 6:
        digitalWrite(greenLED, newStatus);
        break;
      case 7:
        //if (newStatus == 0) allOff();
        //else allOn();
        break;
      }  
}

String macToStr(uint8_t* mac) {
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

/***
void myConnectedCb() {
#ifdef DEBUG
  Serial.println("connected to MQTT server");
#endif
  if (storage.moduleId != 0)
    myMqtt.subscribe("/" + String(storage.moduleId) + "/Sensor.Parameter1");
}

void myDisconnectedCb() {
#ifdef DEBUG
  Serial.println("disconnected. try to reconnect...");
#endif
  delay(500);
  myMqtt.connect();
}

void myPublishedCb() {
#ifdef DEBUG  
  Serial.println("published.");
#endif
}

void myDataCb(String& topic, String& data) {  
#ifdef DEBUG  
  Serial.printf("Received topic: %s data: %s\r\n", topic.c_str(), data.c_str());
#endif
  if (topic ==  String("/NewModule"))
  {
    storage.moduleId = data.toInt();
    stepOk = true;
  }
  else if (topic == String("/"+String(storage.moduleId)+ "/Sensor.Parameter1/NewParameter"))
  {
    stepOk = true;
  }
  else if (topic == String("/"+String(storage.moduleId)+ "/Sensor.Parameter1"))
  {
    switchState = (data == String("1"))? true: false;
    changeStatus(3, switchState ? 1 : 0);
#ifdef DEBUG      
    Serial.printf("switch state received: %s\r\n", switchState);
#endif
  }
}
***/


