#ifndef rowWifi_h
#define rowWifi_h

//#include <Ethernet.h>
#include <WiFiClient.h>
//#include <Arduino.h>

class rowWiFi
{
public:
	rowWiFi(const char *host, const char *path, WiFiClient &client);
	rowWiFi();
	int sendSplit(unsigned long msfromStart, float splitDistance, float totalDistancem);
	int finishSend();
private:
	bool _inRequest;
	const char * _host;
	const char * _path;
	WiFiClient _client;
	int connect();
};


#endif