/* Arduino row Computer
 * Uses a 16x2 lcd to display rowing statistics on an ergometer
 * principles are here : http://www.atm.ox.ac.uk/rowing/physics/ergometer.html#section7
 * 
 */
#include <LiquidCrystal.h>
#define UseLCD // comment out this line to not use a 16x2 LCD

// when we use esp8266... https://www.bountysource.com/issues/27619679-request-event-driven-non-blocking-wifi-api
// initialize the library with the numbers of the interface pins

//end of changes to resolution.

int backLight = 13;

const int switchPin = 6;                    // switch is connected to pin 6
const int analogPin = 3;                    // analog pin (Concept2)

int peakrpm = 0;
bool C2 = false;                            // if we are connected to a concept2
float C2lim = 10;                             // limit to detect a rotation from C2  (0.00107421875mV per 1, so 40 = 42mV

int val;                                    // variable for reading the pin status
int buttonState;                            // variable to hold the button state
const short numrotationspercalc = 1;        // number of rotations to wait for before doing anything, if we need more time this will have to be reduced.
short currentrot = 0;

unsigned long utime;                        // time of tick in microseconds
unsigned long mtime;                        // time of tick in milliseconds

unsigned long laststatechangeus;            // time of last switch.
unsigned long timetakenus;                  // time taken in milliseconds for a rotation of the flywheel
float instantaneousrpm;             // rpm from the rotatiohn
float nextinstantaneousrpm;                 // next rpm reading to compare with previous
unsigned long lastrotationus;               // milliseconds taken for the last rotation of the flywheel
unsigned long startTimems = 0;              // milliseconds from startup to first sample
unsigned long rotations = 0;                // number of rotations since start
unsigned long laststrokerotations = 0;      // number of rotations since last drive
unsigned long laststroketimems = 0;         // milliseconds from startup to last stroke drive
unsigned long strokems;                     // milliseconds from last stroke to this one
unsigned long rotationsInDistance = 0;      // number of rotations already accounted for in the distance.
float driveLengthm = 0;                     // last stroke length in meters
float rpmhistory[100];                      // array of rpm per rotation for debugging
unsigned long microshistory[100];           // array of the amount of time taken in calc/display for debugging.

short nextrpm;                              // currently measured rpm, to compare to last.

float driveAngularVelocity;                // fastest angular velocity at end of drive
bool afterfirstdecrotation = false;        // after the first deceleration rotation (to give good figures for drag factor);
int diffrotations;                         // rotations from last stroke to this one

int screenstep=0;                           // int - which part of the display to draw next.
float k = 0.000185;                         //  drag factor nm/s/s (displayed *10^6 on a Concept 2) nm/s/s == W/s/s
                                            //  The displayed drag factor equals the power dissipation (in Watts), at an angular velocity of the flywheel of 100 rad/sec.
                                            
float c = 2.8;                              //The figure used for c is somewhat arbitrary - selected to indicate a 'realistic' boat speed for a given output power. c/p = (v)^3 where p = power in watts, v = velocity in m/s  so v = (c/p)^1/3 v= (2.8/p)^1/3
                                            //Concept used to quote a figure c=2.8, which, for a 2:00 per 500m split (equivalent to u=500/120=4.17m/s) gives 203 Watts. 
                                            
float mPerRot = 0;                          // meters per rotation of the flywheel

unsigned long driveStartRotations;           // number of rotations at start of drive.
float mStrokePerRotation = 0;               // meters of stroke per rotation of the flywheel to work out how long we have pulled the handle in meters from rotations.

bool Accelerating;                          //whether or not we were accelerating at the last tick

unsigned long driveEndms;                   // time of the end of the last drive
unsigned long driveBeginms;                 // time of the start of the last drive
unsigned int lastDriveTimems;               // time that the last drive took in milliseconds

//Stats for display
float split;                                // split time for last stroke in seconds
unsigned long spm = 0;                      // current strokes per minute.  
float distancem = 0;                        // distance rowed in meters.
float StrokeToDriveRatio = 0;                // the ratio of time taken for the whole stroke to the drive , should be roughly 3:1

