//-------------------------------------------------------------------
//               clicks
unsigned long clicks = 0;                   // number of clicks since start
unsigned long clicksInDistance = 0;         // number of clicks already accounted for in the distance.
unsigned int diffclicks;                             // clicks from last stroke to this one
unsigned long driveStartclicks;             // number of clicks at start of drive.
byte currentrot = 0;                       // current rotation (for number of clicks per calculation)
//-------------------------------------------------------------------
//               global constants
float c = 2.8;                              //The figure used for c is somewhat arbitrary - selected to indicate a 'realistic' boat speed for a given output power. c/p = (v)^3 where p = power in watts, v = velocity in m/s  so v = (c/p)^1/3 v= (2.8/p)^1/3
                                            //Concept used to quote a figure c=2.8, which, for a 2:00 per 500m split (equivalent to u=500/120=4.17m/s) gives 203 Watts. 
//-------------------------------------------------------------------
//               rpm/angular Velocity
float previousDriveAngularVelocity;         // fastest angular velocity at end of previous drive
//float driveAngularVelocity;                 // fastest angular velocity at end of drive
float recoveryBeginAngularVelocity;              // angular velocity at the end of the recovery  (before drive)
float recoveryEndAngularVelocity;
float previousRecoveryEndAngularVelocity;
unsigned long previousRecoveryEndms;
unsigned long recoveryBeginms; 
//-------------------------------------------------------------------
//               acceleration/deceleration
const unsigned int consecutivedecelerations = 2;//number of consecutive decelerations before we are decelerating
const unsigned int consecutiveaccelerations = 2;// number of consecutive accelerations before detecting that we are accelerating.
unsigned int decelerations = consecutivedecelerations +1;             // number of decelerations detected.
unsigned int accelerations = 0;             // number of acceleration rotations;
//-------------------------------------------------------------------
//               drag factor 
//int k3 = 0, k2 = 0, k1 = 0;                 // previous k values for smoothing   
int kIndex = 0;     
const int maxKArray = 60;                   
int kArray[maxKArray];                                  // array of k values (so we can get a median when we are done.
float mPerClick = 0;                        // meters per rotation of the flywheel (calculated from drag factor)

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

