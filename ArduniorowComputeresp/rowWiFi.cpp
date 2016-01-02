//
//
//#include "Arduino.h"
#include "rowWiFi.h"
#include "WiFiClient.h"

rowWiFi::rowWiFi()
{
	_inRequest = false;
}
rowWiFi::rowWiFi(const char *host, const char *path)
{
	_host = host;
	_path = path;
	_inRequest = false;
}

int rowWiFi::connect()
{
	if (!wificlient->connected())
	{
		Serial.println(F("connecting"));
		wificlient->stop();
		return wificlient->connect((const char*)_host, 80);
	}
	else
	{
  while(wificlient->available())
  {
    Serial.print((char)wificlient->read());
  }
    //_client.flush();//get rid of any returned data - one way connection
		return true;
	}
}

int rowWiFi::Register(String MAC, String Name)
{
  Serial.println(F("register"));
  if (connect())
  {
    Serial.println(F("Sending to Server: ")); Serial.println(_host);
    String request = "GET /";
    request += _path;
    request += "/register.aspx?m=";
    request += MAC;
    request += "&Name=";
    request += Name;
    request += " HTTP/1.1\r\nHost: "; 
    request += _host;
    request += "\r\nUser-Agent: IPHomeBox/1.0\r\n";
    request += "Accept: text/html\r\n";
    request += "Conection: keep-alive\r\n\r\n";
    wificlient->print(request);
    return true;
  }
  else
  {
    Serial.println(F("Cannot connect to Server"));
    return false;
  }
}


int rowWiFi::sendSplit(String MAC, unsigned long msfromStart, float strokeDistance, float totalDistancem, unsigned long msDrive, unsigned long msRecovery, int spm,  int PowerArray[],int PowerSamples)
{
	Serial.println(F("sendSplit"));
	if (connect())
	{
		//if (!_inRequest)
		//{
			Serial.println(F("Sending to Server: ")); Serial.println(_host);
      String request = "GET /";
			request += _path;
			request += "/upload.aspx?m=";
      request += MAC;
			request += "&t=";
			request += msfromStart;
			request += "&sd=";
			request += strokeDistance;
			request += "&d=";
			request += totalDistancem;
      request += "&msD=";
      request += msDrive;
      request += "&msR=";
      request += msRecovery;
      request += "&Dr=";
      int i = 0;
      while(PowerArray[i] != -1 && i < PowerSamples)
      {
        if(i > 0) 
        {
          request +="%2C";
        }
        request+=PowerArray[i];  
        i++;
      }
      request += "&spm=";
      request += spm;
      if(i==0) request +=0;
			request += " HTTP/1.1\r\nHost: "; 
			request += _host;
      request += "\r\nUser-Agent: IPHomeBox/1.0\r\n";
      request += "Accept: text/html\r\n";
      request += "Conection: keep-alive\r\n\r\n";
      
      Serial.println();
      Serial.println();
      Serial.print(request);
      wificlient->print(request);
		return true;
	}
	else
	{
		Serial.println(F("Cannot connect to Server"));
		return false;
	}
}


