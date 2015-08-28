#include <LiquidCrystal.h>
// http://www.atm.ox.ac.uk/rowing/physics/ergometer.html#section7
// read reed switch from rowing machine.
// based on Lady Ada's "debounced blinking bicycle light" code

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

int switchPin = 6;                          // switch is connected to pin 2
int val;                                    // variable for reading the pin status
int val2;                                   // variable for reading the delayed/debounced status
int buttonState;                            // variable to hold the button state
int count=0;
unsigned long laststatechange;            //time of last switch.
unsigned long timetakenms;
unsigned long instantaneousrpm;
unsigned long nextinstantaneousrpm;
unsigned long startTime = 0;
//unsigned long angularmomentum_gm2 = 100;//0.1001 kgm2 == 100.1g/m2 moment of intertia
float I/*moment of  interia*/;
float k;/*drag factor*/
float c; //The figure used for c is somewhat arbitrary - selected to indicate a 'realistic' boat speed for a given output power. Concept used to quote a figure c=2.8, which, for a 2:00 per 500m split (equivalent to u=500/120=4.17m/s) gives 203 Watts. 

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
       if (val != buttonState)            // the button state has changed!
          {                
             if (val == LOW)                    // check if the button is pressed
                {  
                  //initialise the start time
                  if(startTime == 0) startTime = millis();
                  count++;
//                  if (count==2)
//                      {
                          timetakenms = millis() - laststatechange;
                          //Serial.print("TimeTaken(ms):");
                          //Serial.println(timetakenms);
                          nextinstantaneousrpm = 60000/timetakenms;
                          lcd.setCursor(0, 1);
                          lcd.setCursor(0,1);
                          lcd.print("                ");
                          lcd.setCursor(0,1);
                          lcd.print("RPM:");
                          lcd.print(instantaneousrpm);
                          //Serial.print("RPM:");
                          //Serial.println(instantaneousrpm);
                          lcd.setCursor(0,0);
                         lcd.print("LOW ");
                          if(nextinstantaneousrpm > instantaneousrpm)
                          {
                                        lcd.print("Acc");            
                          }
                          else
                          {
                            lcd.print("Dec");
                           //Serial.println("Decelerating"); 
                          }
                          //watch out for integer math problems here
                          //Serial.println((nextinstantaneousrpm - instantaneousrpm)/timetakenms);
                          count=0;
                          instantaneousrpm = nextinstantaneousrpm;
                          laststatechange = millis();
                          delay(5);//wait 5ms
//                      }                     
                } 
                else
                {
                  lcd.setCursor(0,0);
                  lcd.print("HIGH"); 
                }
          }
          buttonState = val;                       // save the new state in our variable
}

//k can be displayed on the Concept monitor (units 10-6 N m s2)
//k = - I ( dω / dt ) (1 / ω2 ) = I d(1/ω) / dt
float calculateDragFactor(float deceleration, float angularvelocity)
{
  return -I*deceleration*(1/pow(angularvelocity,2));
}

//(9.2)	u = ( k / c )1/3 ω
float calculateSplit(float angularVelocity)
{
  return pow((k/c),(1/3)) * angularVelocity;
}


