//-------------------------------------------------------------------
// Steve Aitken - 2015
// Main Engine
//
// This is the part of the row computer that does all the working out- 
// given a time set as uTime, and a call to registerClick every time a part of the wheel passes, 
// this will work out the drag factor, power, split, powergraph etc.. from strokes and print them to serial
//

//-------------------------------------------------------------------
//               click based variables
unsigned long clicks = 0;                   // number of clicks since start
unsigned long clicksInDistance = 0;         // number of clicks already accounted for in the distance.
unsigned int diffclicks;                    // clicks from last stroke to this one
unsigned long driveStartclicks;             // number of clicks at start of drive.
byte currentrot = 0;                        // current rotation (for number of clicks per calculation)
//-------------------------------------------------------------------
//               global constants
float c = 2.8;                              //The figure used for c is somewhat arbitrary - selected to indicate a 'realistic' boat speed for a given output power. c/p = (v)^3 where p = power in watts, v = velocity in m/s  so v = (c/p)^1/3 v= (2.8/p)^1/3
                                            //Concept used to quote a figure c=2.8, which, for a 2:00 per 500m split (equivalent to u=500/120=4.17m/s) gives 203 Watts. 
//-------------------------------------------------------------------
//               rpm/angular Velocity
float previousDriveAngularVelocity;         // fastest angular velocity at end of previous drive
//float driveAngularVelocity;               // fastest angular velocity at end of drive
float recoveryBeginAngularVelocity;         // angular velocity at the beginning of the recovery  end drive)
float recoveryEndAngularVelocity;           // angular velocity at the end of the revovery
float previousRecoveryEndAngularVelocity;   // previous recovery end angular velocity
unsigned long previousRecoveryEndms;        // time in ms of the end of the previous recovery
unsigned long recoveryBeginms;              // time in ms of the beginning of the recovery
//-------------------------------------------------------------------
//               acceleration/deceleration
const unsigned int consecutivedecelerations = 2;          //number of consecutive decelerations before we are decelerating
const unsigned int consecutiveaccelerations = 2;          // number of consecutive accelerations before detecting that we are accelerating.
unsigned int decelerations = consecutivedecelerations +1; // number of decelerations detected.
unsigned int accelerations = 0;                           // number of acceleration rotations;
//-------------------------------------------------------------------
//               drag factor  
int kIndex = 0;                                           //current position in the drag factor array
const int maxKArray = 60;                                 //maximum number of samples to store in the drag factor array
int kArray[maxKArray];                                    // array of k values (so we can get a median when we are done.
float mPerClick = 0;                                      // meters per click from the flywheel (calculated from drag factor)

//Set boat type - need some c factors for 4, single etc..
void setBoatType(short BoatType)
{
  boatType = BoatType;
  switch(BoatType)
  {
    case BOAT4:
      c = 2.8;
    break;
    case BOAT8:
      c = 2.8;
    break;
    case BOAT1:
      c = 2.8;
    break;
  }
}

//set the Erg type (concept 2 or otehr)
void setErgType(short newErgType)
{
  ergType = newErgType;
  switch(newErgType)
  {
    case ERGTYPEVFIT:
        //V-Fit rower with tach.
        analogSwitch = false;
        I = 0.048;//experimentally verified
        clicksPerRotation = 1;
        clicksPerCalc = 1;
        k = 0.000175;    
        mStrokePerRotation = 0;//meters of stroke per rotation of the flywheel - V-fit.
        break;
    default:
        //Concept 2
        analogSwitch = true;
        I = 0.101;
        //do the calculations less often to allow inaccuracies to be averaged out.
        clicksPerCalc = 3;//take out a lot of noise before we detect drive / recovery.
        //number of clicks per rotation is 3 as there are three magnets.
        clicksPerRotation = 3;
        k = 0.000135;
        mStrokePerRotation = 0.08;//meters of stroke per rotation of the flywheel - C2.
        ergType = ERGTYPEC2;    
        break;
  }
  mPerClick = pow((k/c),(0.33333333333333333))*2*PI/clicksPerRotation;
}

