#pragma once

#ifndef WINVER
#define WINVER 0x0501
#endif

#include <winsock2.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>

#include "XPLMUtilities.h"

#define PORT 21237
#define MSG_SIZE 256

class Transponder
{
	public:
		char myId[MSG_SIZE];
		char msg[MSG_SIZE];

		Transponder();
		~Transponder();
		DWORD receive();
		DWORD send();
		void start();

	protected:
		WSADATA w;
		SOCKET outSocket, inSocket;
		int status, buflen;
		unsigned sinlen;
		struct sockaddr_in incoming;
		struct sockaddr_in outgoing;
};

