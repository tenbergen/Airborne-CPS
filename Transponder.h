
#include <sys/types.h>
#include <winsock.h>

#pragma once

#define PORT 21237
#define MSG_SIZE 256

class Transponder
{
	public:
		char msg[MSG_SIZE];
		char myId[MSG_SIZE];

		Transponder();
		~Transponder();
		DWORD receive();
		DWORD send();
		void start();

	protected:
		WSADATA w;
		SOCKET outSocket;
		SOCKET inSocket;
		int status;
		int buflen;
		unsigned sinlen;
		struct sockaddr_in incoming;
		struct sockaddr_in outgoing;
};

