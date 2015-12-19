//-------------------------------------------------------------------
// Steve Aitken - 2015 - with much help in testing from Al Bates - who spent many hours with audio files retesting and retesting the methods until we settled on this one.
//               C2(analog handling
// This code uses the analog read from the analog pin as often as possible (100us on Arduino) to detect/predict when a zero crossing happened.
// 100us is not precise enough to get an accurate reading of power or drag factor, so we have to use the gradient before we hit zero to draw 
// a line through zero - this gives us a more accurate and precise estimate of the time that the value crossed zero.
// with this time, we reset the uTime global variable to the actual time in micro seconds that this would have happend, 
// and we set val to LOW indicating that we have detected a click
//-------------------------------------------------------------------

static const byte AnalogMinValue = 4*5;              // minimum value of pulse to recognise the wheel as spinning
static const byte AnalogCountMin = 8;               // minimum number of values above the min value before we start monitoring
byte AnalogCount = 0;                         // indicator to show how high the analog limit has been (count up /count down)
byte lastAnalogSwitchValue = 0;               // the last value read from the C2
bool AnalogDropping = false;                  // indicates if teh analog value is now dropping from a peak (wait until it gets to zero).
float previousGradient = 0;                   // gradient from the previous two analog samples
float gradient = 0;                           // current gradient 
unsigned long lastAnalogReadus = 0;           // the previous measured analog value.
unsigned long firstGreaterThanZeroTus = 0;    // the first sample that is greater than zero
unsigned long peakTus = 0;                    // the peak Time
unsigned long zeroTus = 0;                    // the time we returned to zero
byte peakDecayFactor = 50;                     // peak decay factor - less than 50 = peak first, greater than 50 = decay first.

// Gradient stuff.
const byte numGradients = 5;                   // number of gradients to take a median of
float GradientArray[numGradients];            // array of raw gradients
byte currentGradienti = 0;                      // current gradient index in array
byte numGradientsInArray = 0;                  // number of gradients we have in the array.

float AddGradientAndGetMedian(float gradient)
{
  GradientArray[currentGradienti] = gradient;
  currentGradienti++; if(currentGradienti >= numGradients) currentGradienti = 0;
  numGradientsInArray++; if(numGradientsInArray >= numGradients) numGradientsInArray = numGradients-1;
  return median(GradientArray, numGradientsInArray);
}

