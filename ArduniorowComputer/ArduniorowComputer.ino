/* Arduino row Computer
 * Uses an UNO + LCD keypad shield like this: http://www.lightinthebox.com/16-x-2-lcd-keypad-shield-for-arduino-uno-mega-duemilanove_p340888.html?currency=GBP&litb_from=paid_adwords_shopping
 * principles behind calculations are here : http://www.atm.ox.ac.uk/rowing/physics/ergometer.html#section7
 * 13% memory used for LCD / menu
 * 41% total
 */

#include <avr/sleep.h>
#include <LiquidCrystal.h>
#define UseLCD // comment out this line to not use a 16x2 LCD keypad shield, and just do serial reporting.

// when we use esp8266... https://www.bountysource.com/issues/27619679-request-event-driven-non-blocking-wifi-api
// initialize the library with the numbers of the interface pins

//end of changes to resolution.

//code for keypad:
static int DEFAULT_KEY_PIN = 0; 
static int UPKEY_ARV = 144; //that's read "analogue read value"
static int DOWNKEY_ARV = 329;
static int LEFTKEY_ARV = 505;
static int RIGHTKEY_ARV = 0;
static int SELKEY_ARV = 721;
static int NOKEY_ARV = 1023;
static int _threshold = 50;

//KEY index definitions
#define NO_KEY 0
#define UP_KEY 3
#define DOWN_KEY 4
#define LEFT_KEY 2
#define RIGHT_KEY 5
#define SELECT_KEY 1

//session(menu) definitions
#define JUST_ROW 0
#define DISTANCE 1
#define TIME 2
#define DRAGFACTOR 3
#define RPM 4
#define SETTINGS 5
#define BACKLIGHT 6
#define ERGTYPE 7
#define BOATTYPE 8
#define WEIGHT 9
#define POWEROFF 10
#define BACK 11

//erg type definitions
#define ERGTYPEVFIT 0 // V-Fit air rower.
#define ERGTYPEC2 1 //Concept 2

//boat type defintions.
#define BOAT4 0
#define BOAT8 1
#define BOAT1 2

short ergType = ERGTYPEVFIT;
short boatType = BOAT4;

long targetDistance = 2000;

int targethours = 0;
int targetmins = 20;
int targetseconds = 0;

int sessionType = JUST_ROW;
//end code for keypad

//int backLight = 13;

const int switchPin = 2;                    // switch is connected to pin 6
const int analogPin = 1;                    // analog pin (Concept2)

int peakrpm = 0;

bool AnalogSwitch = false;                            // if we are connected to a concept2
float AnalogSwitchlim = 10;                             // limit to detect a rotation from C2  (0.00107421875mV per 1, so 40 = 42mV

int clicksPerRotation = 1;                  // number of magnets , or clicks detected per rotation.

unsigned long lastlimittime = 0;
float AnalogSwitchMax = 10;
float AnalogSwitchMin = 0;
int lastAnalogSwitchValue = 0;                        // the last value read from the C2

int val;                                    // variable for reading the pin status
int buttonState;                            // variable to hold the button state
short numclickspercalc = 3;        // number of clicks to wait for before doing anything, if we need more time this will have to be reduced.
short currentrot = 0;

unsigned long utime;                        // time of tick in microseconds
unsigned long mtime;                        // time of tick in milliseconds

unsigned long laststatechangeus;            // time of last switch.
unsigned long timetakenus;                  // time taken in milliseconds for a rotation of the flywheel
unsigned long lastrotationus;               // milliseconds taken for the last rotation of the flywheel
unsigned long startTimems = 0;              // milliseconds from startup to first sample
unsigned long clicks = 0;                   // number of clicks since start
unsigned long laststrokeclicks = 0;         // number of clicks since last drive
unsigned long laststroketimems = 0;         // milliseconds from startup to last stroke drive
unsigned long strokems;                     // milliseconds from last stroke to this one
unsigned long clicksInDistance = 0;         // number of clicks already accounted for in the distance.
float driveLengthm = 0;                     // last stroke length in meters
static const short numRpms = 100;
int rpmhistory[numRpms];                      // array of rpm per rotation for debugging
unsigned long microshistory[numRpms];           // array of the amount of time taken in calc/display for debugging.

