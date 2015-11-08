#include <LiquidCrystal.h>
// http://www.atm.ox.ac.uk/rowing/physics/ergometer.html#section7
// read reed switch from rowing machine.
// based on Lady Ada's "debounced blinking bicycle light" code
// when we use esp8266... https://www.bountysource.com/issues/27619679-request-event-driven-non-blocking-wifi-api
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

float mPerRot = 0.301932;                   // meters per rotation of the flywheel
int switchPin = 6;                          // switch is connected to pin 6
int val;                                    // variable for reading the pin status
int val2;                                   // variable for reading the delayed/debounced status
int buttonState;                            // variable to hold the button state
const short numrotationspercalc = 1;
short currentrot = 0;

unsigned long utime; 
unsigned long mtime;
unsigned long laststatechangeus;            //time of last switch.
unsigned long timetakenus;                //time taken in milliseconds for a rotation of the flywheel
unsigned long instantaneousrpm;           //rpm from the rotatiohn
float nextinstantaneousrpm;       // next rpm reading to compare with previous
unsigned long lastrotationus;              // milliseconds taken for the last rotation of the flywheel
unsigned long startTimems = 0;              // milliseconds from startup to first sample
unsigned long rotations = 0;              // number of rotations since start
unsigned long laststrokerotations = 0;    // number of rotations since last drive
unsigned long laststroketimems = 0;         // milliseconds from startup to last stroke drive
unsigned long spm = 0;                    // current strokes per minute.  
unsigned long difftms;                      // milliseconds from last stroke to this one
unsigned long driveTimems;                  //time of the end of the last drive

float rpmhistory[100];
unsigned long microshistory[100];

short nextrpm;

float newk;
float driveAngularVelocity;                // fastest angular velocity at end of drive
bool afterfirstdecrotation = false;       // after the first deceleration rotation (to give good figures for drag factor);
long diffrotations;                       // rotations from last stroke to this one
float split;                              // split time for last stroke in seconds
int screenstep=0;                         // int - which part of the display to draw next.

//unsigned long angularmomentum_gm2 = 100;//0.1001 kgm2 == 100.1g/m2 moment of intertia
float I = 0.1001/*moment of  interia*/;
float k = 170;/*drag factor 10^6 nm/s/s*/
float c = 2.8; //The figure used for c is somewhat arbitrary - selected to indicate a 'realistic' boat speed for a given output power.
         //Concept used to quote a figure c=2.8, which, for a 2:00 per 500m split (equivalent to u=500/120=4.17m/s) gives 203 Watts. 
bool Accelerating;


void setup() 
{
 pinMode(switchPin, INPUT_PULLUP);                // Set the switch pin as input
 Serial.begin(115200);                      // Set up serial communication at 115200bps
 buttonState = digitalRead(switchPin);  // read the initial state
   // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  //lcd.print("hello, Lisa!");
}

void loop()
{
  val = digitalRead(switchPin);            // read input value and store it in val                       
       if (val != buttonState && val == LOW)            // the button state has changed!
          {   
            currentrot ++;
            if(currentrot>= numrotationspercalc )
            {
              currentrot = 0;
              utime = micros();    
              mtime = millis();      
              //initialise the start time
              if(startTimems == 0) startTimems = mtime;
              timetakenus = utime - laststatechangeus;
              rotations++;
              //Serial.print("TimeTaken(ms):");
              //Serial.println(timetakenms);
              nextinstantaneousrpm = (float)60000000.0*numrotationspercalc/timetakenus;
              float radSec = (6.283185307*numrotationspercalc)/((float)timetakenus/1000000.0);
              float prevradSec = (6.283185307*numrotationspercalc)/((float)lastrotationus/1000000.0);
              float angulardeceleration = (prevradSec-radSec)/((float)timetakenus/1000000.0);
              nextrpm ++;
              rpmhistory[nextrpm] = nextinstantaneousrpm;
              dumprpms();
              if(nextrpm >=99) nextrpm = 0;
              if(nextinstantaneousrpm >= instantaneousrpm)
                {
                    //lcd.print("Acc");        
                    if(!Accelerating)
                    {
                      //beginning drive.
                      Serial.println("lastk");
                      Serial.println(newk);
                    }
                    driveAngularVelocity = radSec;
                    driveTimems = mtime;
                    afterfirstdecrotation = false;
                    Accelerating = true;    
                }
                else if(nextinstantaneousrpm <= (instantaneousrpm*0.99))
                {
                    
                    //lcd.print("Dec");
                    if(Accelerating)//previously accelerating
                    { //finished drive
                      diffrotations = rotations - laststrokerotations;
                      difftms = mtime - laststroketimems;
                      spm = 60000 /difftms;
                      laststrokerotations = rotations;
                      laststroketimems = mtime;
                      split =  ((float)difftms)/((float)diffrotations*mPerRot*2) ;//time for stroke /1000 for ms *500 for 500m = *2
                      //   /1000*500 = /2
                    }
                    else
                    {
                      float secondsdecel = ((float)mtime-(float)driveTimems)/1000;
                      float angularDecel = (driveAngularVelocity-radSec)/(secondsdecel);
                      newk = calculateDragFactor(angularDecel, radSec);
                    }
                    Accelerating = false;
                                              
                }                          
                lastrotationus = timetakenus;
                instantaneousrpm = nextinstantaneousrpm;
                //watch out for integer math problems here
                //Serial.println((nextinstantaneousrpm - instantaneousrpm)/timetakenms); 
            } 
            laststatechangeus=utime;
            writeNextScreen();
            microshistory[nextrpm] = micros()-utime;
          }
  buttonState = val;                       // save the new state in our variable
}

