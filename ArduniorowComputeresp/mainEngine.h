//-------------------------------------------------------------------
// Steve Aitken - 2015
// Main engine header
// this contains all the varialbes within the engine that we need to be able to access externally.
//
//
#include "Arduino.h"
//-------------------------------------------------------------------
//               session(menu) definitions
#define JUST_ROW 0
#define DISTANCE 1
#define TIME 2
#define INTERVAL 3
#define DRAGFACTOR 4
#define RPM 5
#define WATTS 6
#define SETTINGS 7
#define BACKLIGHT 8
#define ERGTYPE 9
#define BOATTYPE 10
#define WEIGHT 11
#define POWEROFF 12
#define BACK 13

//-------------------------------------------------------------------
//               erg type definitions
#define ERGTYPEVFIT 0 // V-Fit air rower.
#define ERGTYPEC2 1   // Concept 2
//-------------------------------------------------------------------
//               boat type defintions.
#define BOAT4 0
#define BOAT8 1
#define BOAT1 2
//-------------------------------------------------------------------
//               Erg types
short ergType = ERGTYPEVFIT;                // erg type currently selected.
short boatType = BOAT4;                     // boat type to simulate
//-------------------------------------------------------------------
//               Targets
long targetDistance = 2000;                 // Target distance in meters (2000 = 2k erg)
long targetSeconds = 20*60;                 // target time in seconds also interval 'on' time
long intervalSeconds = 60;                  // time for rest in interval mode
byte numIntervals = 5;                       // number of intervals in interval mode.
//-------------------------------------------------------------------
byte intervals = 0;//number of intervals we have done.
byte sessionType = JUST_ROW;
//-------------------------------------------------------------------
//               Erg Specific Settings
bool analogSwitch = false;                  // if we are connected to a concept2
byte clicksPerRotation = 1;                 // number of magnets , or clicks detected per rotation.
short clicksPerCalc = 3;                    // number of clicks to wait for before doing anything, if we need more time this will have to be reduced.
float I = 0.04;                             // moment of  interia of the wheel - 0.1001 for Concept2, ~0.05 for V-Fit air rower.*/;
float mStrokePerRotation = 0;               // relation from rotation to meters of pull on the handle
//-------------------------------------------------------------------
//               timing
unsigned long uTime;                        // time of tick in microseconds
unsigned long mTime;                        // time of tick in milliseconds
unsigned long lastStateChangeus;            // time of last click
unsigned long timeTakenus;                  // time taken in milliseconds for a click from the flywheel
unsigned long lastRotationus;               // milliseconds taken for the last click of the flywheel
unsigned long startTimems = 0;              // milliseconds from startup to first sample
unsigned long lastStrokeClicks = 0;         // number of clicks since last drive
unsigned long lastStrokeTimems = 0;         // milliseconds from startup to last stroke drive
unsigned long strokems;                     // milliseconds from last stroke to this one
unsigned long driveEndms = 0;               // time of the end of the last drive (beginning of recovery.
unsigned long recoveryEndms = 0;            // time of the end of the recovery
unsigned int lastDriveTimems = 0;           // time that the last drive took in milliseconds
float secondsDecel =  0;                    // number of seconds spent decelerating - this changes as the deceleration happens
float previousSecondsDecel = 0;             // seconds decelerating from the previous recovery (doesn't change during the recovery)
unsigned long lastCalcChangeus = 0;         // the time of the last click that caused calculations.
//-------------------------------------------------------------------
//               strokes
unsigned int totalStroke = 0;
float driveLengthm = 0;                     // last stroke length in meters
//-------------------------------------------------------------------
//               rpm/angular Velocity
static const short numRpms = 100;           // size of the rpm array
int rpmHistory[numRpms];                    // array of rpm per rotation for debugging
//unsigned long microshistory[numRpms];     // array of the amount of time taken in calc/display for debugging.
short nextRPM = 0;                          // currently measured rpm, to compare to last -index in above array.
int peakRPM = 0;                            // highest measured rpm
//-------------------------------------------------------------------
//               Drag factor
float k = 0.000185;                         // drag factor nm/s/s (displayed *10^6 on a Concept 2) nm/s/s == W/s/s
                                            // The displayed drag factor equals the power dissipation (in Watts), at an angular velocity of the flywheel of 100 rad/sec.    
//-------------------------------------------------------------------
//               Stats for display
float split = 0;                            // split time for last stroke in seconds
float power = 0;                            // last stroke power in watts
byte spm = 0;                               // current strokes per minute.  
float distancem = 0;                        // distance rowed in meters.
float recoveryToDriveRatio = 0;             // the ratio of time taken for the whole stroke to the drive , should be roughly 3:1

//-------------------------------------------------------------------
//               Power Graph
static const int powerSamples = 40;         // number of samples in the power graph
int powerArray[powerSamples];               // array of powers (per rotation/click from the flywheel)
int nextPower = 0;                          // current index in the powerArray.

//Calculate the median of an array of numbers with num being how many numbers to consider.
float median(float array[], int num){
    float new_array[num];
    for(int x=0; x< num; x++)
    {
      new_array[x] = array[x];
    }
     //ARRANGE VALUES
    for(int x=0; x<num; x++){
         for(int y=0; y<num-1; y++){
             if(new_array[y]>new_array[y+1]){
                 float temp = new_array[y+1];
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


//Calculate the median of an array of numbers with num being how many numbers to consider.
int median(int array[], int num){
    int new_array[num];
    for(int x=0; x< num; x++)
    {
      new_array[x] = array[x];
    }
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