short nextrpm = 0;                              // currently measured rpm, to compare to last.

float driveAngularVelocity;                 // fastest angular velocity at end of drive
bool afterfirstdecrotation = false;         // after the first deceleration rotation (to give good figures for drag factor);
int diffclicks;                             // clicks from last stroke to this one

int screenstep=0;                           // int - which part of the display to draw next.
int k3 = 185;
int k2 = 185;
int k1 = 185;
float k = 0.000185;                         //  drag factor nm/s/s (displayed *10^6 on a Concept 2) nm/s/s == W/s/s
                                            //  The displayed drag factor equals the power dissipation (in Watts), at an angular velocity of the flywheel of 100 rad/sec.
                                            
float c = 2.8;                              //The figure used for c is somewhat arbitrary - selected to indicate a 'realistic' boat speed for a given output power. c/p = (v)^3 where p = power in watts, v = velocity in m/s  so v = (c/p)^1/3 v= (2.8/p)^1/3
                                            //Concept used to quote a figure c=2.8, which, for a 2:00 per 500m split (equivalent to u=500/120=4.17m/s) gives 203 Watts. 
                                            
float mPerClick = 0;                        // meters per rotation of the flywheel

unsigned long driveStartclicks;             // number of clicks at start of drive.
float mStrokePerRotation = 0;               // meters of stroke per rotation of the flywheel to work out how long we have pulled the handle in meters from clicks.

bool Accelerating;                          //whether or not we were accelerating at the last tick

unsigned long driveEndms;                   // time of the end of the last drive
unsigned long driveBeginms;                 // time of the start of the last drive
unsigned int lastDriveTimems;               // time that the last drive took in milliseconds

//Stats for display
float split;                                // split time for last stroke in seconds
unsigned long spm = 0;                      // current strokes per minute.  
float distancem = 0;                        // distance rowed in meters.
float RecoveryToDriveRatio = 0;                // the ratio of time taken for the whole stroke to the drive , should be roughly 3:1

//Constants that vary with machine:
float I = 0.04;                             // moment of  interia of the wheel - 0.1001 for Concept2, ~0.05 for V-Fit air rower.*/;

#ifdef UseLCD
  LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

void setup() 
{
   pinMode(switchPin, INPUT_PULLUP);        // Set the switch pin as input
   Serial.begin(115200);                    // Set up serial communication at 115200bps
   buttonState = digitalRead(switchPin);    // read the initial state
   // set up the LCD's number of columns and rows: 
   #ifdef UseLCD
    lcd.begin(16, 2);  
    lcd.clear();
   #endif
  //pinMode(backLight, OUTPUT);
  //digitalWrite(backLight, HIGH); // turn backlight on. Replace 'HIGH' with 'LOW' to turn it off.
  analogReference(DEFAULT);//analogReference(INTERNAL);
  delay(100);
  if(analogRead(analogPin) == 0 & digitalRead(switchPin) ==  HIGH) 
  {//Concept 2 - set I and flag for analogRead.
    setErgType(ERGTYPEC2);
    Serial.println("Concept 2 detected on pin 3");
  }
  else
  {
    setErgType(ERGTYPEVFIT);
    Serial.println("No Concept 2 detected on Analog pin 3");
  }

  #ifdef UseLCD
    startMenu();
  #endif
  // Print a message to the LCD.
}

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
        AnalogSwitch = false;
        I = 0.05;
        clicksPerRotation = 1;
        numclickspercalc = 1;
        k3 = 85;
        k2 = 85;
        k1 = 85;
        k = 0.000085;  
        mStrokePerRotation = 0;//meters of stroke per rotation of the flywheel - V-fit.
        break;
    default:
        AnalogSwitch = true;
        I = 0.101;
        //do the calculations less often to allow inaccuracies to be averaged out.
        numclickspercalc = 1;//take out a lot of noise before we detect drive / recovery.
        //number of clicks per rotation is 3 as there are three magnets.
        clicksPerRotation = 3;
        mStrokePerRotation = 0;//meters of stroke per rotation of the flywheel - C2.
        ergType = ERGTYPEC2;    
        break;
  }
}

