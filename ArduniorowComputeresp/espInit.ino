#include "ESP8266WiFi.h"
#include "rowWifi.h"
#include <EEPROM.h>
WiFiClient client;
WiFiServer server(80);
#include "./DNSServer.h"                  // Patched lib
DNSServer         dnsServer;              // Create the DNS object
const byte        DNS_PORT = 53;          // Capture DNS requests on port 53
String st, host, site;
//SSID when in unassociated mode.
const char * ssid = "IProw";
//name of this node.
String myName = "";
String MAC;                  // the MAC address of your Wifi shield
rowWiFi RowServer("monitoring.intelligentplant.local","row", client);

void SendSplit(unsigned long msfromStart, float strokeDistance,  float totalDistancem, unsigned long msDrive, unsigned long msRecovery, int PowerArray[])
{
  RowServer.sendSplit(MAC, msfromStart, strokeDistance, totalDistancem, msDrive, msRecovery, PowerArray, powerSamples);
}

void setupWiFi() {
  st = "";//empty the option list string to save some memory.
  st.reserve(0);
  EEPROM.begin(512);
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
  Serial.println(F("Startup"));
  // read eeprom for ssid and pass
  Serial.println(F("Reading EEPROM ssid"));
  char esid[32];
  for (int i = 0; i < 32; ++i)
    {
      esid[i] = EEPROM.read(i);
    }
  Serial.print(F("SSID: "));
  Serial.println(esid);
  Serial.println(F("Reading EEPROM pass"));
  char epass[64] ;
  for (int i = 32; i < 96; i++)
    {
      epass[i-32] = char(EEPROM.read(i));
    }
  Serial.print(F("PASS: "));
  Serial.println(epass);  
  host.reserve(64);
  for (int i = 96; i < 160; i++)
  {
    if(EEPROM.read(i) != 0)
      {
        host += char(EEPROM.read(i));
      }
  }
  Serial.print(F("Gestalt: " ));
  Serial.println(host);
  myName.reserve(40);
  for (int i = 160; i < 200; i++)
  {
    if(EEPROM.read(i) != 0)
      {
        myName += char(EEPROM.read(i));
      }
  }
  Serial.print(F("MyName: " ));
  Serial.println(myName);
  site.reserve(20);
  for(int i = 200; i < 220; i++)
  {
    if(EEPROM.read(i) != 0)
      {
        site += char(EEPROM.read(i));
      }
  }
  Serial.print(F("Site: "));
  Serial.println(site);
  if ( esid != "" ) {
      // test esid 
      Serial.print(F("connecting to : "));
      Serial.print(esid);
      Serial.print(F("with password: "));
      Serial.println(epass);
      if ( testWifi(esid, epass) == 20 ) { 
          Serial.println(F("Connected, returning"));
      }
      else
      {
        //timed out - setup access point again
        setupAP();
      }
  }
  else
  {
    //no ssid - setup access point.
    setupAP(); 
  }
  delay(100);
  Serial.println("exiting");
  delay(200);
}



String getMac()
{
  return MAC;
}

int testWifi(char esid[], char epass[]) {
  int c = 0;
  int x = 0;
    if(epass != "")
    {
    x = WiFi.begin(esid,epass);
    }
    else
    {
      x = WiFi.begin(esid);
    }
  Serial.println(F("Waiting for Wifi to connect"));  
  while ( c < 200 ) {
      x = WiFi.status();
    // wait 10 seconds for connection:
    //delay(10000);
      if(x == WL_CONNECTED) {
      Serial.println(WiFi.localIP());
      //delay(1000);
      return(20); 
      } 
    Serial.println(F("rechecking Connection.."));
    delay(100);
    c++;
  }
  Serial.println(F("Connect timed out, opening AP"));
  return(10);
} 

void launchWeb(int webtype) {
          Serial.println();
          Serial.println(F("WiFi connected"));
          Serial.println(WiFi.localIP());
          Serial.println(WiFi.softAPIP());
          dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
          // Start the server
          server.begin();
          
          Serial.println(F("Server started"));   
          int b = 20;
          int c = 0;
          while(b == 20) 
          { 
             b = mdns1(webtype);
             dnsServer.processNextRequest();
          }
}

void setupAP(void) {
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println(F("scan done"));
  if (n == 0)
    Serial.println(F("no networks found"));
  else
  {
    Serial.print(n);
    Serial.println(F(" networks found"));
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(F(": "));
      Serial.print(WiFi.SSID(i));
      Serial.print(F(" ("));
      Serial.print(WiFi.RSSI(i));
      Serial.print(F(")"));
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(); 
  st.reserve(60 + n * 20);
  st = F("<select name='ssid'>");
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += F("<option value = '");
      st += WiFi.SSID(i);
      st += F("'/>");
      st += WiFi.SSID(i);
      st += F("</option>");
    }
  st += F("</select>");
  delay(100);

  WiFi.softAP(ssid);
  Serial.println(F("softap"));
  Serial.println();
  launchWeb(1);
  Serial.println(F("over"));
}

