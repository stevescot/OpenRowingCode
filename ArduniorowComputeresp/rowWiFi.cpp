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


int rowWiFi::sendSplit(String MAC, unsigned long msfromStart, float strokeDistance, float totalDistancem, unsigned long msDrive, unsigned long msRecovery)
{
	Serial.println(F("sendSplit"));
	if (connect())
	{
		if (!_inRequest)
		{
			Serial.println(F("Sending to Server: ")); Serial.println(_host);
			_client.print(F("GET /"));
			_client.print(_path);
			_client.print(F("/Public/StoreSplit.aspx?m="));
      _client.print(MAC);
			_client.print(F("&t="));
			_client.print(msfromStart);
			_client.print(F("&sd="));
			_client.print(strokeDistance);
			_client.print(F("&d="));
			_client.print(totalDistancem);
      _client.print(F("&msD="));
      _client.print(msDrive);
      _client.print(F("&msR="));
      _client.print(msRecovery);
			_client.print(F(" HTTP/1.1 \r\n\r\n"));
			_inRequest = true;
		}
		return true;
	}
	else
	{
		Serial.println(F("Cannot connect to Server"));
		return false;
	}
}


