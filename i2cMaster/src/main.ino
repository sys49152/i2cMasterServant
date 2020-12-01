/*
header files for OTA
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

String myMacStr = "";
String hostName;

// Replace with your network credentials
#include "credential.h"

// header files for I2C
//
#include <Arduino.h>
#include <Wire.h>
const int nanoID = 4; // i2c address of servant

// Blynk lib.
/* Comment this out to disable prints and save space */
// #define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

WidgetLCD lcd(V0);
WidgetLED led1(V10);
WidgetLED led2(V11);
WidgetTerminal terminal(V20);

// timer lib.
#include <elapsedMillis.h>

// PubSubClient MQTT
#include <PubSubClient.h>
const char* MQTTserver = "192.168.178.3";
WiFiClient espClient;
PubSubClient MQTTclient(espClient);

void setup() {
  // LED status shows we are in setup
  pinMode(LED_BUILTIN, OUTPUT);
  // inverted logic LOW means LED on
  digitalWrite(LED_BUILTIN, LOW); // LOW all the way through setup, then HIGH

  Serial.begin(115200);
  Serial.println("\nBooting");
  //set hostname
  byte myMAC[6];
  WiFi.macAddress(myMAC);
  for (int i=0; i < 6; i++) {
    if(myMAC[i] < (byte) 16) myMacStr += "0"; // minimum 2 HEX digits, if not add leading 0
    myMacStr += String(myMAC[i], HEX);
    //Serial.print(i);
    //Serial.println(" "+myMacStr);
  }
  myMacStr.toLowerCase();
  hostName = "expirementalWemosD1mini-" + myMacStr.substring(6);
  Serial.println("hostname: "+hostName);
  ArduinoOTA.setHostname(hostName.c_str()); // for OTA
 
  WiFi.hostname(hostName.c_str());  // set hostname DHCP client

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if(WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start flashing new firmware.");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd flashing new firmware.");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // connect to local Blynk server
  Blynk.config(auth, IPAddress(192, 168, 178, 3), 8080);
  // clear the terminal content
  terminal.clear();

  // init. I2C
  Wire.begin(); // start I2C master

  // init MQTT
  MQTTclient.setServer(MQTTserver, 1883);
  MQTTclient.setCallback(MQTTcallback);

  digitalWrite(LED_BUILTIN, HIGH);  // setup done
}


void PinOn(uint16_t pin) {
  Wire.beginTransmission(nanoID);  // begin data transfer to device number 4 on I2C
  Wire.write("H");                 // command to set pin to HIGH
  Wire.write(highByte(pin));       // pin number high byte
  Wire.write(lowByte(pin));        // pin number low byte
  Wire.endTransmission();          // end of transfer
  MQTTclient.publish(String("home/" + hostName + "/status").c_str(), String("H" + String(pin)).c_str()); // write status to MQTT Server
}

void PinOff(uint16_t pin) {
  Wire.beginTransmission(nanoID);  // begin data transfer to device number nanoID on I2C
  Wire.write("L");                 // command to set pin to LOW
  Wire.write(highByte(pin));       // pin number high byte
  Wire.write(lowByte(pin));        // pin number low byte
  Wire.endTransmission();          // end of transfer
  MQTTclient.publish(String("home/" + hostName + "/status").c_str(), String("L" + String(pin)).c_str()); // write status to MQTT Server
}

bool PinInput(uint16_t pin) {
  Wire.beginTransmission(nanoID);  // begin data transfer to device number nanoID on I2C
  Wire.write("I");                 // command to request input
  Wire.write(highByte(pin));       // pin number high byte
  Wire.write(lowByte(pin));        // pin number low byte
  Wire.endTransmission();          // end of transfer

  Wire.requestFrom(nanoID, 1);     // request 1 byte
  while(Wire.available()<1) {        // wait for answer of nano
    delay(5);
  }
  bool pinStatus = Wire.read();    // read the answer
  if (pinStatus==HIGH) {
    MQTTclient.publish(String("home/" + hostName + "/status").c_str(), String("I" + String(pin) + String("HIGH")).c_str() ); // write status to MQTT Server
  } else {
    MQTTclient.publish(String("home/" + hostName + "/status").c_str(), String("I" + String(pin) + String("LOW")).c_str() ); // write status to MQTT Server
  }
  return pinStatus;                // return answer to call
}

//
// BLYNK functions
//
BLYNK_CONNECTED() {
  Blynk.syncAll();
}

//
// button 1
BLYNK_WRITE(V1) {
  bool LEDstate;
  LEDstate = param.asInt();
  lcd.clear();
  lcd.print(0, 0, LEDstate);
  if(LEDstate == 0){
    led1.off();
    PinOff(8);
  } else {
    led1.on();
    PinOn(8);
  }
}

