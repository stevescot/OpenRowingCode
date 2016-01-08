//-------------------------------------------------------------------
// Steve Aitken - 2015 
// Serial interface for the row computer
// allows the following commands:
//               Serial Interface variables


#include <EEPROM.h>
String SerialStr = "";                        // string to hold next serial command.
String variable = "";                   // variable to set 
//const char * value = "";                    // new value for the variable above.
//-------------------------------------------------------------------

void processSerial()
{
  if (Serial.available() >0) {
    char nextChar = Serial.read();
    Serial.println(nextChar);
    if(nextChar == '=')
    {
      variable = SerialStr;
      SerialStr = "";
      Serial.println("Variable " + variable);
    }
    else if(nextChar == '\n' || nextChar==';')
    {
     Serial.println("\\n"); 
      Serial.print(variable);
      Serial.print(" ");
       if(!variable.equals(""))
      {
        Serial.print(variable);
        Serial.print(" ");
        if(variable.equals("SessionType"))
        {
              sessionType = SerialStr.toInt();
              Serial.println(F("Set"));
        }
        else if(variable.equals("Interval"))
        {
              targetSeconds = SerialStr.toInt();
              Serial.println(F("Set"));
        }
        else if(variable.equals("Rest"))
        {
              intervalSeconds = SerialStr.toInt();
              Serial.println(F("Set"));
        }
        else if(variable.equals("Intervals"))
        {
              numIntervals = SerialStr.toInt();
              Serial.println(F("Set"));
        }
        else if(variable.equals("TargetDistance"))
        {
              targetDistance = SerialStr.toInt();
              Serial.println(F("Set"));
        }
        else if(variable.equals("TargetTime"))
        {
              targetSeconds = SerialStr.toInt();
              Serial.println(F("Set"));
        }
        else if(variable.equals("StartInTenths"))
        {
              distancem = 0;
              Serial.print( "   = ");
              Serial.println(SerialStr);
              raceStartTimems = millis() + SerialStr.toInt()*100;
        }
        else if(variable.equals("NewSession"))
        {
          resetSession();
        }
        else if(variable.equals("Restart"))
        {
  //        reset();
        }
        else if(variable.equals("DumpRPM"))
        {
          dumprpms();
        }
        else if(variable.equals("zerodistance"))
        {
          distancem = 0;
        }
        else 
        {
          #ifdef debughttp
            Serial.println(F(" Unreckognised"));
            Serial.println();
            Serial.println(SerialStr);
           #endif
        }
      }
      SerialStr = "";
      variable = "";
    }
    else
    {
      SerialStr += nextChar;
    }
  }
}

