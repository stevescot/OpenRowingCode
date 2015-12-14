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
bool AnalogSwitch = false;                  // if we are connected to a concept2
byte clicksPerRotation = 1;                  // number of magnets , or clicks detected per rotation.
short numclickspercalc = 3;                 // number of clicks to wait for before doing anything, if we need more time this will have to be reduced.
float I = 0.04;                             // moment of  interia of the wheel - 0.1001 for Concept2, ~0.05 for V-Fit air rower.*/;
float mStrokePerRotation = 0;               // relation from rotation to meters of pull on the handle
//-------------------------------------------------------------------
//               timing
unsigned long utime;                        // time of tick in microseconds
unsigned long mtime;                        // time of tick in milliseconds
unsigned long laststatechangeus;            // time of last click
unsigned long timetakenus;                  // time taken in milliseconds for a click from the flywheel
unsigned long lastrotationus;               // milliseconds taken for the last click of the flywheel
unsigned long startTimems = 0;              // milliseconds from startup to first sample
unsigned long laststrokeclicks = 0;         // number of clicks since last drive
unsigned long laststroketimems = 0;         // milliseconds from startup to last stroke drive
unsigned long strokems;                     // milliseconds from last stroke to this one
unsigned long driveEndms = 0;               // time of the end of the last drive (beginning of recovery.
unsigned long recoveryEndms = 0;            // time of the end of the recovery
unsigned int lastDriveTimems = 0;           // time that the last drive took in milliseconds
float secondsdecel =  0;                    // number of seconds spent decelerating.
//-------------------------------------------------------------------
//               strokes
unsigned int totalStroke = 0;
float driveLengthm = 0;                     // last stroke length in meters
//-------------------------------------------------------------------
//               rpm/angular Velocity
static const short numRpms = 100;           // size of the rpm array
int rpmhistory[numRpms];                    // array of rpm per rotation for debugging
//unsigned long microshistory[numRpms];       // array of the amount of time taken in calc/display for debugging.
short nextrpm = 0;                          // currently measured rpm, to compare to last -index in above array.
int peakrpm = 0;                            // highest measured rpm
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
float RecoveryToDriveRatio = 0;             // the ratio of time taken for the whole stroke to the drive , should be roughly 3:1

//-------------------------------------------------------------------
//               Power Graph
static const int PowerSamples = 40;
int PowerArray[PowerSamples];

//
unsigned long lastcalcchangeus = 0;       // the time of the last click that caused calculations.
