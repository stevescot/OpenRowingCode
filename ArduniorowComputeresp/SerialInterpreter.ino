//-------------------------------------------------------------------
//               Serial Interface variables
String SerialStr = "";                      // string to hold next serial command.
String variable = "";                       // variable to set 
String value = "";                          // new value for the variable above.
//-------------------------------------------------------------------

void processSerial()
{
  if (Serial.available() >0) {
    char nextChar = Serial.read();
    //Serial.println(nextChar);
    if(nextChar == '=')
    {
      variable = SerialStr;
      SerialStr = "";
    }
    if(nextChar == '\n')
    {
      if(variable == "Session")
      {
            sessionType = SerialStr.toInt();
            Serial.println(F("Session Set"));
      }
      else if(variable == "Interval")
      {
            targetSeconds = SerialStr.toInt();
            Serial.println(F("Interval Set"));
      }
      else if(variable =="Rest")
      {
            intervalSeconds = SerialStr.toInt();
            Serial.println(F("Rest Set"));
      }
      else if(variable =="Intervals")
      {
            numIntervals = SerialStr.toInt();
            Serial.println(F("Num Intervals Set"));
      }
      else if(variable =="TargetDistance")
      {
            numIntervals = SerialStr.toInt();
            Serial.println(F("TargetDistance Set"));
      }
      else if(variable == "TargetTime")
      {
            targetSeconds = SerialStr.toInt();
            Serial.println(F("Target Time Set"));
      }
      else if(variable == "DumpRPM")
      {
            dumprpms();
      }
      else 
      {
          Serial.println(F("Unrecognised"));
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

