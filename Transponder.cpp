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
		if (intruderID == myLocation.id()) {
			lla.lat = intruder.lat();
			lla.lon = intruder.lon();
			lla.alt = intruder.alt();
			//char * qwe;
			//sprintf(qwe, "(lla) %f:%f:%f\n", lla.lat, lla.lon, lla.alt);
			//XPLMDebugString(qwe);
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

void getPhysicalAddressForUniqueID()
{
	static char uniqueID[128];
	std::string hardware_address{};
	IP_ADAPTER_INFO *pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
	char * mac_addr = (char*)malloc(13);

	DWORD dwRetVal;
	ULONG outBufLen = sizeof(IP_ADAPTER_INFO);

	if (GetAdaptersInfo(pAdapterInfo, &outBufLen) != ERROR_SUCCESS) {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(outBufLen);
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &outBufLen)) == NO_ERROR) {
		PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
		while (pAdapter && hardware_address.empty()) {
			sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
				pAdapterInfo->Address[0], pAdapterInfo->Address[1],
				pAdapterInfo->Address[2], pAdapterInfo->Address[3],
				pAdapterInfo->Address[4], pAdapterInfo->Address[5]);

			hardware_address = mac_addr;
			strcat(uniqueID, mac_addr);

			XPLMDebugString("Hardware Address: ");
			XPLMDebugString(hardware_address.c_str());
			XPLMDebugString("\n");
			pAdapter = pAdapter->Next;
		}
	}
	else {
		XPLMDebugString("Failed to retrieve network adapter information.\n");
	}

	free(pAdapterInfo);
}