void setErgType(short newErgType)
{
  ergType = newErgType;
  switch(newErgType)
  {
    case ERGTYPEVFIT:
        //V-Fit rower with tach.
        analogSwitch = false;
        I = 0.03;
        clicksPerRotation = 1;
        clicksPerCalc = 3;
        k = 0.000085;  
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

void calculateInstantaneousPower()
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
  if(nextPower < powerSamples && instantaneouspower > 0)
  {
    Serial.println(instantaneouspower);
    powerArray[nextPower] = instantaneouspower;
    nextPower++;
  }
  else
  {
    if(nextPower >= powerSamples)
    {
      Serial.println(F("More samples than power array"));
    }
    else
    {
      Serial.println(instantaneouspower);
      //negative power
    }
  }
}

void addDragFactorToArray()
{
  float nextk = I * ((1.0/recoveryEndAngularVelocity)-(1.0/recoveryBeginAngularVelocity))/(secondsDecel)*1000000;
  if(nextk >40 && nextk < 300 && kIndex < maxKArray)
  {
    kArray[kIndex] = nextk;
    kIndex ++;
  }
  else
  {//dodgy k - write out to serial if debug.
  #ifdef debug
    Serial.print(F("k:")); Serial.print(nextk); Serial.print(F("recoverybeginrad")); Serial.print(recoveryBeginAngularVelocity); Serial.print(F("recoveryend")); Serial.print(recoveryEndAngularVelocity); Serial.print(F("recoverySeconds")); Serial.print(secondsDecel);
    Serial.print(" last Recovery time "); Serial.println(previousSecondsDecel);
//    Serial.print(F("k:")); Serial.println(nextk); Serial.print(F("recw:")); Serial.println(recoveryBeginAngularVelocity); Serial.print(F("dw")); Serial.println(driveAngularVelocity); 
    Serial.print(F("peakRPM")); Serial.println(peakRPM); Serial.print(F("sdecel")); Serial.println(secondsDecel);
  #endif
  }
}

void getDragFactor()
{
  if(kIndex > 5)
  {//more than 5 values so calculate k
    k = (float)median(kArray,kIndex)/1000000;  //get a median of all the k calculations
    mPerClick = pow((k/c),(0.33333333333333333))*2*PI/clicksPerRotation;//v= (2.8/p)^1/3  
    Serial.println();
    Serial.println();
    Serial.print("K MEDIAN"); Serial.println(k*1000000); Serial.print("samples "); Serial.println(kIndex);
    Serial.println();
    Serial.println();
    kIndex = 0;
  }
}

void registerClick()
{
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
        rpmHistory[nextRPM] = (60000000.0*clicksPerCalc/clicksPerRotation)/timeTakenus;
        nextRPM ++;
        if(nextRPM >=numRpms) nextRPM = 0;//wrap around to the start again.
        const int rpmarraycount = 3;
        int rpms[rpmarraycount] = {getRpm(0), getRpm(-1), getRpm(-2)};//,getRpm(-3),getRpm(-4)};
        int currentmedianrpm = median(rpms,rpmarraycount);
        int rpms2[rpmarraycount] = {getRpm(-3), getRpm(-4),getRpm(-5)};//,getRpm(-8),getRpm(-9)};
        int previousmedianrpm = median(rpms2, rpmarraycount);
        
        if(currentmedianrpm > peakRPM) peakRPM = currentmedianrpm;
        float radSec = (float)currentmedianrpm/60*2*PI;
        float prevradSec = (float)previousmedianrpm/60*2*PI;
        float angulardeceleration = (prevradSec-radSec)/((float)timeTakenus/1000000.0);
        //Serial.println(nextinstantaneousrpm);
        if(radSec > prevradSec || (accelerations > consecutiveaccelerations && radSec == prevradSec))//faster, or previously going faster and the same rpm
          { //on first acceleration - work out the total time decelerating.
            accelerations ++;
            if(accelerations == 1 && decelerations > consecutivedecelerations) 
              {//first acceleration - capture the seconds decelerating
                nextPower = 0;
                previousSecondsDecel = secondsDecel;
              }
            calculateInstantaneousPower();
            if(accelerations == consecutiveaccelerations && decelerations > consecutivedecelerations)
              {//beginning of drive /end recovery - we have been consistently decelerating and are now consistently accelerating
                totalStroke++;
                writeStrokeRow();
                getDragFactor();
                decelerations = 0;
                driveStartclicks = clicks;
                                //recovery is the stroke minus the drive, drive is just drive
                recoveryToDriveRatio = (float)(secondsDecel) / ((float)lastDriveTimems/1000);
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
                diffclicks = clicks - lastStrokeClicks;
                strokems = mTime - lastStrokeTimems;
                spm = 60000 /strokems;
                lastStrokeClicks = clicks;
                lastStrokeTimems = mTime;
                split =  ((float)strokems)/((float)diffclicks*mPerClick*2) ;//time for stroke /1000 for ms *500 for 500m = /(*2)
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
    if((mTime-startTimems)/1000 > targetSeconds)
    {
      switch(sessionType)
      {
        case INTERVAL:
          if(intervals <= numIntervals)
          {
            showInterval(intervalSeconds); 
            //then reset the start time for count down to now for the next interval.
            startTimems = millis();
          }
          else
          {//stop.
            Serial.println(F("Done"));
            while(true);
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
  Serial.print(F("Interval "));
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
  }
  Serial.println(F("Interval Over"));
}

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


void dumprpms()
{
    Serial.println(F("Rpm dump"));
    for(int i = 0; i < numRpms; i++)
    {
      Serial.println(rpmHistory[i]);
    }
    //nextRPM = 0;
}




int median(int new_array[], int num){
     //ARRANGE VALUES
    for(int x=0; x<num; x++){
         for(int y=0; y<num-1; y++){
             if(new_array[y]>new_array[y+1]){
                 int temp = new_array[y+1];
                 new_array[y+1] = new_array[y];
                 new_array[y] = temp;
             }
         }
     }
    //CALCULATE THE MEDIAN (middle number)
    if(num % 2 != 0){// is the # of elements odd?
        int temp = ((num+1)/2)-1;
        //cout << "The median is " << new_array[temp] << endl;
  return new_array[temp];
    }
    else{// then it's even! :)
        //cout << "The median is "<< new_array[(num/2)-1] << " and " << new_array[num/2] << endl;
  return ((new_array[(num/2)-1] + new_array[num/2]) / 2); 
    }
}

int getRpm(short offset)
{
  if(offset >0) 
  {
    Serial.println("Warning, rpm in the future requested.");
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