//
// button 2
BLYNK_WRITE(V2) {
  bool LEDstate;
  LEDstate = param.asInt();
  lcd.clear();
  lcd.print(0, 1, LEDstate);
  if(LEDstate == 0){
    led2.off();
    PinOff(9);
  } else {
    led2.on();
    PinOn(9);
  }
}

//
// button 5
BLYNK_WRITE(V5) {
  bool buttonState;
  buttonState = param.asInt();
  if(buttonState == 0){  // -> switch off
    MQTTclient.publish("shellies/shellybulb-3CC556/color/0/command", "off");  // Aussenlampe Vlotho aus
  } else {               // -> switch on
    MQTTclient.publish("shellies/shellybulb-3CC556/color/0/command", "on");   // Aussenlampe Vlotho an
  }
}

//
// button 6
BLYNK_WRITE(V6) {
  bool buttonState;
  buttonState = param.asInt();
  if(buttonState == 0){  // -> switch off
    MQTTclient.publish("shellies/shellyplug-s-040AAA/relay/0/command", "off");  // Joshua Lampe aus
  } else {               // -> switch on
    MQTTclient.publish("shellies/shellyplug-s-040AAA/relay/0/command", "on");   // Joshua Lampe an
  }
}

//
// Terminal
BLYNK_WRITE(V20) {
  char command;
  uint16_t value;

  terminal.println("You said:");
  terminal.write(param.getBuffer(), param.getLength());
  terminal.println();
  if (String(param.asStr()) == String("clear")) {
    terminal.clear();
  }
  // Ensure everything is sent.
  terminal.flush();
  sscanf(param.asStr(), "%c%hu", &command, &value); // %hu for uint16_t
  // select command
  switch(command) {
    case 'H':   // set pin to high
      PinOn(value);
    break;
    case 'L':   // set pin to low
      PinOff(value);
    break;
    default:
    break;
  }
}

//
// PubSubClient MQTT
//
void MQTTreconnect() {
  // Loop until we're reconnected
  while (!MQTTclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (MQTTclient.connect(myMacStr.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //MQTTclient.publish("outTopic","hello world");
      // ... and resubscribe
      MQTTclient.subscribe("shellies/shellybulb-3CC556/color/0");                      // Aussenlampe Vlotho
      MQTTclient.subscribe("shellies/shellyplug-s-040AAA/relay/0");                    // Lampe SZ Joshua
      MQTTclient.subscribe(String("home/" + hostName + "/pin").c_str());               // Arduino Nano
      MQTTclient.publish(String("home/" + hostName + "/status").c_str(), "ONLINE");       // Arduino Nano MQTT feedback

    } else {
      Serial.print("failed, rc=");
      Serial.print(MQTTclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
  String pinVal;
  String payloadStr;

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  payloadStr="";
  for (unsigned int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
    payloadStr+=(char)payload[i];
  }
  Serial.println();
  if(String("shellies/shellybulb-3CC556/color/0") == String(topic)) {
    // Vlotho Aussenlampe
    if(length > 2) { // "off" > 2
      // button has to be OFF as well
      Blynk.virtualWrite(V5, 0);
    } else {         // "on"
      // button has to be ON as well
      Blynk.virtualWrite(V5, 1);
    }
  }
  else if(String("shellies/shellyplug-s-040AAA/relay/0") == String(topic)) {
    // Joshua Lampe SZ
    if(length > 2) { // "off" > 2
      // button has to be OFF as well
      Blynk.virtualWrite(V6, 0);
    } else {         // "on"
      // button has to be ON as well
      Blynk.virtualWrite(V6, 1);
    }
  }
  else if(String("home/" + hostName + "/pin") == String(topic)) {
    if(payloadStr.substring(0,1) == "H") {
      // set pin high
      PinOn((uint16_t) payloadStr.substring(1).toInt());
    } else if(payloadStr.substring(0,1) == "L") {
      // set pin low
      PinOff((uint16_t) payloadStr.substring(1).toInt());
    } else if(payloadStr.substring(0,1) == "I") {
      // set pin low
      PinInput((uint16_t) payloadStr.substring(1).toInt());
    }
  }
}

//elapsedMillis waitTimer = 0;
void loop() {
  // check for OTA updates
  ArduinoOTA.handle();

  // handle all Blynk stuff
  Blynk.run();

  // MQTT part
  if(!MQTTclient.connected()) {
    MQTTreconnect();
  }
  MQTTclient.loop();

}