void loop()
{
  mtime = millis();
  utime = micros(); 
  if(AnalogSwitch)
  {
    //simulate a reed switch from the coil
    int analog = analogRead(analogPin);
    //make AnalogSwitchlim a running 4 second average of the sample.
//    if(micros() > utime && utime > 0)
//    {//if this isn't an overflow, and there has been at least one sample, start to tune the C2lim.
//      //C2lim = C2lim + ((float)val - C2lim)*((float)(micros()-utime)/4000000);
      if(analog > AnalogSwitchMax) AnalogSwitchMax = analog;
      if(analog < AnalogSwitchMin) AnalogSwitchMin = analog;
      if(mtime > lastlimittime + 1000)//tweak the limits each second
      {
        lastlimittime = mtime;
        AnalogSwitchlim = (AnalogSwitchMax + AnalogSwitchMin)/2;
        //reset the max/min
        AnalogSwitchMin = analog;
        AnalogSwitchMax = analog;
      }
//    }
    if(AnalogSwitchlim < 5) AnalogSwitchlim = 5;//don't let it get back to zero if nothing is happening...
    //if(analog > AnalogSwitchlim) 
    
    //detect rising side
    if(buttonState == LOW)
    {
      if(analog < (float)lastAnalogSwitchValue*0.9 || analog == 0)
      {
        val = HIGH;
      }
    }
    else
    {//ButtonState == HIGH
      if(analog > (float)(lastAnalogSwitchValue+2))
      {
        val = LOW;//detected it starting to pass
      }
    }

    //detect dropping side
    
    lastAnalogSwitchValue = analog;
  }
  else
  {
    val = digitalRead(switchPin);            // read input value and store it in val                       
  }
       if (val != buttonState && val == LOW && (utime- laststatechangeus) >10000)            // the button state has changed!
          { 
            currentrot ++;
            clicks++;
            if(currentrot >= numclickspercalc)
            {
              currentrot = 0;
              //initialise the start time
              if(startTimems == 0) 
              {
                startTimems = mtime;
                 #ifdef UseLCD
                    lcd.clear();
                 #endif
              }
              timetakenus = utime - laststatechangeus;
              //Serial.print("TimeTaken(ms):");
              //Serial.println(timetakenms);
              rpmhistory[nextrpm] = (60000000.0*numclickspercalc/clicksPerRotation)/timetakenus;
              nextrpm ++;
              if(nextrpm >=numRpms) nextrpm = 0;//wrap around to the start again.
              int currentmedianrpm = median_of_3(getRpm(0), getRpm(-1), getRpm(-2));
              int previousmedianrpm = median_of_3(getRpm(-1), getRpm(-2), getRpm(-3));
              if(currentmedianrpm > peakrpm) peakrpm = currentmedianrpm;
              float radSec = currentmedianrpm/60*6.283185307;//(6.283185307*numclickspercalc/clicksPerRotation)/((float)timetakenus/1000000.0);
              float prevradSec = previousmedianrpm/60*6.283185307;//(6.283185307*numclickspercalc/clicksPerRotation)/((float)lastrotationus/1000000.0);
              float angulardeceleration = (prevradSec-radSec)/((float)timetakenus/1000000.0);
              dumprpms();
              //Serial.println(nextinstantaneousrpm);
              if(currentmedianrpm > previousmedianrpm*1.05)
                { //lcd.print("Acc");        
                    if(!Accelerating)
                    {//beginning of drive /end recovery
                      Serial.println("acceleration");
                      Serial.println(previousmedianrpm);
                      Serial.println(currentmedianrpm);
                      driveBeginms = mtime;
                      float secondsdecel = ((float)mtime-(float)driveEndms)/1000;
                      if(I * ((1.0/radSec)-(1.0/driveAngularVelocity))/(secondsdecel) > 0)
                      {//if drag factor detected is positive.
                        k3 = k2; k2 = k1;
                        k1 = I * ((1.0/radSec)-(1.0/driveAngularVelocity))/(secondsdecel)*1000000;  //nm/s/s == W/s/s
                        k = (float)median_of_3(k1,k2,k3)/1000000;  //adjust k by half of the difference from the last k
                        mPerClick = pow((k/c),(0.33333333333333333))*2*3.1415926535/clicksPerRotation;//v= (2.8/p)^1/3  
                      }
                      driveStartclicks = clicks;
                      //safest time to write a screen or to serial (slowest rpm)
                    }
                    driveAngularVelocity = radSec;
                    driveEndms = mtime;
                    lastDriveTimems = driveEndms - driveBeginms;
                    //recovery is the stroke minus the drive, drive is just drive
                    RecoveryToDriveRatio = (strokems-lastDriveTimems) / lastDriveTimems;
                    afterfirstdecrotation = false;
                    Accelerating = true;    
                }
//                else if(currentmedianrpm <= ((float)previousmedianrpm *((((float)numclickspercalc-1)*1.2)/((float)numclickspercalc))) && numclickspercalc < 5)
//                {        //looks like we missed a click - add it in but skip calculations for this go
//                  Serial.println("missed a click");
//                  clicks++;
//                  //reduce sampled instantaneousrpm to allow next sample to work if we were decelerating
//                  if(!Accelerating) 
//                  {
//                    instantaneousrpm *= 0.95;
//                  }
//                  else
//                  {//safe to reduce it faster
//                    instantaneousrpm = instantaneousrpm *0.7;
//                  }
//                  //don't store the previous rpm
//                }
                else if(currentmedianrpm <= (previousmedianrpm*0.98))
                {
                    if(Accelerating)//previously accelerating
                    { //finished drive
                    Serial.println("Decel");
                    Serial.println(previousmedianrpm);
                    //Serial.print(" ");
                    Serial.println(currentmedianrpm);
                      //Serial.println("ACC");
                      diffclicks = clicks - laststrokeclicks;
                      strokems = mtime - laststroketimems;
                      spm = 60000 /strokems;
                      laststrokeclicks = clicks;
                      laststroketimems = mtime;
                      split =  ((float)strokems)/((float)diffclicks*mPerClick*2) ;//time for stroke /1000 for ms *500 for 500m = /(*2)
                      //Serial.print(split);
                      driveLengthm = (float)(clicks - driveStartclicks) * mStrokePerRotation;
                    }
                    Accelerating = false;           
                }
                
                if(mPerClick <= 20)
                {
                  distancem += (clicks-clicksInDistance)*mPerClick;
                  clicksInDistance = clicks;
                }
                //if we are spinning slower than 10ms per spin then write the next screen
                if(timetakenus > 10000) writeNextScreen();
                lastrotationus = timetakenus;
                //watch out for integer math problems here
                //Serial.println((nextinstantaneousrpm - instantaneousrpm)/timetakenms); 
                laststatechangeus=utime;
            } 
          }

          if((millis()-mtime) >=6)
          {
            Serial.print("warning - loop took (ms):");
            Serial.println(millis()-mtime);
            Serial.print("screen:");
            Serial.println(screenstep);
          }
//          microshistory[nextrpm] = micros()-utime;
  buttonState = val;                       // save the new state in our variable
}