int mdns1(int webtype)
{  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return(20);
  }
  Serial.println();
  Serial.println(F("New client"));

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
    Serial.print(F("Invalid request: "));
    Serial.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print(F("Request: "));
  Serial.println(req);
  client.flush(); 
  String s;
  if ( webtype == 1 ) {
    if(req.equals("/"))
    {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        s =  F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>");
        s += F("<head><style>table#settings {");
        s += F("font-size:16px;");
        s += F("font-family: 'Trebuchet MS', Arial, Helvetica, sans-serif;");
        s += F("border-collapse: collapse;");
        s += F("border-spacing: 0;");
        s += F("width: 100%;");
        s += F("}");
        s += F("#settings td, #settings th {");
        s += F("border: 1px solid #ddd;");
        s += F("text-align: left;");
        s += F("padding: 8px;");
        s += F("}");
        s += F("#settings tr:nth-child(even){background-color: #f2f2f2}");
        s += F("#settings th {");
        s += F("padding-top: 11px;");
        s += F("padding-bottom: 11px;");
        s += F("background-color: #4CAF50;");
        s += F("color: white;");
        s += F("}");
        s += F("</style>");
        s += F("</head>");
        s += F("<body>");
        client.print(s);
        s = F("<img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAIAAAACACAYAAADDPmHLAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAABMdJREFUeNrsnYtx2kAQho9UoA6MO6CDqAPTQZQKTCowroB0IHdAXAF2BTgVQCrAqcDhZk4TmbFBj13dQ983sxOHcRR0++9q9x5gDAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAxMpkwP8rO1p");
        s += F("+tFnttRdne1yRLtbh66O9nbGNEwckxuqC409txZClQ9nS+ZWtGbr4WXZ0PpkgAaY9nV8ZNcHIUv9HhSFEyJugTRlOXb4IX086bc9wUVwCmCGAcQsgY0jHLYBXhnTcAngJ/HowAAfBLoBHSmQZwPJL8Do8UiJkKhT9dAARszCsBYyerlPCJUM33kywZMjSrAnKC91BaZj394LEnkDbqs2PdnPStj26Sn5fey13xV3mKnzb5z/hhniZN+j7Ny615wxXWhSm+zo/goic3Mj");
        s += F("N9m1csUjfHxEbI7vxozL7OFkjiLCLwNwMt13r1RWJz+5PFof++8Da1Un39OyK7iejeNimVIp+MkT/drpuW1eniZJ5dP4lQaQ6j2DHfNVjjHaSBfciMAF8dLOlU34Kgpi5e5IYm6XEG9oFLoCUBDEzsnsreq+z5JE5/5wg8hE6v/dKa5mAAOpWBCyArfK9z7tUoG8JWoiZYDlQ8ZyF8qYOLrv4qC9CO4mcKab+XkWhpnPKk0xTOMcMNRAhMWSXdWj6pubKb2R6oRhaKAvil");
        s += F("JgK2qKWyvMOdVqjWmCteBNtp5SlBbGJVADbM8/wQrIj0C7++lbifQVRRCiAJgXcUioAVyE8gzoKouukSOgCaNLHZ1I+OHi+EYlVs6V5v3y9uZB5Umldm84lfEph/BV/Y2TpQwDnjoZ9U7zZ082i0Jybho+AJsvle1/F3xw/dl5rGaQI1Cz+dvi6d9CptoHa05FLfC2yCFRNBE2lJ4K0iz/O/Ac+Fay5FMnBz8vFm9fFoJkZ3/JrrO2gymyi5qYPij+dWkCsC9NOPwv82hgv");
        client.print(s);
        s = F("W8I0C5AtxV8QIjhbg0lv+tg5tXHEq58IBtkWnhu54qLE6eKdgfrBkFIo2vOakfL9zBK2Oho2qRV/Q/B08nd7+PP3B6/Z3+Mg6PsALRr+7v3RHkyLxbZQj3tVj5OM6Ndttbcm/C1Q+cijX3WLXSy7YFcjzAbq0R+TAMSPOhP98QlgTEvKg0R/rAKo2pwZ0d9/e/0uYhGkusYwWPQbo7v9ayjbmLR2GQ8W/V1nmUJtFxdE/3izQP3Id0b0pzch1DYbzIn+dmRG7xNAfVlsU8mDR");
        s += F("//kk7WBO6WBqz4i3rI/2p/az9ZuFJ7j9rrfTfgfSz9tEdX2nq61BFBhFXbboNeunGecc//Wfq6+9avN4OdG5wskfh7tR+DR3zSqraAfpARwaVbtyomgLgTr0Ocz/+bB9Dv7l7kBkX6O3wc6i+gl+rVnAiWiuMkXUqRwKtlb5T/EVHBfIUgXp4sAo9/r1vohK/I+QlgIZYM10e9HABJCmApkgw3R/74I9PVZeQ+1NrAttz3aVNudPAYigK+m+f4Gsco/FAFAu7mMa40Lf2Fso+B");
        s += F("e68JkgBFHPxlg5NFfZYBc6Fq2or0zfPxbNNGvRWHi32bGl1oghCC2wCcBQhhh9COESKJ/4kEIuYEmxd+eYQAAAAAAAAAAAAAAAAAAAAAAAIAW/BNgAJTlfGP7NWKCAAAAAElFTkSuQmCC' /> ");
        s += F("<h2>Row Computer Wifi Setup</h2>");
        client.print(s);
        s = ipStr;
        s += F("<form method='get' action='a'>");
        s += F("<table id='settings'><tr><th>Setting</th><th>Value</th></tr><tr><td><label>SSID: </label></td><td>");
        s += st;
        s += F("<input type = 'submit' value='rescan networks'></td></tr>");
        s += F("<tr><td><label>Password</label></td><td><input name='pass' length=64 size=64/></td></tr>");
        s += F("<tr><td><label>Server:</label></td><td><input name='g' value='rowing.intelligentplant.com' size=64/></td></tr>");
        s += F("<tr><td><label>My Name</label></td><td><input name='n' length=40 value='RowComputer1' size=64/></td></tr>");
        s += F("<tr><td><label>Site</label></td><td><input name='s' length=20 value='Row' size=64/><input type='submit' value='save details'/></td></tr>");
        s += F("</table></form>");
        s += F("<form method='get' action='r'>");
        s += F("</html>\r\n\r\n");
        Serial.println(F("Sending 200"));
    }
      else if (req.startsWith("/r"))
      {//rescan - disconnect and scan for wifi networks - then return.
        client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html> disconnecting and scanning networks, when connected again - click <a href ='/' >here</a>"));
        client.flush(); 
        delay(100);
        setupAP();
      }
      else if ( req.startsWith("/a?ssid=") ) {
        // /a?ssid=blahhhh&pass=poooo&g=demo1&s=pnid
        Serial.println(F("clearing eeprom"));
        for (int i = 0; i < 220; ++i) { EEPROM.write(i, 0); }
        String qsid; 
        int currentstart;
        int currentend;
        currentstart = 8;
        currentend = req.indexOf('&');
        qsid = req.substring(currentstart,currentend);
        Serial.println(qsid);
        Serial.println();
        String qpass;
        currentstart = currentend + 6;
        currentend = req.indexOf('&',currentstart);
        qpass = req.substring(currentstart,currentend);
        Serial.println(qpass);
        Serial.println();
        currentstart = currentend + 3;
        currentend = req.indexOf('&',currentstart);
        String qg;
        qg = req.substring(currentstart,currentend);
        Serial.println(qg);
        Serial.println();
        String qn;//name
        currentstart = currentend + 3;
        currentend = req.indexOf('&',currentstart+1);
        qn = req.substring(currentstart, currentend);
        Serial.println(qn);
        Serial.println(F(""));
        String qs;//site
        currentstart = currentend +3;
        currentend = req.indexOf('&',currentstart+1);
        qs = req.substring(currentstart, currentend);
        Serial.println(qs);
        Serial.println();
        Serial.println(F("writing eeprom ssid:"));
        for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
        Serial.println(F("writing eeprom pass:")); 
        for (int i = 0; i < qpass.length(); ++i)
          {
            EEPROM.write(32+i, qpass[i]);
            Serial.print(F("Wrote: "));
            Serial.println(qpass[i]); 
          }    
        Serial.println(F("writing eeprom g:"));
        for (int i = 0; i < qg.length(); ++i)
        {
          EEPROM.write(96+i, qg[i]);
          Serial.print(F("Wrote: "));
          Serial.println(qg[i]);
        }
        Serial.println(F("writing eeprom n:"));
        for(int i = 0; i < qn.length(); ++i)
        {
          EEPROM.write(160+i, qn[i]);
          Serial.print(F("Wrote: "));
          Serial.println(qn[i]);
        }
        Serial.println(F("writing eeprom s:"));
        for(int i = 0; i < qs.length(); ++i)
        {
          EEPROM.write(200+i, qs[i]);
          Serial.print(F("Wrote: "));
          Serial.println(qs[i]);
        }
        EEPROM.commit();
        s = F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 ");
        s += F("Found ");
        s += req;
        s += F("<p> saved to eeprom... reset to boot into new wifi</html>\r\n\r\n");
      }
      else
      {
        client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html> click here to setup row machine <a href ='/' >here</a>"));
        client.flush(); 
        delay(100);
      }
    /*  else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println(F("Sending 404");
      }*/
  } 
  else
  {
      if (req == "/")
      {
        s = F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266");
        s += F("<p>");
        s += F("</html>\r\n\r\n");
        Serial.println(F("Sending 200"));
      }
      else if ( req.startsWith("/cleareeprom") ) {
        s = F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266");
        s += F("<p>Clearing the EEPROM<p>");
        s += F("</html>\r\n\r\n");
        Serial.println(F("Sending 200"));  
        Serial.println(F("clearing eeprom"));
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        EEPROM.commit();
      }
      else
      {
        s = F("HTTP/1.1 404 Not Found\r\n\r\n");
        Serial.println(F("Sending 404"));
      }       
  }
  client.print(s);
  Serial.println(F("Done with client"));
  return(20);
}
