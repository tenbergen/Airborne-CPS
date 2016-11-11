#include "Transponder.h"

#pragma comment(lib,"WS2_32")

Transponder::Transponder()
{
	if (WSAStartup(0x0101, &w) != 0) {
		exit(0);
	}

	myLocation.set_id(12345); // TODO use mac address
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
	char incomingMessage[MSG_SIZE];
	int intruderID;
	for (;;)
	{
		recvfrom(inSocket, incomingMessage, MSG_SIZE, 0, (struct sockaddr *)&incoming, (int *)&sinlen);
		intruder.ParseFromString(incomingMessage);
		intruderID = intruder.id();
		if (intruderID != myLocation.id()) {
			lla.lat = intruder.lat();
			lla.lon = intruder.lon();
			lla.alt = intruder.alt();
		}
		// TODO map the aircraft
	}
	return 0;
}

DWORD Transponder::send()
{
	std::string serializedLLA;
	for (;;)
	{
		// TODO get datarefs
		myLocation.set_lat(1.1);
		myLocation.set_lon(2.1);
		myLocation.set_alt(3.2);
		myLocation.SerializeToString(&serializedLLA);
		const char* tempLLA = serializedLLA.c_str();
		sendto(outSocket, tempLLA, strlen(msg), 0, (struct sockaddr *) &outgoing, sinlen);
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