void writeNextScreen()
{
    #ifdef UseLCD
        if(sessionType == DRAGFACTOR)
        {
          lcd.clear();
          lcd.print("Drag Factor");
          lcd.setCursor(0,1);
          lcd.print(k*1000000);
          return;//no need for other screen stuff.
        }else if (sessionType == RPM)
        {
          lcd.clear();
          lcd.print("R");
          lcd.print(getRpm(0));
          lcd.setCursor(0,1);
          lcd.print("M:");
          lcd.print(median_of_3(getRpm(0), getRpm(-1), getRpm(-2)));
          return;//no need for other screen stuff.
        }
     #endif
  //Display Format:
  // 2:16  r3.1SPM:35
  //5000m     10:00
  int timemins, timeseconds, splitmin, splits, dm;
  screenstep++;
  switch(screenstep)
  {//only write a little bit to the screen to save time.
    case 0:
    //lcd.clear();
    #ifdef UseLCD
      lcd.setCursor(0,0);
    #endif
    break;
    case 1:
    {
      splitmin = (int)(split/60);
      int splits = (int)(((split-splitmin*60)));
        if(splitmin < 10)
        {//only display the split if it is less than 10 mins per 500m
          #ifdef UseLCD
            //lcd.print("S");
            lcd.print(splitmin);//minutes in split.
            lcd.print(":");
            if(splits <10) lcd.print("0");
            lcd.print(splits);//seconds
            //lcd 0->5, 0  used
          #endif
        }
        Serial.print("Split:\t");
        Serial.print(splitmin);
        Serial.print(":");
        Serial.println(splits);
    }
    break;
    case 2:
      //lcd 10->16,0 used
      #ifdef UseLCD
        lcd.setCursor(10,0);
        lcd.print("SPM:");
        lcd.print(spm);
        lcd.print("  ");
      #endif
    Serial.print("SPM:\t");
    Serial.println(spm);
    break;
    case 3:
      #ifdef UseLCD
       lcd.setCursor(0,1);
       //Distance in meters:
      if(sessionType == DISTANCE)
      {
        long distanceleft = targetDistance - distancem;
        if(distanceleft <1000) lcd.print("0");
        if(distanceleft <100) lcd.print("0");
        if(distanceleft <10) lcd.print("0");
        lcd.print((int)distanceleft);
        lcd.print("m");
      }
      else
      {
        if(distancem <1000) lcd.print("0");
        if(distancem <100) lcd.print("0");
        if(distancem <10) lcd.print("0");
        lcd.print((int)distancem);
        lcd.print("m");
      }
      #endif
      Serial.print("Distance:\t");
      Serial.print(distancem);
      Serial.println("m");
      
      //lcd 0->5, 1 used
      //Drag factor
      /*lcd.print("D:");
      lcd.println(k*1000000);*/
    break;
    case 4:
      //lcd 11->16 used
      
      #ifdef UseLCD
        lcd.setCursor(11,1);
        if(sessionType == TIME)
        {//count-down from the target time.
          timemins = (targetmins*60 + targetseconds - (mtime-startTimems)/1000)/60;
          if(timemins < 0) timemins = 0;
          timeseconds = (targetmins*60 + targetseconds - (mtime-startTimems)/1000) - timemins * 60 ;
          if(timeseconds < 0) timeseconds = 0;
        }
        else
        {
          timemins = (mtime-startTimems)/60000;
          timeseconds = (long)((mtime)-startTimems)/1000 - timemins*60;
        }
          if(timemins <10) lcd.print("0");
          lcd.print(timemins);//total mins
          lcd.print(":");
          if(timeseconds < 10) lcd.print("0");
          lcd.print(timeseconds);//total seconds.*/
      #endif
        Serial.print("time:\t");
        Serial.print(timemins);
        Serial.print(":");
        Serial.println(timeseconds);
        //lcd.print(k);
        //Serial.println(k);
//      lcd.print("s");
//      lcd.print(diffclicks);
      break;
    case 5://next lime
    #ifdef UseLCD
      //lcd 6->9 , 0
       lcd.setCursor(5,0);
       lcd.print("r");
       lcd.print(RecoveryToDriveRatio,1);
    #endif
       Serial.print("Drag factor \t");
       Serial.println(k*1000000);
       Serial.print("anMin\t");
       Serial.println(AnalogSwitchMin);
       Serial.print("anMax\t");
       Serial.println(AnalogSwitchMax);
      //lcd.setCursor(10,1);
      //lcd.print(" AvS:");
      //lcd.print(clicks/(millis()-startTime));     
      break;
   case 6:
    //lcd 6->9 , 0
      // lcd.setCursor(4,0);
      // lcd.print("r");
      // lcd.print(RecoveryToDriveRatio);

      Serial.print("Drive angularve: ");
      Serial.println(driveAngularVelocity);

      Serial.print("rpm\t");
      Serial.println(getRpm(0));
      Serial.print("peakrpm:\t");
      Serial.println(peakrpm);
      //lcd.setCursor(10,1);
      //lcd.print(" AvS:");
      //lcd.print(clicks/(millis()-startTime));   
    break;

    default:
      screenstep = -1;//will clear next time, as 1 is added and if 0 will be cleared.
  }    
}