//Calculate the amount of power used to accelerate the wheel by the change in rpm for this click
float calculateInstantaneousPower()
{
  //= I ( dω / dt ) dθ + k ω2 dθ 
  // That is for energy so all above /dt = power
  float dtheta = (2*PI/clicksPerRotation*clicksPerCalc);
  //Serial.print((float)timeTakenus/1000000);
  float instw = (float)getRpm(0)/60*2*PI;
  float prevw = (float)getRpm(-1)/60*2*PI;
  float dTs = (float)timeTakenus/1000000;
  float instantaneouspower = (I *(instw-prevw)/(dTs)*dtheta + k * pow(instw,2) * dtheta)/dTs;
#ifdef debug
  Serial.print(instantaneouspower); Serial.println("W");
#endif
  return instantaneouspower;
}

void storeInstantaneousPower(float instantaneouspower)
{
  if(nextPower < powerSamples && instantaneouspower > 0)
  {
    powerArray[nextPower] = instantaneouspower;
    nextPower++;
  }
  else
  {
    if(nextPower >= powerSamples)
    {
      #ifdef debug
      Serial.println(F("More samples than power array"));
      #endif
    }
    else
    {
      //negative power
    }
  }
}

//calculate the drag factor for this part of the recovery and add it to the array
void addDragFactorToArray()
{
  float nextk = I * ((1.0/recoveryEndAngularVelocity)-(1.0/recoveryBeginAngularVelocity))/(secondsDecel)*1000000;
  if(split < 180)// split is less than 3 mins, calculate drag factor
  {
    if(nextk >40 && nextk < 300 && kIndex < maxKArray)
    {
      kArray[kIndex] = nextk;
      kIndex ++;
    }
    else
    {//dodgy k - write out to serial if debug.
    #ifdef debug
      Serial.print(F("k:")); Serial.print(nextk); Serial.print(F("recoverybeginrad")); Serial.print(recoveryBeginAngularVelocity); Serial.print(F("recoveryend")); Serial.print(recoveryEndAngularVelocity); Serial.print(F("recoverySeconds")); Serial.print(secondsDecel);
      Serial.print(F(" last Recovery time ")); Serial.println(previousSecondsDecel);
  //    Serial.print(F("k:")); Serial.println(nextk); Serial.print(F("recw:")); Serial.println(recoveryBeginAngularVelocity); Serial.print(F("dw")); Serial.println(driveAngularVelocity); 
      Serial.print(F("peakRPM")); Serial.println(peakRPM); Serial.print(F("sdecel")); Serial.println(secondsDecel);
    #endif
    }
  }
}

//calculate the median drag factor from all the drag factors that were calculated during the recovery, 
//and set the number of meters per click from the wheel accordingly
void getDragFactor()
{
  if(kIndex > 5)
  {//more than 5 values so calculate k
    k = (float)median(kArray,kIndex)/1000000;  //get a median of all the k calculations
    mPerClick = pow((k/c),(0.33333333333333333))*2*PI/clicksPerRotation;//v= (2.8/p)^1/3  
    #ifdef debug
    Serial.println();
    Serial.println();
    Serial.print(F("K MEDIAN")); Serial.println(k*1000000); Serial.print(F("samples ")); Serial.println(kIndex);
    Serial.println();
    Serial.println();
    #endif
    kIndex = 0;
  }
}

unsigned long getCurrentTimems()
{
  if(startTimems!=0)
  {
    return mTime - startTimems;
  }
  else
  {
    return 0;
  }
}

