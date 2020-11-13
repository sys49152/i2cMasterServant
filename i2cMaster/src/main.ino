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

// timer lib.
#include <elapsedMillis.h>

void setup() {
  Serial.begin(115200);
  Serial.println("\nBooting");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // LOW all the way through setup, then HIGH
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

  // init. I2C
  Wire.begin(); // start I2C master

  digitalWrite(LED_BUILTIN, HIGH);  // setup done
}
void PinOn(byte pin) {
  Wire.beginTransmission(nanoID);  // begin data transfer to device number 4 on I2C
  Wire.write("H");                 // command to set pin to HIGH
  Wire.write(pin);                 // pin number
  Wire.endTransmission();          // end of transfer
}

void PinOff(byte pin) {
  Wire.beginTransmission(nanoID);  // begin data transfer to device number nanoID on I2C
  Wire.write("L");                 // command to set pin to LOW
  Wire.write(pin);                 // pin number
  Wire.endTransmission();          // end of transfer
}

bool PinInput(byte pin) {
  Wire.beginTransmission(nanoID);  // begin data transfer to device number nanoID on I2C
  Wire.write("I");                 // command to request input
  Wire.write(pin);                 // pin number
  Wire.endTransmission();          // end of transfer

  Wire.requestFrom(nanoID, 1);     // request 1 byte
  while(Wire.available()<1) {        // wait for answer of nano
    delay(5);
  }
  bool pinStatus = Wire.read();    // read the answer
  return pinStatus;                // return answer to call
}

elapsedMillis waitTimer = 0;
void loop() {
  static bool LEDstatus = LOW;
  ArduinoOTA.handle();

  if (waitTimer > 5000) {
    if (LEDstatus == LOW) {
      PinOn(13);
      if (PinInput(3)) {
        PinOff(6);
        PinOn(5);
      } else {
        PinOn(6);
        PinOff(5);
      }
      LEDstatus = HIGH;
      waitTimer = 0;
      Serial.print(millis());
      Serial.println(" LED on");
    } else {
      PinOff(13);
      if (PinInput(3)) {
        PinOff(6);
        PinOn(5);
      } else {
        PinOn(6);
        PinOff(5);
      }
      LEDstatus = LOW;
      waitTimer = 0;
      Serial.print(millis());
      Serial.println(" LED off");
    }
  }
}

