#ifndef LCDinclude
#define LCDinclude

#include "Arduino.h"

#define UseLCD 
//-------------------------------------------------------------------
//               KEY index definitions
#define NO_KEY 0
#define UP_KEY 3
#define DOWN_KEY 4
#define LEFT_KEY 2
#define RIGHT_KEY 5
#define SELECT_KEY 1
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//-------------------------------------------------------------------
//               reference values for each key on the  keypad:
static int DEFAULT_KEY_PIN = 0; 
static int UPKEY_ARV = 144; 
static int DOWNKEY_ARV = 329;
static int LEFTKEY_ARV = 505;
static int RIGHTKEY_ARV = 0;
static int SELKEY_ARV = 721;
static int NOKEY_ARV = 1023;
static int _threshold = 50;

//-------------------------------------------------------------------
//               custom Character definitions
#define LCDSlowDown 0
#define LCDSpeedUp 1
#define LCDJustFine 3

#endif