//take uTime and mTime and use them to calculate all of the figures we need given a click from the wheel.
void registerClick()
{
  if(raceStartTimems == 0 || mTime > raceStartTimems)
  {//we are racing and it's started, or we aren't racing, so register clicks
  currentrot ++;
      clicks++;
      if(currentrot >= clicksPerCalc)
      {
        currentrot = 0;
        //initialise the start time
        if(startTimems == 0) 
        {
          startTimems = mTime;
           #ifdef UseLCD
              lcd.clear();
           #endif
        }
        timeTakenus = uTime - lastCalcChangeus;
        rpmHistory[nextRPM] = (float)(60000000.0*clicksPerCalc/clicksPerRotation)/timeTakenus;
        nextRPM ++;
        if(nextRPM >=numRpms) nextRPM = 0;//wrap around to the start again.
        const int rpmarraycount = 3;
        float rpms[rpmarraycount] = {getRpm(0), getRpm(-1), getRpm(-2)};//,getRpm(-3),getRpm(-4)};
        float currentmedianrpm = median(rpms,rpmarraycount);
        if(currentmedianrpm > peakRPM) peakRPM = currentmedianrpm;
        float radSec = (float)currentmedianrpm/60*2*PI;
        float instpow = calculateInstantaneousPower();
        //Serial.println(nextinstantaneousrpm);
        if(instpow > 10)//at least 10 watts input power
          { //on first acceleration - work out the total time decelerating.
            accelerations ++;
            if(accelerations == 1 && decelerations > consecutivedecelerations) 
              {//first acceleration - capture the seconds decelerating
                nextPower = 0;
                previousSecondsDecel = secondsDecel;
              }
            storeInstantaneousPower(instpow);
            if(accelerations == consecutiveaccelerations && decelerations > consecutivedecelerations)
              {//beginning of drive /end recovery - we have been consistently decelerating and are now consistently accelerating
                totalStroke++;
                getDragFactor();
                decelerations = 0;
                driveStartclicks = clicks;
                                //recovery is the stroke minus the drive, drive is just drive
                recoveryToDriveRatio = (float)(previousSecondsDecel) / ((float)lastDriveTimems/1000);
            #ifdef debug
                Serial.println(); Serial.println(secondsDecel); Serial.println(((float)strokems/1000)-secondsDecel); Serial.println(recoveryToDriveRatio);
            #endif
                //safest time to write a screen or to serial (slowest rpm)
              }
              else if(accelerations > consecutiveaccelerations)
              {
                //get the angular velocity before the change. 
                //set the drive angular velocity to be the value it was 4 clicks ago (before any deceleration
//                driveAngularVelocity = radSec;//(float)getRpm(-consecutiveaccelerations-1)/60*2*PI;
                driveEndms = mTime;
                lastDriveTimems = driveEndms - recoveryEndms;
                //driveAngularVelocity = radSec;//and start monitoring the next drive (make the drive angular velocity low,
                decelerations = 0;
                peakRPM = 0;
              }
          }
          else
          {
              if(decelerations ==0 && accelerations > consecutiveaccelerations)
              {
                //first deceleration
                diffclicks = clicks - lastStrokeClicks;
                strokems = mTime - lastStrokeTimems;
                spm = 60000 /strokems;
                lastStrokeClicks = clicks;
                lastStrokeTimems = mTime;
                split =  ((float)strokems)/((float)diffclicks*mPerClick*2) ;//time for stroke /1000 for ms *500 for 500m = /(*2)
                writeStrokeRow();
                #ifdef recoverywork
                  doRecoveryWork();
                  unsigned long timeAfter = micros();
                  unsigned long timeTakenus = uTime - timeAfter;
                #endif
              }
              decelerations ++;
              if(decelerations == consecutivedecelerations)
              {//still decelerating (more than three decelerations in a row).
//                previousDriveAngularVelocity = driveAngularVelocity;    //store the previous deceleration
                recoveryBeginms = mTime;
                recoveryBeginAngularVelocity = radSec;
                power = 2.8 / pow((split / 500),3.0);//watts = 2.8/(split/500)³ (from concept2 site)
                //Serial.print(split);
                //store the drive speed
                accelerations = 0;//reset accelerations counter
              }
              else if(decelerations > consecutivedecelerations)
              {
                driveLengthm = (float)(clicks - driveStartclicks) * mStrokePerRotation;
                accelerations = 0;//reset accelerations counter
                recoveryEndAngularVelocity = radSec;
                recoveryEndms = mTime;
                secondsDecel = (float)((float)recoveryEndms- recoveryBeginms)/1000;
                addDragFactorToArray();
              }
          }
          if(mPerClick <= 20 && mPerClick >=0)
          {
            distancem += (clicks-clicksInDistance)*mPerClick;
            clicksInDistance = clicks;
          }
          lastRotationus = timeTakenus;
          lastCalcChangeus = uTime;
          if(sessionType== DISTANCE)
          {
            if(distancem >= targetDistance)
            {
              writeStrokeRow();
              monitorEnabled = false;
            }
          }
    if((mTime-startTimems)/1000 > targetSeconds)
    {
      switch(sessionType)
      {
        case INTERVAL:
          intervalDistances[intervals] = distancem -intervalDistances[intervals-1];
          if(intervals < numIntervals)
          {
            showInterval(intervalSeconds); 
            //then reset the start time for count down to now for the next interval.
            startTimems = millis();
          }
          else
          {//stop.
            Serial.println(F("Done"));
            #ifdef UseLCD
            reviewIntervals();
            #endif
          }
          
          intervals ++;
          break;
        case TIME:
          Serial.println(F("Done"));
          while(true);
          break;
        default:
        //default = = do nothing.
        break;
      }
    }
  }
  }
}

