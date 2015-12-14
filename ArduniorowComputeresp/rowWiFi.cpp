//
//
//#include "Arduino.h"
#include "rowWiFi.h"
#include "WiFiClient.h"

rowWiFi::rowWiFi()
{
	_inRequest = false;
}
rowWiFi::rowWiFi(const char *host, const char *path, WiFiClient &client)
{
	_host = host;
	_path = path;
	_client = client;
	_inRequest = false;
	//connect();
}

int rowWiFi::connect()
{
	if (!_client.connected())
	{
		Serial.println(F("connecting"));
		_client.stop();
		return _client.connect((const char*)_host, 80);
	}
	else
	{
		return true;
	}
}


int rowWiFi::sendSplit(String MAC, unsigned long msfromStart, float strokeDistance, float totalDistancem, unsigned long msDrive, unsigned long msRecovery, int PowerArray[],int PowerSamples)
{
	Serial.println(F("sendSplit"));
	if (connect())
	{
		//if (!_inRequest)
		//{
			Serial.println(F("Sending to Server: ")); Serial.println(_host);
      String request = (F("GET /"));
			request += _path;
			request += F("/upload.aspx?m=");
      request += MAC;
			request += F("&t=");
			request += msfromStart;
			request += F("&sd=");
			request += strokeDistance;
			request += F("&d=");
			request += totalDistancem;
      request += F("&msD=");
      request += msDrive;
      request += F("&msR=");
      request += msRecovery;
      request += F("&Dr=");
      int i = 0;
      while(PowerArray[i] != -1 && i < PowerSamples)
      {
        if(i > 0) request +="%2C";
        request+=PowerArray[i];  
      }
      if(i==0) request +=0;
			request += F("\r\nHost: "); 
			request += _host;
      request += F("\r\nUser-Agent: IPHomeBox/1.0\r\n");
      request += F("Accept: text/html\r\n");
      request +=F("Conection: keep-alive\r\n\r\n");
      _client.print(request);
      request = "";
			//_inRequest = true;
		//}
		return true;
	}
	else
	{
		Serial.println(F("Cannot connect to Server"));
		return false;
	}
}


