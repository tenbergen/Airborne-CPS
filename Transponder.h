
#include <sys/types.h>
#include <winsock.h>
#include "location.pb.h"

#pragma once

#define PORT 21237
#define MSG_SIZE 256

class Transponder
{
	public:
		char msg[MSG_SIZE];
		Transponder();
		~Transponder();
		DWORD receive();
		DWORD send();
		void start();

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

