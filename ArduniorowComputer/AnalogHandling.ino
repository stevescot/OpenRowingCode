//-------------------------------------------------------------------
//               C2(analog handling
static int AnalogMinValue = 4*5;              // minimum value of pulse to recognise the wheel as spinning
static byte AnalogCountMin = 8;             // minimum number of values above the min value before we start monitoring
byte AnalogCount = 0;                        // indicator to show how high the analog limit has been (count up /count down)
byte lastAnalogSwitchValue = 0;              // the last value read from the C2
bool AnalogDropping = false;                // indicates if teh analog value is now dropping from a peak (wait until it gets to zero).
float previousGradient = 0;
float gradient = 0;
unsigned long lastAnalogReadus = 0;

void doAnalogRead()
{//simulate a reed switch from the coil
    int analog = analogRead(analogPin);
    val = HIGH;
    gradient = (analog - lastAnalogSwitchValue)/(uTime-lastAnalogReadus);
    float gradientOfGradient = (previousGradient-gradient)/(uTime-lastAnalogReadus);
    if(!AnalogDropping)
    {
      if(analog < lastAnalogSwitchValue && (uTime- lastStateChangeus) >5000)
      {//we are starting to drop - mark the value as low, and analog as dropping.
        //use this to see if the analog limit has tended to be above 6
        if(analog >= AnalogMinValue)
        {
          AnalogCount ++;
        }
        else
        {
          AnalogCount -= 2;
        }
        if(AnalogCount > AnalogCountMin*2) AnalogCount = AnalogCountMin*2;
        if(AnalogCount < 0) AnalogCount = 0;
        if(AnalogCount > AnalogCountMin)//on average the analog value has been above 6 for the last 10 samples.
        {
          val = LOW;
          AnalogDropping = true;
          //adjust the time using the gradient:
            long timeAdjustment = (float)previousGradient/(-gradientOfGradient);
            if(timeAdjustment > 120) 
            {
              Serial.print("Warning, adjustment too high, something went wrong ");
              Serial.println(timeAdjustment);
              Serial.print("Analog Value:");
              Serial.println(analog);
              Serial.print("Analog Count");
              Serial.println(AnalogCount);
            }
            uTime = lastAnalogReadus + timeAdjustment;
        }
      }
    }
    if(analog== 0) AnalogDropping = false;//we have reached 0 - reset analog dropping so we can monitor for it once analog starts to drop.
    lastAnalogSwitchValue = analog;
    lastAnalogReadus = uTime;
    previousGradient = gradient;
}
