/* Arduino row Computer
 *  esp8266 version for wifi
 * principles behind calculations are here : http://www.atm.ox.ac.uk/rowing/physics/ergometer.html#section7
 */
#include "mainEngine.h"   
//#define debug  // uncomment this to get more verbose serial output
//ota update : https://github.com/esp8266/Arduino/blob/master/doc/ota_updates/ota_updates.md

//-------------------------------------------------------------------
//               pins
const byte switchPin = 1;                    // switch is connected to pin 2
const byte analogPin = 2;                    // analog pin (Concept2)
//-------------------------------------------------------------------
//               reed (switch) handling
int val;                                    // variable for reading the pin status
int buttonState;                            // variable to hold the button state

void setup() 
{
  
   Serial.begin(115200);                    // Set up serial communication at 115200bps
  Serial.println(F("startup"));
  setupWiFi();
  Serial.println(F("done Wifi"));
   pinMode(switchPin, INPUT_PULLUP);        // Set the switch pin as input
   buttonState = digitalRead(switchPin);    // read the initial state
   // set up the LCD's number of columns and rows: 
  //analogReference(DEFAULT);
  //analogReference(INTERNAL);
  delay(100);
  /*if(analogRead(analogPin) == 0 & digitalRead(switchPin) ==  HIGH) 
  {//Concept 2 - set I and flag for analogRead.
    setErgType(ERGTYPEC2);
    Serial.print("Concept 2 detected on pin ");
    Serial.println(analogPin);
  }
  else
  {*/
    setErgType(ERGTYPEVFIT);
    Serial.print(F("No Concept 2 detected on Analog pin "));
    Serial.println(analogPin);
    Serial.print(F("Detecting reed switch on pin "));
    Serial.println(switchPin);
  /*}*/
  Serial.println(F("Stroke\tSPM\tSplit\tWatts\tDistance\tTime\tDragFactor"));
}

void loop()
{
  mtime = millis();
  utime = micros(); 
  processSerial();
  if(AnalogSwitch)
  {
    doAnalogRead();
  }
  else
  {
    val = digitalRead(switchPin);            // read input value and store it in val                       
  }
   if (val != buttonState && val == LOW && (utime- laststatechangeus) >5000)            // the button state has changed!
    { 
      registerClick();
         #ifdef UseLCD
            writeNextScreen();
         #endif
      laststatechangeus=utime;
    }
    if((millis()-mtime) >=10)
    {
      Serial.print(F("warning - loop took (ms):"));
      Serial.println(millis()-mtime);
    }
  buttonState = val;                       // save the new state in our variable
}


void writeStrokeRow()
{
  String tab = F("\t");
  Serial.print(totalStroke); Serial.print(tab);
  Serial.print(spm); Serial.print(tab);
  Serial.print(getSplitString()); Serial.print(tab);
  Serial.print(power); Serial.print(tab);
  Serial.print(distancem); Serial.print(tab);
  Serial.print(getTime()); Serial.print(tab);
  Serial.print(k*1000000); 
  Serial.println();
  float splitdistance = (float)strokems/1000/split*500;
  SendSplit(mtime, splitdistance, distancem, lastDriveTimems, strokems - lastDriveTimems);
}