void dumprpms()
{
  /*
  if(nextrpm > 97)
  {
    Serial.println("Rpm dump");
    for(int i = 0; i < 190; i++)
    {
      Serial.println(rpmhistory[i]);
    }
    nextrpm = 0;
  }
  */
}

//Current selection (Distance, Time)
#ifdef UseLCD
void startMenu()
{
  menuType();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Begin Rowing");
}

void menuType()
{
  int c = NO_KEY;
  while(getKey() == SELECT_KEY) delay(300);
  writeType();
  while(c != SELECT_KEY)
  {
    c = getKey();
    if(c == DOWN_KEY)
    {
      sessionType ++;
      writeType();
      delay(500);
    }
    else if (c == UP_KEY)
    {
      sessionType --;
      writeType();
      delay(500);
    }
  }
  while(c == SELECT_KEY) c = getKey();//wait until select is unpressed.
  delay(200);
  switch(sessionType)
  {
    case DISTANCE:
      menuSelectDistance();
      break;
    case TIME: 
      menuSelectTime();
      break;
    case SETTINGS:
      menuSettings();    
      menuType();  
    default:
      //just row with that as a setting (rpm / drag factor etc..)
    break;
      
  }
}

void menuSettings()
{
  int c = NO_KEY;
  while(getKey() == SELECT_KEY) delay(300);
  writeSettingsMenu();
  while(c != SELECT_KEY)
  {
    c = getKey();
    if(c == DOWN_KEY)
    {
      sessionType ++;
      writeSettingsMenu();
      delay(500);
    }
    else if (c == UP_KEY)
    {
      sessionType --;
      writeSettingsMenu();
      delay(500);
    }
  }
  while(c == SELECT_KEY) c = getKey();//wait until select is unpressed.
  delay(200);
  switch(sessionType)
  {
    case BACKLIGHT:
      menuSelectBacklight();
      menuSettings();
      break;
    case ERGTYPE:
      menuSelectErgType();
      menuSettings();
      break;
    case BOATTYPE:
      menuSelectBoatType();
      menuSettings();
      break;
    case POWEROFF:
      menuSleep();
      menuSettings();
      break;
    default:
      //back to other menu
    break;
      
  }
}

