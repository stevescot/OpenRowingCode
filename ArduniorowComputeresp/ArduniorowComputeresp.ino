/* Arduino row Computer
 * Uses a 16x2 lcd to display rowing statistics on an ergometer
 * principles are here : http://www.atm.ox.ac.uk/rowing/physics/ergometer.html#section7
 * 
 */
#include "ESP8266WiFi.h"
#include "rowWifi.h"
#include <ESP8266mDNS.h>
#include "gpio.h"
#include <WiFiClient.h>
#include <EEPROM.h>
WiFiClient client;
WiFiServer server(80);

MDNSResponder mdns;
String st, host, site;
//SSID when in unassociated mode.
const char * ssid = "IntelligentPlant";
//name of this node.
String myName = "";
String MAC;                  // the MAC address of your Wifi shield
rowWiFi RowServer("demo1.intelligentplant.com","IProw", client);



bool wifiactive = false;                    //
const int switchPin = 2;                          // switch is connected to pin 6
int val;                                    // variable for reading the pin status
int buttonState;                            // variable to hold the button state
const short numrotationspercalc = 1;        // number of rotations to wait for before doing anything, if we need more time this will have to be reduced.
short currentrot = 0;

unsigned long utime;                        //time of tick in microseconds
unsigned long mtime;                        //time of tick in milliseconds

unsigned long lastRecoveryms;               //last time in recovery
unsigned long laststatechangeus;            //time of last switch.
unsigned long timetakenus;                  //time taken in milliseconds for a rotation of the flywheel
unsigned long instantaneousrpm;             //rpm from the rotatiohn
float nextinstantaneousrpm;                 // next rpm reading to compare with previous
unsigned long lastrotationus;               // milliseconds taken for the last rotation of the flywheel
unsigned long startTimems = 0;              // milliseconds from startup to first sample
unsigned long rotations = 0;                // number of rotations since start
unsigned long rotationsInDistance = 0;     // number of rotations already accounted for in the distance.
unsigned long laststrokerotations = 0;      // number of rotations since last drive
unsigned long laststroketimems = 0;         // milliseconds from startup to last stroke drive
unsigned long strokems;                      // milliseconds from last stroke to this one

float rpmhistory[100];                      //array of rpm per rotation for debugging
unsigned long microshistory[100];           //array of the amount of time taken in calc/display for debugging.

short nextrpm;                              // currently measured rpm, to compare to last.

float driveAngularVelocity;                // fastest angular velocity at end of drive
int diffrotations;                         // rotations from last stroke to this one

int screenstep=0;                           // int - which part of the display to draw next.
float k = 0.000185;                         //  drag factor nm/s/s (displayed *10^6 on a Concept 2
float c = 2.8;                              //The figure used for c is somewhat arbitrary - selected to indicate a 'realistic' boat speed for a given output power.
                                            //Concept used to quote a figure c=2.8, which, for a 2:00 per 500m split (equivalent to u=500/120=4.17m/s) gives 203 Watts. 
                                            
float mPerRot = 0;                          // meters per rotation of the flywheel

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
float I = 0.05;                             // moment of  interia of the wheel - 0.1001 for Concept2, ~0.05 for V-Fit air rower.*/;

void setup() 
{
  setupWiFi();
  st = "";//empty the option list string to save some memory.
  st.reserve(0);
 pinMode(switchPin, INPUT_PULLUP);                // Set the switch pin as input
 Serial.begin(115200);                      // Set up serial communication at 115200bps
 buttonState = digitalRead(switchPin);  // read the initial state
}

void setupWiFi() {
  EEPROM.begin(512);
  Serial.begin(115200);
  delay(10);
  byte mac[6];   
  WiFi.macAddress(mac);
  MAC = "";
  for(int i=0; i<6;i++)
  {
    MAC += String(mac[i],16);
    //if we want http colons:
    //if(i < 5) MAC +="%3A";
  }
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  esid.reserve(32);
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  epass.reserve(64);
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  Serial.println(epass);  
  host.reserve(64);
  for (int i = 96; i < 160; i++)
  {
    host += char(EEPROM.read(i));
  }
  Serial.print("Gestalt: " );
  Serial.println(host);
  myName.reserve(40);
  for (int i = 160; i < 200; i++)
  {
    myName += char(EEPROM.read(i));
  }
  Serial.print("MyName: " );
  Serial.println(myName);
  site.reserve(20);
  for(int i = 200; i < 220; i++)
  {
    site += char(EEPROM.read(i));
  }
  Serial.print("Site: ");
  Serial.println(site);
  if ( esid.length() > 1 ) {
      // test esid 
      WiFi.mode(WIFI_STA);
      WiFi.begin(esid.c_str(), epass.c_str());
      if ( testWifi() == 20 ) { 
          wifiactive = true;
          return;
      }
  }
  /*not returned above, so go to setup*/
  setupAP(); 
}

int testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { return(20); } 
    delay(500);
    Serial.print(WiFi.status());    
    c++;
  }
  Serial.println("Connect timed out, opening AP");
  return(10);
} 

void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());
  mdns.begin("row", WiFi.softAPIP());
  // Start the server
  server.begin();
  Serial.println("Server started");   
  int b = 20;
  int c = 0;
  while(b == 20) 
  { 
     b = mdns1(webtype);
  }
}

void setupAP(void) {
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st.reserve(60 + n * 20);
  st = "<select name='ssid'>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<option value = '";
      st += WiFi.SSID(i);
      st += "'/>";
      st += WiFi.SSID(i);
      st += "</option>";
    }
  st += "</select>";
  delay(100);

  WiFi.softAP(ssid);
  Serial.println("softap");
  Serial.println("");
  launchWeb(1);
  Serial.println("over");
}

int mdns1(int webtype)
{
  // Check for any mDNS queries and send responses  mdns.update();
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return(20);
  }
  Serial.println("");
  Serial.println("New client");

  // Wait for data from client to become available
  while(client.connected() && !client.available()){
    delay(1);
   }
  
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
  client.flush(); 
  String s;
  if ( webtype == 1 ) {
      if (req == "/")
      {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
        s += ipStr;
        s += "<p>";
        s += "<form method='get' action='a'><label>SSID: </label>";
        s += st;
        s += "<input name='pass' length=64/><label>GSrv:</label><input name='g'/><label>MyName</label><input name='n' length=40/><label>Site</label><input name='s' length=20/><input type='submit'></form>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/a?ssid=") ) {
        // /a?ssid=blahhhh&pass=poooo&g=demo1&s=pnid
        Serial.println("clearing eeprom");
        for (int i = 0; i < 220; ++i) { EEPROM.write(i, 0); }
        String qsid; 
        int currentstart;
        int currentend;
        currentstart = 8;
        currentend = req.indexOf('&');
        qsid = req.substring(currentstart,currentend);
        Serial.println(qsid);
        Serial.println("");
        String qpass;
        currentstart = currentend + 6;
        currentend = req.indexOf('&',currentstart);
        qpass = req.substring(currentstart,currentend);
        Serial.println(qpass);
        Serial.println("");
        currentstart = currentend + 3;
        currentend = req.indexOf('&',currentstart);
        String qg;
        qg = req.substring(currentstart,currentend);
        Serial.println(qg);
        Serial.println("");
        String qn;//name
        currentstart = currentend + 3;
        currentend = req.indexOf('&',currentstart+1);
        qn = req.substring(currentstart, currentend);
        Serial.println(qn);
        Serial.println("");
        String qs;//site
        currentstart = currentend +3;
        currentend = req.indexOf('&',currentstart+1);
        qs = req.substring(currentstart, currentend);
        Serial.println(qs);
        Serial.println("");
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
        Serial.println("writing eeprom pass:"); 
        for (int i = 0; i < qpass.length(); ++i)
          {
            EEPROM.write(32+i, qpass[i]);
            Serial.print("Wrote: ");
            Serial.println(qpass[i]); 
          }    
        Serial.println("writing eeprom g:");
        for (int i = 0; i < qg.length(); ++i)
        {
          EEPROM.write(96+i, qg[i]);
          Serial.print("Wrote: ");
          Serial.println(qg[i]);
        }
        Serial.println("writing eeprom n:");
        for(int i = 0; i < qn.length(); ++i)
        {
          EEPROM.write(160+i, qn[i]);
          Serial.print("Wrote: ");
          Serial.println(qn[i]);
        }
        Serial.println("writing eeprom s:");
        for(int i = 0; i < qs.length(); ++i)
        {
          EEPROM.write(200+i, qs[i]);
          Serial.print("Wrote: ");
          Serial.println(qs[i]);
        }
        EEPROM.commit();
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 ";
        s += "Found ";
        s += req;
        s += "<p> saved to eeprom... reset to boot into new wifi</html>\r\n\r\n";
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
      }
  } 
  else
  {
      if (req == "/")
      {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
        s += "<p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/cleareeprom") ) {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
        s += "<p>Clearing the EEPROM<p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");  
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        EEPROM.commit();
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
      }       
  }
  client.print(s);
  Serial.println("Done with client");
  return(20);
}


