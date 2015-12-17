/* Arduino row Computer
 * Uses an UNO + LCD keypad shield like this: http://www.lightinthebox.com/16-x-2-lcd-keypad-shield-for-arduino-uno-mega-duemilanove_p340888.html?currency=GBP&litb_from=paid_adwords_shopping
 * principles behind calculations are here : http://www.atm.ox.ac.uk/rowing/physics/ergometer.html#section7
 * 13% memory used for LCD / menu
 * 41% total
 */
#include <avr/sleep.h>
#define UseLCD //comment out this line if you don't have an LCD shield
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
  delay(100);
  analogReference(DEFAULT);
  if(analogRead(analogPin) == 0 & digitalRead(switchPin) ==  HIGH) 
  {//Concept 2 - set I and flag for analogRead.
    setErgType(ERGTYPEC2);
    Serial.print(F("Concept 2 detected on pin "));
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
  //done with LCD sheild - safe to use internal reference now (is there a workaround for this...?)
  analogReference(INTERNAL);
  // Print a message to the LCD.
}

void loop()
{
  mTime = millis();
  uTime = micros(); 
  processSerial();
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
         #ifdef UseLCD
            writeNextScreen();
         #endif
      lastStateChangeus=uTime;
    }
    if((millis()-mTime) >=23)
    {
      Serial.print(F("warning - loop took (ms):"));
      Serial.println(millis()-mTime);
    }
  buttonState = val;                       // save the new state in our variable
}


void writeStrokeRow()
{
  char tabchar = '\t';
  Serial.print(totalStroke); Serial.print(tabchar);
  Serial.print(spm); Serial.print(tabchar);
  Serial.print(getSplitString()); Serial.print(tabchar);
  Serial.print(power); Serial.print(tabchar);
  Serial.print(distancem); Serial.print(tabchar);
  Serial.print(getTime()); Serial.print(tabchar);
  Serial.print(k*1000000); 
  Serial.println();
}


