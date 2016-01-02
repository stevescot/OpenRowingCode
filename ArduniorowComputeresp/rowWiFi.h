#ifndef rowWifi_h
#define rowWifi_h

//#include <Ethernet.h>
#include <WiFiClient.h>
//#include <Arduino.h>

class rowWiFi
{
public:
	rowWiFi(const char *host, const char *path);
	rowWiFi();
  int Register(String MAC, String Name);
	int sendSplit(String MAC, unsigned long msfromStart, float strokeDistance, float totalDistancem, unsigned long msDrive, unsigned long msRecovery, int spm, int PowerArray[],int PowerSamples, String statusstr, int lastCommand);
	int finishSend();
  int connect();
  WiFiClient* wificlient;
private:
	bool _inRequest;
	const char * _host;
	const char * _path;
};


#endif