void doAnalogRead()
{//simulate a reed switch from the coil
    int analog = analogRead(analogPin);
    val = HIGH;
    gradient = (float)(analog - lastAnalogSwitchValue)/(uTime-lastAnalogReadus);
    if(analog== 0 && AnalogDropping) 
    {
      zeroTus = uTime;
    }
    if(!AnalogDropping)
    {
      if(analog > 0 && lastAnalogSwitchValue ==0)
      {
        firstGreaterThanZeroTus = uTime;
      }
      else if(firstGreaterThanZeroTus == lastAnalogReadus && peakDecayFactor <50)//detect that previous value was the first one above zero, and we are peak first, then decay,
      {
        float medianGradient = AddGradientAndGetMedian(gradient);
        unsigned long usdiffprev = (float)lastAnalogSwitchValue / gradient ;
        if(usdiffprev < (uTime-lastAnalogReadus))
        {//numbers are reasonable - calculate the actual time that this happened, and use it.
         uTime = lastAnalogReadus - usdiffprev;
         val = LOW;
        }
        else
        {//leads to before the previous sample, go for the previous sample
          uTime = firstGreaterThanZeroTus - (uTime - firstGreaterThanZeroTus);
          val = LOW;
          //print out some stats...
              Serial.print(F("Warning, adjustment too high, something went wrong -  "));
              Serial.println(usdiffprev);
              Serial.print(F("Analog Value:"));
              Serial.println(analog);
              Serial.print(F("Analog Count"));
              Serial.println(AnalogCount);
              Serial.print(F("Previous Value"));
              Serial.println(lastAnalogSwitchValue);
              Serial.print(F("Current Value"));
              Serial.println(analog);
              Serial.print(F("gradient"));
              Serial.println(gradient);
              Serial.print(F("prevgradient"));
              Serial.println(previousGradient);
              Serial.print(F("mediangradient"));
              Serial.println(medianGradient);
        }
      }
      if(analog < lastAnalogSwitchValue && (uTime- lastStateChangeus) >5000)
      {//we are starting to drop -and analog as dropping.
        peakTus = lastAnalogReadus;
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
          AnalogDropping = true;
        }
      }
    }
    else
    {//we are dropping
      if(peakDecayFactor >50)//peak towards end of time sample
        {
        if(lastAnalogSwitchValue > 0 && analog ==0 )//we have been dropping and have now hit zero - find when we would have hit it given the previous gradient.
        {
          float medianGradient = AddGradientAndGetMedian(previousGradient);
          unsigned long usdiffprev = (float)lastAnalogSwitchValue / (-previousGradient);
          if(previousGradient < 0 && (lastAnalogReadus + usdiffprev) < uTime)
          {//numbers are reasonable - calculate the actual time that this happened, and use it.
           uTime = lastAnalogReadus + usdiffprev;
           val = LOW;
          }
          else
          {
            //probalby not enough samples to reliably detect the point of intersection
                Serial.print(F("Warning, adjustment too high, something went wrong "));
                Serial.println(usdiffprev);
                Serial.print(F("Analog Value:"));
                Serial.println(analog);
                Serial.print(F("Analog Count"));
                Serial.println(AnalogCount);
                Serial.print(F("Previous Value"));
                Serial.println(lastAnalogSwitchValue);
                Serial.print(F("Current Value"));
                Serial.println(analog);
                Serial.print(F("gradient"));
                Serial.println(gradient);
                Serial.print(F("prevgradient"));
                Serial.println(previousGradient);
                Serial.print(F("mediangradient"));
                Serial.println(medianGradient);
                //uTime = lastAnalogReadus;
                val = LOW;
          }
        }
      }
    }
    if(analog== 0 && AnalogDropping) 
    {
      zeroTus = uTime;
      AnalogDropping = false;//we have reached 0 - reset analog dropping so we can monitor for it once analog starts to drop.     
      if((peakTus - firstGreaterThanZeroTus) > (zeroTus - peakTus))
      {
        peakDecayFactor ++;;
        if(peakDecayFactor >100) peakDecayFactor =100;
      }
      else
      {
        peakDecayFactor --;
        if(peakDecayFactor <0) peakDecayFactor = 0;
      }
    }
    lastAnalogSwitchValue = analog;
    lastAnalogReadus = uTime;
    previousGradient = gradient;
}

//the previous AnalogRead function - this gave relatively reliable drag factors but is hopefully improved upon by the above function
// this function attempts to predict when the analog values peaked rather than when they returned to zero.
void AnalogReadOld()
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
            if(timeAdjustment > 120 || timeAdjustment < -120) 
            {
              Serial.print(F("Warning, adjustment too high, something went wrong "));
              Serial.println(timeAdjustment);
              Serial.print(F("Analog Value:"));
              Serial.println(analog);
              Serial.print(F("Analog Count"));
              Serial.println(AnalogCount);
              Serial.print(F("Previous Value"));
              Serial.println(lastAnalogSwitchValue);
              Serial.print(F("Current Value"));
              Serial.println(analog);
              Serial.print(F("gradient"));
              Serial.println(gradient);
              Serial.print(F("gradientOfGradient"));
              Serial.println(gradientOfGradient);
            }
            else
            {
              uTime = lastAnalogReadus + timeAdjustment;
            }
        }
      }
    }
    if(analog== 0) AnalogDropping = false;//we have reached 0 - reset analog dropping so we can monitor for it once analog starts to drop.
    lastAnalogSwitchValue = analog;
    lastAnalogReadus = uTime;
    previousGradient = gradient;
}
