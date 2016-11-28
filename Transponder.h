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

#include <string>
#include <sys/types.h>
#include <stdio.h>
#include <unordered_map>
#include "location.pb.h"
#include "Aircraft.h"
#include "GaugeRenderer.h"

#include "XPLMUtilities.h"

#define PORT 21221
#define MSG_SIZE 256
#define ON 1
#define OFF 0

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

class Transponder
{
public:
	Aircraft* aircraft;
	Transponder(Aircraft*, concurrency::concurrent_unordered_map<std::string, Aircraft*>*);
	~Transponder();
	DWORD receive(), send(), keepalive();
	void start();
	static std::string getHardwareAddress();

protected:
	WSADATA w;
	SOCKET outSocket, inSocket;
	int buflen, communication;
	unsigned sinlen;
	struct sockaddr_in incoming, outgoing;
	xplane::Location intruderLocation, myLocation;
	concurrency::concurrent_unordered_map<std::string, Aircraft*>* intrudersMap;
	concurrency::concurrent_unordered_map<std::string, int> keepAliveMap;
};