void menuSelectBoatType()
{
  
}

void writeSettingsMenu()
{
    lcd.clear();
    while(getKey() == SELECT_KEY) delay(300);
    if(sessionType <= SETTINGS) sessionType = BACK;
    if(sessionType > BACK) sessionType = BACKLIGHT;
    switch(sessionType)
    {
      case BACKLIGHT:
        menuDisplay("Back Light");
        break;
      case ERGTYPE:
        menuDisplay("Erg Type");
        break;
      case BOATTYPE:
        menuDisplay("Boat Type");
        break;
      case WEIGHT:
        menuDisplay("Weight");
        break;
      case POWEROFF:
        menuDisplay("Sleep");
        break;
      case BACK:
        menuDisplay("Back");
        break;
    }
}

void writeType()
{
  lcd.clear();
  //rollback around.
  if(sessionType <=-1) sessionType = SETTINGS;
  if(sessionType > SETTINGS) sessionType = JUST_ROW;
    switch(sessionType)
    {
      case DISTANCE:
        menuDisplay("Distance");
      break;
      case TIME:
        menuDisplay("Time");
      break;
      case DRAGFACTOR:
        menuDisplay("Drag Factor");
        break;
      case RPM:
        menuDisplay("RPM");
        break;
      case SETTINGS:
        menuDisplay("Settings");
        break;
      default:
        sessionType = JUST_ROW;
        menuDisplay("Just Row");
        break;
    }
}