//Constants that vary with machine:
float I = 0.04;                             // moment of  interia of the wheel - 0.1001 for Concept2, ~0.05 for V-Fit air rower.*/;

#ifdef UseLCD
  LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#endif

//change the resolution of analog read to make it faster...
// defines for setting and clearing register bits
#ifndef cbi
  #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
  #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void setup() 
{
   pinMode(switchPin, INPUT_PULLUP);                // Set the switch pin as input
   Serial.begin(115200);                      // Set up serial communication at 115200bps
   buttonState = digitalRead(switchPin);  // read the initial state
   // set up the LCD's number of columns and rows: 
   #ifdef UseLCD
    lcd.begin(16, 2);  
    lcd.print("V-Fit powered by");
    lcd.setCursor(0,1);
    lcd.print("IP Technology");
   #endif
  pinMode(backLight, OUTPUT);
  digitalWrite(backLight, HIGH); // turn backlight on. Replace 'HIGH' with 'LOW' to turn it off.

  analogReference(DEFAULT);//analogReference(INTERNAL);
  // set prescale to 16
  sbi(ADCSRA,ADPS2) ;
  cbi(ADCSRA,ADPS1) ;
  cbi(ADCSRA,ADPS0) ;
  delay(100);
  if(analogRead(analogPin) == 0 & digitalRead(switchPin) ==  HIGH) 
  {//Concept 2 - set I and flag for analogRead.
    C2 = true;
    I = 0.101;
    mStrokePerRotation = 0;//meters of stroke per rotation of the flywheel - C2.
    Serial.println("Concept 2 detected on pin 3");
  }
  else
  {
    C2 = false;
    //I = 0.101;
    mStrokePerRotation = 0;//meters of stroke per rotation of the flywheel - V-fit.
    Serial.println("No Concept 2 detected on Analog pin 3");
  }
  // Print a message to the LCD.
}