//gets the time to display in the format 00:00 - this is either the time left in a Time session, or the time passed in a normal session.
String getTime()
{
  int timemins, timeseconds;
  String timeString = "";
        if(sessionType == TIME)
        {//count-down from the target time.
          timemins = (targetSeconds - (mTime-startTimems)/1000)/60;
          if(timemins < 0) timemins = 0;
          timeseconds = (targetSeconds - (mTime-startTimems)/1000) - timemins * 60 ;
          if(timeseconds < 0) timeseconds = 0;
        }
        else
        {
          timemins = (mTime-startTimems)/60000;
          timeseconds = (long)((mTime)-startTimems)/1000 - timemins*60;
        }
          if(timemins <10) timeString += "0";
          timeString += timemins;//total mins
          timeString += ":";
          if(timeseconds < 10) timeString +="0";
          timeString += timeseconds;//total seconds.*/
          return timeString;
}

//take time and display how long remains on the screen.
void showInterval(long numSeconds)
{
  #ifdef UseLCD
  lcd.clear();
  lcd.setCursor(0,0);
  #endif
  Serial.print("Interval ");
  Serial.println(intervals);
  long startTime = millis()/1000;
  long currentTime = millis()/1000;
  while(startTime + numSeconds > currentTime)
  {
    delay(200);
    currentTime = millis()/1000;
    #ifdef UseLCD
      writeTimeLeft(startTime+numSeconds-currentTime);
    #endif
    //this wont work until we pull intervals out of this loop.
    //statusStr = "Interval%20" + startTime+numSeconds-currentTime + "%20seconds";
  }
  Serial.println(F("Interval Over"));
}

//convert the split variable into a string of format 00:00.00
String getSplitString()
{
  String splitString = "";
  int splitmin = (int)(split/60);
  int splits = (int)(((split-splitmin*60)));
  int splittenths = (int)((((split-splitmin*60)-splits))*10);
  splitString += splitmin;
  splitString += ":";
  if(splits <10) splitString += "0";
  splitString += splits;
  splitString +=".";
  splitString +=splittenths;
  return splitString;
}

//write all the rpm history that we have to serial
void dumprpms()
{
    Serial.println(F("Rpm dump"));
    for(int i = 0; i < numRpms; i++)
    {
      Serial.println(rpmHistory[i]);
    }
    //nextRPM = 0;
}





//Get the rpm offset rotations ago ( getRpm(-1) gets the previous rpm)
int getRpm(short offset)
{
  if(offset >0) 
  {
    Serial.println(F("Warning, rpm in the future requested."));
  }
  int index = nextRPM - 1 + offset;
    while (index >= numRpms)
    {
      index -= numRpms;
    }
    while(index < 0) 
    {
      index += numRpms;
    }
  return rpmHistory[index];
}

void resetSession()
{
  distancem = 0;
  raceStartTimems = 0;
  startTimems = 0;
  lastStrokeClicks = 0;
  lastStrokeTimems = 0;
  driveEndms = 0;
  recoveryEndms = 0;
  lastDriveTimems = 0;
  secondsDecel = 0;
  previousSecondsDecel = 0;
  lastCalcChangeus =0;
  previousSecondsDecel = 0;
  statusStr = "";
  totalStroke = 0;
  split = 0;                            // split time for last stroke in seconds
  power = 0;                            // last stroke power in watts
  spm = 0;                               // current strokes per minute.  
  distancem = 0;                        // distance rowed in meters.
  recoveryToDriveRatio = 0;
}

void detectMachine()
{
//  if(analogRead(analogPin) == 0 & digitalRead(switchPin) ==  HIGH) 
//  {//Concept 2 - set I and flag for analogRead.
//    setErgType(ERGTYPEC2);
//    Serial.print("Concept 2 detected on pin ");
//    Serial.println(analogPin);
//  }
//  else if(analogRead(analogPin) == 0 & digitalRead(switchPin) ==  LOW) 
//  {
//    setErgType(ERGTYPEC2GEN);
//    Serial.print("Concept 2 with generator detected on pin ");
//    Serial.println(analogPin);
//  }
//  else
//  {
    setErgType(ERGTYPEVFIT);
    Serial.print(F("No Concept 2 detected on Analog pin "));
    Serial.println(analogPin);
    Serial.print(F("Detecting reed switch on pin "));
    Serial.println(switchPin);
//  }
  #ifdef SimulateRower
    setErgType(ERGTYPEC2);
  #endif
}
