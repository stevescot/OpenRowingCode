//-------------------------------------------------------------------
// Steve Aitken - 2015 
// Serial interface for the row computer
// allows the following commands:
//               Serial Interface variables
String SerialStr = "";                        // string to hold next serial command.
String variable = "";                   // variable to set 
//const char * value = "";                    // new value for the variable above.
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
      Serial.print(variable);
      Serial.print(" ");
      if(variable =="Session")
      {
            sessionType = SerialStr.toInt();
            Serial.println(F("Set"));
      }
      else if(variable =="Interval")
      {
            targetSeconds = SerialStr.toInt();
            Serial.println(F("Set"));
      }
      else if(variable =="Rest")
      {
            intervalSeconds = SerialStr.toInt();
            Serial.println(F("Set"));
      }
      else if(variable =="Intervals")
      {
            numIntervals = SerialStr.toInt();
            Serial.println(F("Set"));
      }
      else if(variable =="TargetDistance")
      {
            numIntervals = SerialStr.toInt();
            Serial.println(F("Set"));
      }
      else if(variable =="TargetTime")
      {
            targetSeconds = SerialStr.toInt();
            Serial.println(F("Set"));
      }
      else if(variable =="DumpRPM")
      {
            dumprpms();
      }
      else 
      {
          Serial.println(F(" Unreckognised"));
      }
      SerialStr = "";
    }
    else
    {
      SerialStr += nextChar;
    }
  }
}

