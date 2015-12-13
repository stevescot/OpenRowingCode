/* Arduino row Computer
 * Uses an UNO + LCD keypad shield like this: http://www.lightinthebox.com/16-x-2-lcd-keypad-shield-for-arduino-uno-mega-duemilanove_p340888.html?currency=GBP&litb_from=paid_adwords_shopping
 * principles behind calculations are here : http://www.atm.ox.ac.uk/rowing/physics/ergometer.html#section7
 * 13% memory used for LCD / menu
 * 41% total
 */
#include <avr/sleep.h>
#define UseLCD 
#include "mainEngine.h"   
//#define debug  // uncomment this to get more verbose serial output

//-------------------------------------------------------------------
//               pins
const byte switchPin = 2;                    // switch is connected to pin 2
const byte analogPin = 1;                    // analog pin (Concept2)
//-------------------------------------------------------------------
//               reed (switch) handling
int val;                                    // variable for reading the pin status
int buttonState;                            // variable to hold the button state

void setup() 
{
   pinMode(switchPin, INPUT_PULLUP);        // Set the switch pin as input
   Serial.begin(115200);                    // Set up serial communication at 115200bps
   buttonState = digitalRead(switchPin);    // read the initial state
   // set up the LCD's number of columns and rows: 
   #ifdef UseLCD
    lcdSetup();
   #endif
  analogReference(DEFAULT);
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
    Serial.print("No Concept 2 detected on Analog pin ");
    Serial.println(analogPin);
    Serial.print("Detecting reed switch on pin ");
    Serial.println(switchPin);
  }
  Serial.println("Stroke\tSPM\tSplit\tWatts\tDistance\tTime\tDragFactor");
  #ifdef UseLCD
    startMenu();
    //register graphics for up/down
     graphics();
  #endif
  // Print a message to the LCD.
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
      Serial.print("warning - loop took (ms):");
      Serial.println(millis()-mtime);
    }
  buttonState = val;                       // save the new state in our variable
}


void writeStrokeRow()
{
  Serial.print(totalStroke); Serial.print("\t");
  Serial.print(spm); Serial.print("\t");
  Serial.print(getSplitString()); Serial.print("\t");
  Serial.print(power); Serial.print("\t");
  Serial.print(distancem); Serial.print("\t");
  Serial.print(getTime()); Serial.print("\t");
  Serial.print(k*1000000); 
  Serial.println();
}


