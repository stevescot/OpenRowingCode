//-------------------------------------------------------------------
// Steve Aitken - 2015 
// Serial interface for the row computer
// allows the following commands:
//               Serial Interface variables
String SerialStr = "";                        // string to hold next serial command.
const char * variable = "";                   // variable to set 
//const char * value = "";                    // new value for the variable above.
//-------------------------------------------------------------------
const char p_Session[] PROGMEM =            "Session";
const char p_Interval[] PROGMEM =           "Interval";
const char p_Rest[] PROGMEM =               "Rest";
const char p_Intervals[] PROGMEM =          "Intervals";
const char p_TargetDistance[] PROGMEM =     "TargetDistance";
const char p_TargetTime[] PROGMEM =         "TargetTime";
const char p_DumpRPM[] PROGMEM =            "DumpRPM";

void processSerial()
{
  if (Serial.available() >0) {
    char nextChar = Serial.read();
    //Serial.println(nextChar);
    if(nextChar == '=')
    {
      variable = SerialStr.c_str();
      SerialStr = "";
    }
    if(nextChar == '\n')
    {
      if(variable == p_Session)
      {
            sessionType = SerialStr.toInt();
            Serial.println(F("Session Set"));
      }
      else if(variable == p_Interval)
      {
            targetSeconds = SerialStr.toInt();
            Serial.println(F("Interval Set"));
      }
      else if(variable == p_Rest)
      {
            intervalSeconds = SerialStr.toInt();
            Serial.println(F("Rest Set"));
      }
      else if(variable ==p_Intervals)
      {
            numIntervals = SerialStr.toInt();
            Serial.println(F("Num Intervals Set"));
      }
      else if(variable ==p_TargetDistance)
      {
            numIntervals = SerialStr.toInt();
            Serial.println(F("TargetDistance Set"));
      }
      else if(variable == p_TargetTime)
      {
            targetSeconds = SerialStr.toInt();
            Serial.println(F("Target Time Set"));
      }
      else if(variable == p_DumpRPM)
      {
            dumprpms();
      }
      else 
      {
          Serial.println(F("Unreckognised"));
          Serial.println(variable);
      }
      SerialStr = "";
    }
    else
    {
      SerialStr += nextChar;
    }
  }
}

