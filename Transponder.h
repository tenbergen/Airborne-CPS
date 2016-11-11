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

#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include "location.pb.h"
#include "XPLMUtilities.h"

#define PORT 21237
#define MSG_SIZE 256

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

class Transponder
{
	public:
		char msg[MSG_SIZE];
		Transponder();
		~Transponder();
		DWORD receive();
		DWORD send();
		void start();
		static void getPhysicalAddressForUniqueID(void);

	protected:
		WSADATA w;
		SOCKET outSocket, inSocket;
		int buflen;
		unsigned sinlen;
		struct sockaddr_in incoming, outgoing;
		tcas::Location intruder, myLocation;
		struct {
			double lat;
			double lon;
			double alt;
		} lla;
};

