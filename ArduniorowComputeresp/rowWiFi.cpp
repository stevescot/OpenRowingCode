//
//
//#include "Arduino.h"
#include "rowWiFi.h"
#include "WiFiClient.h"

rowWiFi::rowWiFi()
{

}

rowWiFi::rowWiFi(const char *host, const char *path, WiFiClient &client)
{
	_host = host;
	_path = path;
	_client = client;
	//connect();
}

int rowWiFi::finishSend()
{
  _client.stop();
}

int rowWiFi::connect()
{
	if (!_client.connected())
	{
		Serial.println("connecting");
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
	Serial.println("sendSplit");
	if (connect())
	{
    Serial.println("flushing input.");
    _client.flush();
		Serial.println("Sending to Server: "); Serial.println(_host);
		_client.print("GET /");
		_client.print(_path);
		_client.print("/Upload.aspx?m=");
    _client.print(MAC);
		_client.print("&t=");
		_client.print(msfromStart);
		_client.print("&sd=");
		_client.print(strokeDistance);
		_client.print("&d=");
		_client.print(totalDistancem);
    _client.print("&msD=");
    _client.print(msDrive);
    _client.print("&msR=");
    _client.print(msRecovery);
		_client.print(" HTTP/1.1 \r\n\r\n");
		return true;
	}
	else
	{
		Serial.println("Cannot connect to Server");
		return false;
	}
}