void writeNextScreen()
{
  int timemins, timeseconds, splitmin, splits, dm;
  screenstep++;
  switch(screenstep)
  {//only write a little bit to the screen to save time.
    case 0:
    //lcd.clear();
    lcd.setCursor(0,0);
    break;
    case 1:
      splitmin = (int)(split/60);
      if(splitmin < 10)
      {
      lcd.print(splitmin);//minutes in split.
      lcd.print(":");
      splits = (int)(((split-splitmin*60)));
      if(splits <10) lcd.print("0");
      lcd.print(splits);//seconds
      lcd.print("/500");
      }
    break;
    case 2:
    lcd.setCursor(10,0);
      lcd.print("SPM:");
      lcd.print(spm);
      lcd.print("  ");
    break;
    case 3:
     lcd.setCursor(0,1);
     //Distance in meters:
     
     dm = (int)(rotations*mPerRot);
      lcd.print("D:");
      if(dm <1000) lcd.print("0");
      if(dm <100) lcd.print("0");
      if(dm <10) lcd.print("0");
      lcd.print(dm);
      lcd.print("m ");
      

      //Drag factor
      //lcd.print("k:");
      //lcd.print(k);
    break;
    case 4:
      timemins = (mtime-startTimems)/60000;
        lcd.setCursor(11,1);
        if(timemins <10) lcd.print("0");
        lcd.print(timemins);//total mins
        lcd.print(":");
        timeseconds = (long)((mtime)-startTimems)/1000 - timemins*60;
        if(timeseconds < 10) lcd.print("0");
        lcd.print(timeseconds);//total seconds.
//      lcd.print("s");
//      lcd.print(diffrotations);
      break;
    case 5://next lime
    screenstep = -1;
      //lcd.setCursor(10,1);
      //lcd.print(" AvS:");
      //lcd.print(rotations/(millis()-startTime));     
      break;
/*    case 6:
          //lcd.print(" t:");

//      lcd.print("d");
//      lcd.print(rotations);
break;
    case 7:
//      lcd.print("r");
//      lcd.print(instantaneousrpm);
break;
    case 8:
//      lcd.print("n");
//      lcd.print(nextinstantaneousrpm);

      break;
    case 9:

      break;*/
    default:
      screenstep = -1;//will clear next time, as 1 is added and if 0 will be cleared.
  }


  //                                lcd.print("RPM:");
  //                                lcd.print(instantaneousrpm);
  //                                lcd.print("distance:");
  //                                lcd.print(rotations);
  //Serial.print("RPM:");
  //Serial.println(instantaneousrpm);
    
    
    
    
}

//void resetT()
//{
//  t1 = t2;
//  t2 = millis();
//}

////return the power supplied in Watts - given angular velocity change, and time change.
//float PowerSupplied()
//{
//  float result = I * dw()/(dt()/(float)1000) * dtheta() + k *  dw()*dw() * dtheta();
//  //reset previous angular velocity;
//  w1 = dtheta()/dt();
//  return result;
//}
//
////change in angular velocity
//float dw()
//{
//  return dtheta()/dt() - w1;
//}
//
////change in angular velocity rad/s
//float dtheta()
//{
//  return 2 * 3.1415926535;
//}
//
////time difference in milliseconds
//float dt()
//{
//  return t2-t1;
//}

//k can be displayed on the Concept monitor (units 10-6 N m s2)
//k = - I ( dω / dt ) (1 / ω2 ) = I d(1/ω) / dt
float calculateDragFactor(float angulardeceleration, float angularvelocity)
{
  /*
  Serial.println("D:");
  Serial.println(angulardeceleration);
  Serial.println(angularvelocity);
  Serial.println(I);
  Serial.println();
  */
  return I*angulardeceleration*(1.0/(angularvelocity*angularvelocity));
}

void dumprpms()
{
  if(nextrpm > 97)
  {
    /*
    Serial.println("Rpm dump");
    for(int i = 0; i < 90; i++)
    {
      Serial.println(rpmhistory[i]);
    }
  }
    Serial.println("micros dump");
    for(int i = 0; i <90; i++)
    {
      Serial.println(microshistory[i]);
    }
    */
    nextrpm = 0;
  }
}

//(9.2)	u = ( k / c )1/3 ω
float calculateSplit(float angularVelocity)
{
  return pow((k/c),(1/3)) * angularVelocity;
}