void loop()
{
  val = digitalRead(switchPin);            // read input value and store it in val                       
       if ((val != buttonState) && (val == LOW) && (utime-laststatechangeus > 10000) )  
          {   
            if(!wifiactive) setupWiFi();//reconnect to WiFi if we are currently disconnected.
            currentrot ++;
            if(currentrot >= numrotationspercalc)
            {
              utime = micros();    
              mtime = millis();      
              //initialise the start time
              if(startTimems == 0) 
              {
                startTimems = mtime;
                //lcd.clear();
              }
              timetakenus = utime - laststatechangeus;
              rotations++;
              nextinstantaneousrpm = (float)60000000.0*numrotationspercalc/timetakenus;
              float radSec = (6.283185307*numrotationspercalc)/((float)timetakenus/1000000.0);
              float prevradSec = (6.283185307*numrotationspercalc)/((float)lastrotationus/1000000.0);
              float angulardeceleration = (prevradSec-radSec)/((float)timetakenus/1000000.0);
              nextrpm ++;
              rpmhistory[nextrpm] = nextinstantaneousrpm;
              dumprpms();
              if(nextrpm >=99) nextrpm = 0;
              if(mPerRot <= 20)
              {
                distancem += (rotationsInDistance - rotations)*mPerRot;
                rotationsInDistance = rotations;
              }
              //Serial.println(nextinstantaneousrpm);
              if(nextinstantaneousrpm >= instantaneousrpm)
                {
                    //lcd.print("Acc");        
                    if(!Accelerating)
                    {//beginning of drive /end recovery
                      driveBeginms = mtime;
                      float secondsdecel = ((float)lastRecoveryms-(float)driveEndms)/1000;
                      
                      //calculate the drag factor - and tweak previous estimate by 1/3 the difference 
                      k = k + ((I * ((1.0/radSec)-(1.0/driveAngularVelocity))/(secondsdecel))-k)/3.0;
                      mPerRot = pow((k/c),(1.0/3.0))*2*3.1415926535;
                      //calculate rotations on this stroke
                      diffrotations = rotations - laststrokerotations;
                      laststrokerotations = rotations;
                      //calculate time on this stroke
                      strokems = mtime - laststroketimems;
                      //use time and rotations to calculate split on this stroke.
                      split =  ((float)strokems)/((float)diffrotations*mPerRot*2) ;//time for stroke /1000 for ms *500 for 500m = *2
                      spm = 60000 /strokems;
                      laststroketimems = mtime;
                      //now is a good time to send the split - we are at the slowest rotation possible...
                      SendSplit(mtime, diffrotations*mPerRot, distancem, lastDriveTimems, strokems - lastDriveTimems);
                      StrokeToDriveRatio = (strokems / lastDriveTimems);
                    }
                    driveAngularVelocity = radSec;
                    Accelerating = true;    
                }
                else if(nextinstantaneousrpm <= (instantaneousrpm*0.99))
                {
                    if(Accelerating)//previously accelerating
                    { //finished drive
                      driveEndms = mtime - (timetakenus/1000);
                      lastDriveTimems = driveEndms - driveBeginms;
                      //Serial.println("ACC");//this is when we are spinning fastest.
                    }
                    lastRecoveryms = mtime;
                    Accelerating = false;
                }        
                //store this time for a rotation, and rpm to allow it to be used as the previous next time.                  
                lastrotationus = timetakenus;
                instantaneousrpm = nextinstantaneousrpm;
            } 
            laststatechangeus=utime;
          }
          microshistory[nextrpm] = micros()-utime;
  buttonState = val;                       // save the new state in our variable
}

void sleep()
{
    RowServer.finishSend();
    delay(2000);
    WiFi.mode(WIFI_OFF);
    wifiactive = false;
  //gpio_pin_wakeup_enable(GPIO_ID_PIN(4), GPIO_PIN_INTR_HILEVEL);
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

void SendSplit(unsigned long msfromStart, float strokeDistance,  float totalDistancem, unsigned long msDrive, unsigned long msRecovery)
{
  RowServer.sendSplit(MAC, msfromStart, strokeDistance, totalDistancem, msDrive, msRecovery);
}


