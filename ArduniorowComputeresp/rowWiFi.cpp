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
    request += "\r\nHost: "; 
    request += _host;
    request += "\r\nUser-Agent: IPHomeBox/1.0\r\n";
    request += "Accept: text/html\r\n";
    request += "Conection: keep-alive\r\n\r\n";
    _client.print(request);
    request = "";
    return true;
  }
  else
  {
    Serial.println(F("Cannot connect to Server"));
    return false;
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
        if(i > 0) request +="%2C";
        request+=PowerArray[i];  
        i++;
      }
      if(i==0) request +=0;
			request += "\r\nHost: "; 
			request += _host;
      request += "\r\nUser-Agent: IPHomeBox/1.0\r\n";
      request += "Accept: text/html\r\n";
      request += "Conection: keep-alive\r\n\r\n";
//      Serial.println("about to send request");
//      delay(400);
      _client.print(request);
//      Serial.println("sent");
//      delay(100);
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


