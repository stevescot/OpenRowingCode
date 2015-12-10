#include "ESP8266WiFi.h"
#include "rowWifi.h"
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>
WiFiClient client;
WiFiServer server(80);
#include "./DNSServer.h"                  // Patched lib
DNSServer         dnsServer;              // Create the DNS object
const byte        DNS_PORT = 53;          // Capture DNS requests on port 53

MDNSResponder mdns;
String st, host, site;
//SSID when in unassociated mode.
const char * ssid = "IntelligentPlant";
//name of this node.
String myName = "";
String MAC;                  // the MAC address of your Wifi shield
rowWiFi RowServer("demo1.intelligentplant.com","IProw", client);

void setupWiFi() {
    st = "";//empty the option list string to save some memory.
  st.reserve(0);
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
      WiFi.begin(esid.c_str(), epass.c_str());
      if ( testWifi() == 20 ) { 
          //launchWeb(0);
          Serial.println("Connected, returning");
          return;
      }
  }
  setupAP(); 
}

void SendSplit(unsigned long msfromStart, float strokeDistance,  float totalDistancem, unsigned long msDrive, unsigned long msRecovery)
{
  RowServer.sendSplit(MAC, msfromStart, strokeDistance, totalDistancem, msDrive, msRecovery);
}

int testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
 /* while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {return(20); } 
    delay(1000);
    Serial.println(WL_CONNECTED);
    Serial.print(WiFi.status());    
    delay(100);
    Serial.println("rechecking Connection");
    c++;
  }*/
  Serial.println("Connect timed out, opening AP");
  return(10);
} 

void launchWeb(int webtype) {
          Serial.println("");
          Serial.println("WiFi connected");
          Serial.println(WiFi.localIP());
          Serial.println(WiFi.softAPIP());
          //mdns.begin("row", WiFi.softAPIP());
          dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
          // Start the server
          server.begin();
          
          Serial.println("Server started");   
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
