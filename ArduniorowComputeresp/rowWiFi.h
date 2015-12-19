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
  int Register(String MAC, String Name);
	int sendSplit(String MAC, unsigned long msfromStart, float strokeDistance, float totalDistancem, unsigned long msDrive, unsigned long msRecovery, int PowerArray[],int PowerSamples);
	int finishSend();
  int connect();
private:
	bool _inRequest;
	const char * _host;
	const char * _path;
	WiFiClient _client;
};


#endif