void menuSelectErgType()
{
  menuDisplayErgType();
  int c = NO_KEY;
  while(c!=SELECT_KEY)
  {
    c = getKey();
    if(c==UP_KEY) 
    {
      ergType ++;
      if(ergType > ERGTYPEC2) ergType = ERGTYPEVFIT;
      menuDisplayErgType();
      delay(500);
    }
    if(c==DOWN_KEY) 
    {
      ergType --;
      if(ergType < ERGTYPEVFIT) ergType = ERGTYPEC2;
      menuDisplayErgType();
      delay(500);
    }
    setErgType(ergType);
  }
}

void menuDisplayErgType()
{
  lcd.setCursor(0,1);
  switch(ergType)
  {
    case ERGTYPEC2:
      lcd.print("Concept 2");
      break;
    case ERGTYPEVFIT:
      lcd.print("V-Fit    ");
      break;
  }
}

//go to sleep
void menuSleep()
{
  sleep_enable();
  digitalWrite(10,LOW);
  attachInterrupt(0, pin2_isr, LOW);
  /* 0, 1, or many lines of code here */
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();
  sleep_bod_disable();
  sei();
  sleep_cpu();
  /* wake up here */
  sleep_disable();
}

//interrupt sleep routine
void pin2_isr()
{
  sleep_disable();
  detachInterrupt(0);
 // pin2_interrupt_flag = 1;
}

void menuSelectBacklight()
{
  pinMode(10,OUTPUT);
  int state = HIGH;
  digitalWrite(10,state);
  showBacklightState(state);
  int c = NO_KEY;
  while(c!=SELECT_KEY)
  {
    c = getKey();
    if(c==UP_KEY || c==DOWN_KEY)
    {
      if(state == LOW) 
      {
        state = HIGH;
      }
      else
      {
        state = LOW;
      }
      showBacklightState(state);
      digitalWrite(10,state);
      delay(500);
    }
  }
}

void showBacklightState(int state)
{
  lcd.setCursor(0,1);
  if(state == LOW)
  {
      lcd.print("Off");
  }
  else
  {
      lcd.print("On ");
  }
}

void menuSelectDistance()
{
  int c = NO_KEY;
  long increment = 1000;
  writeCurrentDistanceamount(increment);
  delay(600);
  while(c!= SELECT_KEY)
  {
    c = getKey();
    if(c==UP_KEY)
    {
      targetDistance += increment;
      if(targetDistance < 0) targetDistance = 0;
      writeCurrentDistanceamount(increment);
    }
    else if(c== DOWN_KEY)
    {
      targetDistance -= increment;
      if(targetDistance < 0) targetDistance = 0;
      writeCurrentDistanceamount(increment);
    }
    else if(c== RIGHT_KEY)
    {
      increment /=10;
      if(increment == 0) increment = 1;
      writeCurrentDistanceamount(increment);
    }
    else if(c==LEFT_KEY)
    {
      increment*=10;
      if(increment >= 10000) increment = 10000;
      writeCurrentDistanceamount(increment);
    }
  }
}

void writeCurrentDistanceamount(int increment)
{
    lcd.setCursor(0,1);
    if(targetDistance < 10000) lcd.print("0");
    if(targetDistance < 1000) lcd.print("0");
    if(targetDistance < 100) lcd.print("0");
    if(targetDistance < 10) lcd.print("0");
    lcd.print(targetDistance);
    switch(increment)
    {
       case 1:
        lcd.setCursor(4,1);
        break;
      case 10:
        lcd.setCursor(3,1);
        break;
      case 100:
        lcd.setCursor(2,1);
        break;
      case 1000:
        lcd.setCursor(1,1);
        break;
      case 10000:
        lcd.setCursor(0,1);
        break;
       default:
        lcd.setCursor(1,0);
        lcd.print(increment);
       break;
    }
    lcd.cursor();
    delay(200);
}

