/* Arduino row Computer
 *  esp8266 version for wifi
 * principles behind calculations are here : http://www.atm.ox.ac.uk/rowing/physics/ergometer.html#section7
 */
//#define newAPI  - if we have the new API - which has error codes
#include "rowWiFi.h"
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include "mainEngine.h"   
//#define debug  // uncomment this to get more verbose serial output
//ota update : https://github.com/esp8266/Arduino/blob/master/doc/ota_updates/ota_updates.md
//test page: http://monitoring/row/Display.html?MAC=18fe34e62748

//-------------------------------------------------------------------
//               pins
const byte switchPin = 2;                     // switch is connected to GPIO2
const byte holdPin = 0;                       // pin to hold CH_PD high = GPIO0
const byte analogPin = A0;                    // analog pin (Concept2)
//-------------------------------------------------------------------
//               reed (switch) handling
int val;                                      // variable for reading the pin status
int buttonState;                              // variable to hold the button state

unsigned long lastStrokeSentms = 0;

void setup() 
{
  pinMode(holdPin, OUTPUT);  // sets GPIO 0 to output
  digitalWrite(holdPin, HIGH);  // sets GPIO 0 to high (this holds CH_PD high even if the PIR output goes low)
  Serial.begin(115200);                    // Set up serial communication at 115200bps
  Serial.println(F("startup"));
  Serial.print(F("Flash Size:"));
  Serial.println(ESP.getFlashChipSize());
  setupWiFi();
  if(ESP.getFlashChipSize() > 700000)
  {//we have enough space to update automatically
    checkUpdate();
  }
  Serial.println(F("done Wifi"));
  Serial.print(F("MAC:"));
  Serial.println(getMac());
   pinMode(switchPin, INPUT_PULLUP);        // Set the switch pin as input
   buttonState = digitalRead(switchPin);    // read the initial state
   // set up the LCD's number of columns and rows: 
  //analogReference(DEFAULT);
  //analogReference(INTERNAL);
  delay(100);
  if(analogRead(analogPin) == 0 & digitalRead(switchPin) ==  HIGH) 
  {//Concept 2 - set I and flag for analogRead.
    setErgType(ERGTYPEC2);
    Serial.print("Concept 2 detected on pin ");
    Serial.println(analogPin);
  }
  else
  {
    setErgType(ERGTYPEVFIT);
    Serial.print(F("No Concept 2 detected on Analog pin "));
    Serial.println(analogPin);
    Serial.print(F("Detecting reed switch on pin "));
    Serial.println(switchPin);
  }
  Serial.println(F("Stroke\tSPM\tSplit\tWatts\tDistance\tTime\tDragFactor"));
  //
//  Serial.println("sending test row");
//  writeStrokeRow();
//  Serial.println("sent");
}

void loop()
{
  mTime = millis();
  uTime = micros(); 
  processSerial();
  processResponse();
  if(analogSwitch)
  {
    doAnalogRead();
  }
  else
  {
    val = digitalRead(switchPin);            // read input value and store it in val                       
  }
   if (val != buttonState && val == LOW && (uTime- lastStateChangeus) >5000)            // the button state has changed!
    { 
      registerClick();
      lastStateChangeus=uTime;
    }
    if((millis()-mTime) >=10)
    {
      Serial.print(F("warning - loop took (ms):"));
      Serial.println(millis()-mTime);
    }
    if((mTime - lastStrokeSentms) > 3000)
    {//update display if we haven't sent an update for two seconds.
      writeStrokeRow();
    }
  buttonState = val;                       // save the new state in our variable
}


void writeStrokeRow()
{
  lastStrokeSentms = mTime;
  String tab = F("\t");
  Serial.print(totalStroke); Serial.print(tab);
  Serial.print(spm); Serial.print(tab);
  Serial.print(getSplitString()); Serial.print(tab);
  Serial.print(power); Serial.print(tab);
  Serial.print(distancem); Serial.print(tab);
  Serial.print(getTime()); Serial.print(tab);
  Serial.print(k*1000000); 
  Serial.println();
  float splitDistance;
  if(split >0)
  {
   splitDistance = (float)strokems/1000/split*500;
  }
  else 
  {
    splitDistance = 0.00000001;
  }
  SendSplit(mTime, splitDistance, distancem, lastDriveTimems, strokems - lastDriveTimems, powerArray);
}

void checkUpdate()
{
  Serial.println("checking for update");
  Serial.print("Sketch size:");
  Serial.println(ESP.getSketchSize());
    String updateURL = "http://row.intelligentplant.com/row/update.aspx?size=";
    updateURL += ESP.getSketchSize();
    updateURL += "&Date=";
    updateURL += __DATE__;
    updateURL.replace(' ','-');
    Serial.println("update URL:");
    Serial.println(updateURL);
          t_httpUpdate_return ret = ESPhttpUpdate.update(updateURL.c_str());
        //t_httpUpdate_return  ret = ESPhttpUpdate.update("https://server/file.bin");
#ifdef newAPI
        switch(ret) {
            case HTTP_UPDATE_FAILED:
                Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                break;

            case HTTP_UPDATE_NO_UPDATES:
                Serial.println("HTTP_UPDATE_NO_UPDATES");
                break;

            case HTTP_UPDATE_OK:
                Serial.println("HTTP_UPDATE_OK");
                break;
        }
#endif
}