void loop()
{
  if(C2)
  {
    //simulate a reed switch from the coil
    int analog = analogRead(analogPin);
    //make C2lim a running 4 second average of the sample.
    if(micros() > utime && utime > 0)
    {//if this isn't an overflow, and there has been at least one sample, start to tune the C2lim.
      C2lim = C2lim + ((float)val - C2lim)*((float)(micros()-utime)/4000000);
    }
    if(C2lim < 5) C2lim = 5;//don't let it get back to zero if nothing is happening...
    if(analog > C2lim) 
    {
      val = LOW;
    }
    else
    {
      val = HIGH;
    }
  }
  else
  {
    mtime = millis();
    utime = micros(); 
    val = digitalRead(switchPin);            // read input value and store it in val                       
  }
       if (val != buttonState && val == LOW && (utime- laststatechangeus) >5000)            // the button state has changed!
          { 
           
            currentrot ++;
            if(currentrot >= numrotationspercalc)
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
              rotations++;
              //Serial.print("TimeTaken(ms):");
              //Serial.println(timetakenms);
              nextinstantaneousrpm = (float)(60000000.0*numrotationspercalc)/timetakenus;
              if(nextinstantaneousrpm > peakrpm) peakrpm = nextinstantaneousrpm;
              float radSec = (6.283185307*numrotationspercalc)/((float)timetakenus/1000000.0);
              float prevradSec = (6.283185307*numrotationspercalc)/((float)lastrotationus/1000000.0);
              float angulardeceleration = (prevradSec-radSec)/((float)timetakenus/1000000.0);
              nextrpm ++;
              rpmhistory[nextrpm] = nextinstantaneousrpm;
              dumprpms();
              if(nextrpm >=99) nextrpm = 0;
              //Serial.println(nextinstantaneousrpm);
              if(nextinstantaneousrpm >= instantaneousrpm)
                {

                    //lcd.print("Acc");        
                    if(!Accelerating)
                    {//beginning of drive /end recovery
                      Serial.println("acceleration");
                      Serial.println(nextinstantaneousrpm);
                      Serial.println(instantaneousrpm);
                      driveBeginms = mtime;
                      float secondsdecel = ((float)mtime-(float)driveEndms)/1000;
                      k = I * ((1.0/radSec)-(1.0/driveAngularVelocity))/(secondsdecel);  //nm/s/s == W/s/s
                      mPerRot = pow((k/c),(0.33333333333333333))*2*3.1415926535;//v= (2.8/p)^1/3  
                      driveStartRotations = rotations;
                      //safest time to write a screen or to serial (slowest rpm)
                    }
                    driveAngularVelocity = radSec;
                    driveEndms = mtime;
                    lastDriveTimems = driveEndms - driveBeginms;
                    StrokeToDriveRatio = (strokems / lastDriveTimems);
                    afterfirstdecrotation = false;
                    Accelerating = true;    
                    instantaneousrpm = nextinstantaneousrpm;
                }
                else if(nextinstantaneousrpm <= instantaneousrpm *(1.0/(numrotationspercalc+1)))
                {        //looks like we missed a rotation
                  Serial.println("missed a rotation");
                  rotations++;
                }
                else if(nextinstantaneousrpm <= (instantaneousrpm*0.98))
                {
                    if(Accelerating)//previously accelerating
                    { //finished drive
                    Serial.println("Decel");
                    Serial.println(nextinstantaneousrpm);
                    //Serial.print(" ");
                    Serial.println(instantaneousrpm);
                      //Serial.println("ACC");
                      diffrotations = rotations - laststrokerotations;
                      strokems = mtime - laststroketimems;
                      spm = 60000 /strokems;
                      laststrokerotations = rotations;
                      laststroketimems = mtime;
                      split =  ((float)strokems)/((float)diffrotations*mPerRot*2) ;//time for stroke /1000 for ms *500 for 500m = /(*2)
                      //Serial.print(split);
                      driveLengthm = (float)(rotations - driveStartRotations) * mStrokePerRotation;
                    }
                    Accelerating = false;
                    instantaneousrpm = nextinstantaneousrpm;             
                }
                
                if(mPerRot <= 20)
                {
                  distancem += (rotations-rotationsInDistance)*mPerRot;
                  rotationsInDistance = rotations;
                }
                if(timetakenus > 10000) writeNextScreen();
                lastrotationus = timetakenus;
                instantaneousrpm = nextinstantaneousrpm;
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
  //Display Format:
  //2:16 r3  SPM:35
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
        if(distancem <1000) lcd.print("0");
        if(distancem <100) lcd.print("0");
        if(distancem <10) lcd.print("0");
        lcd.print((int)distancem);
        lcd.print("m");
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
      timemins = (mtime-startTimems)/60000;
      #ifdef UseLCD
        lcd.setCursor(11,1);
        if(timemins <10) lcd.print("0");
        lcd.print(timemins);//total mins
        lcd.print(":");
        timeseconds = (long)((mtime)-startTimems)/1000 - timemins*60;
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
//      lcd.print(diffrotations);
      break;
    case 5://next lime
    #ifdef UseLCD
      //lcd 6->9 , 0
       lcd.setCursor(4,0);
       lcd.print("r");
       lcd.print(StrokeToDriveRatio);
    #endif
       Serial.print("Drag factor \t");
       Serial.println(k*1000000);
      //lcd.setCursor(10,1);
      //lcd.print(" AvS:");
      //lcd.print(rotations/(millis()-startTime));     
      break;
   case 6:
    //lcd 6->9 , 0
      // lcd.setCursor(4,0);
      // lcd.print("r");
      // lcd.print(StrokeToDriveRatio);

      Serial.print("Drive angularve: ");
      Serial.println(driveAngularVelocity);

      Serial.print("rpm\t");
      Serial.println(instantaneousrpm);
      Serial.print("peakrpm:\t");
      Serial.println(peakrpm);
      //lcd.setCursor(10,1);
      //lcd.print(" AvS:");
      //lcd.print(rotations/(millis()-startTime));   
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


