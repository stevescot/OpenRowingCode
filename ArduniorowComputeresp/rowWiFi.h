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
	int sendSplit(String MAC, unsigned long msfromStart, float strokeDistance, float totalDistancem, unsigned long msDrive, unsigned long msRecovery);
	int finishSend();
private:
	const char * _host;
	const char * _path;
	WiFiClient _client;
	int connect();
};


#endif