void menuSelectTime()
{
  int charpos = 3;
  //charpos is the current selected character on the display, 
  //0 = 10s of hours, 1 = hours, 3 = tens of minutes, 4 = minutes, 6 = tens of seconds, 7 = seconds.
  writeTargetTime(charpos);
  int c = getKey();
  while(c!= SELECT_KEY)
  {
    c = getKey();
    short incrementhours = 0;
    short incrementmins = 0;
    short incrementseconds = 0;
    switch (charpos)
    {
      case 0:
        incrementhours = 10;
        break;
      case 1://hours
        incrementhours = 1;
        break;
      case 3:
        incrementmins = 10;
        break;
      case 4:
        incrementmins = 1;
        break;
      case 6:
        incrementseconds = 10;
        break;
      case 7:
        incrementseconds = 1;
        break;
      default:
        charpos = 0;
        incrementhours = 10;
    }
    if(c==UP_KEY)
    {
      targetmins += incrementmins;
      targethours += incrementhours;
      targetseconds += incrementseconds;
      if(targetseconds > 59) 
      {
        targetseconds = 0;
        targetmins ++;
      }
      if(targetmins >59) 
      {
        targetmins = 0;
        targethours ++;
      }
      if(targethours > 24) 
      {
        targethours = 24;
      }
      delay(500);
      writeTargetTime(charpos);
    }
    else if(c== DOWN_KEY)
    {
      
      targetmins -= incrementmins;
      targethours -= incrementhours;
      targetseconds -= incrementseconds;
      if(targetseconds < 0) 
      {
        targetseconds = 0;
        targetmins --;
      }
      if(targetmins <0) 
      {
        targetmins = 0;
        targethours --;
      }
      if(targethours < 0) targethours = 0;
      delay(500);
      writeTargetTime(charpos);
    }
    else if (c== RIGHT_KEY)
    {
      charpos ++;
      if(charpos ==2 || charpos == 5) charpos ++;
      delay(500);
      writeTargetTime(charpos);
    }
    else if (c == LEFT_KEY)
    {
      charpos --;
      if(charpos ==2 || charpos == 5) charpos --;
      delay(500);
      writeTargetTime(charpos);
    }
  }
}

void writeTargetTime(int charpos)
{
    lcd.setCursor(0,1);
    if(targethours < 10) lcd.print("0");
    lcd.print(targethours);
    lcd.print(":");
    if(targetmins < 10) lcd.print("0");
    lcd.print(targetmins);
    lcd.print(":");
    if(targetseconds < 10) lcd.print("0");
    lcd.print(targetseconds);
    lcd.setCursor(charpos,1);
    lcd.cursor();
}

int getKey()
{
  int _curInput = analogRead(0);
  //Serial.println(_curInput);
  int _curKey;
  if (_curInput > UPKEY_ARV - _threshold && _curInput < UPKEY_ARV + _threshold ) _curKey = UP_KEY;
      else if (_curInput > DOWNKEY_ARV - _threshold && _curInput < DOWNKEY_ARV + _threshold ) _curKey = DOWN_KEY;
      else if (_curInput > RIGHTKEY_ARV - _threshold && _curInput < RIGHTKEY_ARV + _threshold ) _curKey = RIGHT_KEY;
      else if (_curInput > LEFTKEY_ARV - _threshold && _curInput < LEFTKEY_ARV + _threshold ) _curKey = LEFT_KEY;
      else if (_curInput > SELKEY_ARV - _threshold && _curInput < SELKEY_ARV + _threshold ) _curKey = SELECT_KEY;
      else _curKey = NO_KEY;
  return _curKey;
  delay(100);
}

//

void menuDisplay(char* text)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(text);
}
#endif

int median_of_3( int a, int b, int c ){       //Median filter
  int the_max = max( max( a, b ), c );
  int the_min = min( min( a, b ), c );
  int the_median = the_max ^ the_min ^ a ^ b ^ c;
  return( the_median );
}

int getRpm(short offset)
{
  int index = nextrpm - 1 + offset;
    while (index >= numRpms)
    {
      index -= numRpms;
    }
    while(index < 0) 
    {
      index += numRpms;
    }
  return rpmhistory[index];
}

