#pragma once

#ifndef WINVER
#define WINVER 0x0501
#endif

#if IBM
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#pragma comment(lib, "IPHLPAPI.lib")
#include <iphlpapi.h>

#include <atomic>
#include <vector>
#include <string>
#include <sys/types.h>
#include <stdio.h>
#include <unordered_map>
#include "location.pb.h"
#include "Aircraft.h"
#include "GaugeRenderer.h"
#include "Decider.h"

#include "XPLMUtilities.h"

#define PORT 21221
#define MSG_SIZE 256

class Transponder
{
public:
	
	Transponder(Aircraft*, concurrency::concurrent_unordered_map<std::string, Aircraft*>*, Decider*);
	~Transponder();

	DWORD receive(), send(), keepalive();
	void start();
	static std::string getHardwareAddress();

protected:
	WSADATA w;
	SOCKET outSocket, inSocket;
	int buflen;
	std::atomic<int> communication;
	unsigned sinlen;
	struct sockaddr_in incoming, outgoing;
	xplane::Location intruderLocation, myLocation;
	concurrency::concurrent_unordered_map<std::string, Aircraft*>* intrudersMap;

	struct lla {
		double lat;
		double lon;
		double alt;
	} myLLA;

private:
	Decider * decider_;
	Aircraft* aircraft;
	std::vector<Aircraft*> allocated_aircraft;

	concurrency::concurrent_unordered_map<std::string, int> keepAliveMap;
};