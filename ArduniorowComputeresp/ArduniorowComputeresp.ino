/* Arduino row Computer
 *  esp8266 version for wifi
 * principles behind calculations are here : http://www.atm.ox.ac.uk/rowing/physics/ergometer.html#section7
 */
 
// #define DEBUG
//#define DEBUG_OUTPUT Serial
#define newAPI 
#define debughttp
#define SimulateRower

// - if we have the new API - which has error codes
String MAC ="";                  // the MAC address of your Wifi shield
int lastCommand = -1;
extern "C" {
  #include "user_interface.h"
  #if ndef newAPI
  uint16 readvdd33(void);
  bool wifi_set_sleep_type(enum sleep_type_t type);
  enum sleep_type_t wifi_get_sleep_type(void);
 #endif
}

const int maxSleep = 20;

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <gpio.h>

#include "mainEngine.h"   
//#define debug  // uncomment this to get more verbose serial output
//http://www.cnx-software.com/2015/10/29/getting-started-with-nodemcu-board-powered-by-esp8266-wisoc/
//ota update : https://github.com/esp8266/Arduino/blob/master/doc/ota_updates/ota_updates.md
//test page: http://monitoring/row/Display.html?MAC=18fe34e62748

//-------------------------------------------------------------------
//               pins
const byte switchPin = 2;                     // switch is connected to GPIO2
//const byte wakePin   = 0;                     // pin to wake with     = GPIO0
const byte analogPin = A0;                    // analog pin (Concept2)
//const int msToResendSplit = 1000;
const int sleepTimeus = 60000000;                 // how long with no input before going to low power.
const int maxTimeForPulseus = 50000;
//-------------------------------------------------------------------
//               reed (switch) handling
int val;                                      // variable for reading the pin status
int buttonState;                              // variable to hold the button state

bool sleep = false;

unsigned long lastStrokeSentms = 0;

void setup() 
{
  Serial.begin(115200);                    // Set up serial communication at 115200bps
  #if debug
  Serial.setDebugOutput(true);
  #endif
  Serial.println(F("startup"));
  Serial.print(F("Flash Size:"));
  Serial.println(ESP.getFlashChipSize());
  setupWiFi();
  Serial.println(F("done Wifi"));
  Serial.print(F("MAC:"));
  Serial.println(getMac());
  checkUpdate();
  pinMode(switchPin, INPUT_PULLUP);        // Set the switch pin as input
  buttonState = digitalRead(switchPin);    // read the initial state
  delay(100);
  detectMachine();
  Serial.println(F("Stroke\tSPM\tSplit\tWatts\tDistance\tTime\tDragFactor"));
  wifi_set_sleep_type(MODEM_SLEEP_T);
}

void loop()
{
  mTime = millis();
  uTime = micros(); 
  processSerial();
  processResponse();
  statusStr = "";
  if(monitorEnabled)
  {
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
        wakeUp();
        registerClick();
        lastStateChangeus=uTime;
        //delay(calculateSleepTime(lastStateChangeus, uTime));
      }
      if((millis()-mTime) >=10)
      {//loop took a long time, 
        Serial.print(F("warning - loop took (ms):"));
        Serial.println(millis()-mTime);
      }
      if((lastStateChangeus + sleepTimeus < uTime) || (lastStateChangeus + maxTimeForPulseus < uTime  && sleep))
      {//powersave after 10 seconds of no data.         
        sleepUntilRace();
      }
  }
  buttonState = val;                       // save the new state in our variable
  }

//calculate the time to sleep before attempting to read the state of the switches.
unsigned long calculateSleepTime(unsigned long previousClickus, unsigned long currentClickus)
{
    unsigned long currentTime = micros();
    unsigned long msbetweenClicks = (currentClickus-previousClickus) / 1000 ;
    unsigned long msAlreadyElapsed = (currentTime - currentClickus) / 1000 + 1;
    if(msAlreadyElapsed > msbetweenClicks)
    {
      return 0;
    }
    else
    {
      const float fractionalIncrease = 0.5;
      unsigned long msWithAcceleration = fractionalIncrease * (msbetweenClicks - msAlreadyElapsed);
      if(msWithAcceleration > maxSleep)
      {
        return maxSleep;
      }
      else
      {
        Serial.println(msWithAcceleration);
        return msWithAcceleration;
      }
    }
}

void sleepUntilRace()
{
  if((raceStartTimems > 0) && (mTime < raceStartTimems))
  {//race is coming up..
    if(mTime+20000 > raceStartTimems)
    {
      updateStatus("Waiting for Race"); 
      goToModemSleep();
      delay(raceStartTimems - mTime - 5000);
      wakeUp();
    }
  }
  else
  {//stop for a couple of seconds to save power, then recheck for maxTimeForPulseus
    updateStatus("Idle - Sleeping"); 
    goToModemSleep();
    delay(2000);
    lastStateChangeus = uTime;//reset lastStateChange so that we monitor for a while after our two second delay.
  }
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
  sendSplit(MAC, getCurrentTimems(), splitDistance, distancem, lastDriveTimems, strokems - lastDriveTimems, spm, powerArray, nextPower, statusStr, lastCommand);
}

void checkUpdate()
{
  if(ESP.getFlashChipSize() > 700000)
  {//we have enough space to update automatically
  updateStatus("checking for update");
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
  else
  {
    Serial.println("not enough room for update");
    Serial.print("Chip Size: ");
    Serial.println(ESP.getFlashChipSize());
    Serial.print("Sketch Size: ");
    Serial.println(ESP.getSketchSize());
  }
}



