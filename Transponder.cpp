
#include "Transponder.h"

#pragma comment(lib,"WS2_32")

Transponder::Transponder()
{
	if (WSAStartup(0x0101, &w) != 0) {
		exit(0);
	}

	strcpy(myId, "12345");
	sinlen = sizeof(struct sockaddr_in);
	memset(&incoming, 0, sinlen);

	inSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	incoming.sin_addr.s_addr = htonl(INADDR_ANY);
	incoming.sin_port = htons(PORT);
	incoming.sin_family = PF_INET;

	bind(inSocket, (struct sockaddr *)&incoming, sinlen);

	outSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	BOOL bOptVal = TRUE;
	setsockopt(outSocket, SOL_SOCKET, SO_BROADCAST, (char *) &bOptVal, sizeof(int));

	outgoing.sin_addr.s_addr = htonl(-1);
	outgoing.sin_port = htons(PORT);
	outgoing.sin_family = PF_INET;

	bind(outSocket, (struct sockaddr *)&outgoing, sinlen);
}


Transponder::~Transponder()
{
	closesocket(outSocket);
	closesocket(inSocket);
	WSACleanup();
}

DWORD Transponder::receive()
{
	// listen for a message
	for (;;)
	{
		char tempMsg[MSG_SIZE];
		recvfrom(inSocket, tempMsg, MSG_SIZE, 0, (struct sockaddr *)&incoming, (int *)&sinlen);
		if (strcmp(tempMsg, myId) != 0) {
			strcpy(msg, tempMsg);
		}
		
	}
	return 0;
}

DWORD Transponder::send()
{
	char query[256];

	strcpy(query, "12345");

	for (;;)
	{
		sendto(outSocket, query, strlen(msg), 0, (struct sockaddr *) &outgoing, sinlen);
		Sleep(1000);
	}
}

static DWORD WINAPI startBroadcasting(void* param)
{
	Transponder* t = (Transponder*) param;
	return t->send();
}

static DWORD WINAPI startListening(void* param)
{
	Transponder* t = (Transponder*) param;
	return t->receive();
}

void Transponder::start()
{
	DWORD ThreadID;
	CreateThread(NULL, 0, startListening, (void*) this, 0, &ThreadID);
	CreateThread(NULL, 0, startBroadcasting, (void*) this, 0, &ThreadID);
}
