#include "Transponder.h"

#pragma comment(lib,"WS2_32")

Transponder::Transponder(Aircraft* ac, concurrency::concurrent_unordered_map<std::string, Aircraft*>* intruders, Decider* decider)
{
	if (WSAStartup(0x0101, &w) != 0) {
		exit(0);
	}
	decider_ = decider;
	aircraft = ac;
	intrudersMap = intruders;
	myLocation.set_id(getHardwareAddress());
	
	sinlen = sizeof(struct sockaddr_in);
	memset(&incoming, 0, sinlen);
	inSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	incoming.sin_addr.s_addr = htonl(INADDR_ANY);
	incoming.sin_port = htons(PORT);
	incoming.sin_family = PF_INET;
	bind(inSocket, (struct sockaddr *)&incoming, sinlen);

	outSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	BOOL bOptVal = TRUE;
	setsockopt(outSocket, SOL_SOCKET, SO_BROADCAST, (char *)&bOptVal, sizeof(int));
	outgoing.sin_addr.s_addr = htonl(-1);
	outgoing.sin_port = htons(PORT);
	outgoing.sin_family = PF_INET;
	bind(outSocket, (struct sockaddr *)&outgoing, sinlen);
}

Transponder::~Transponder()
{
	communication = 0;
	closesocket(outSocket);
	closesocket(inSocket);
	WSACleanup();
	XPLMDebugString("Transponder::~Transponder - performed WSACleanup()\n");

	for (std::vector<Aircraft*>::iterator iter = allocated_aircraft.begin(); iter != allocated_aircraft.end(); ) {
		delete *iter;
		iter = allocated_aircraft.erase(iter);
	}

	XPLMDebugString("Transponder::~Transponder - deleted");
}

DWORD Transponder::receive()
{
	char const * intruderID;
	char const * myID;

	while (communication)
	{
		int size = myLocation.ByteSize();
		char * buffer = (char *)malloc(size);
		myID = myLocation.id().c_str();
		recvfrom(inSocket, buffer, size, 0, (struct sockaddr *)&incoming, (int *)&sinlen);

		std::chrono::milliseconds ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		intruderLocation.ParseFromArray(buffer, size);
		intruderID = intruderLocation.id().c_str();

		if (strcmp(myID, intruderID) != 0) {
			Angle latitude = { intruderLocation.lat(), Angle::AngleUnits::DEGREES };
			Angle longitude = { intruderLocation.lon(), Angle::AngleUnits::DEGREES };
			Distance altitude = { intruderLocation.alt(), Distance::DistanceUnits::METERS };
			LLA updated_position = { intruderLocation.lat(), intruderLocation.lon(), intruderLocation.alt(), Angle::AngleUnits::DEGREES, Distance::DistanceUnits::METERS };

			char debug_buf[128];
			snprintf(debug_buf, 128, "Transponder::receive - intruderID: %s, pos: (%.3f, %.3f, %.3f)\n", intruderID, latitude.to_degrees(), longitude.to_degrees(), altitude.to_feet());
			XPLMDebugString(debug_buf);

			Aircraft* intruder = (*intrudersMap)[intruderLocation.id()];

			if (!intruder) {
				intruder = new Aircraft(intruderLocation.id());
				allocated_aircraft.push_back(intruder);
				
				// Fill in the current values so that the aircraft will not have to wildly different position values
				// If the position current is not set, position old will get set to LLA::ZERO while position current will
				// be some real value, so setting the position current here prevents the LLAs from being radically different
				intruder->position_current_ = updated_position;
				intruder->position_current_time_ = ms_since_epoch;

				(*intrudersMap)[intruder->id_] = intruder;
			}
				
			keepAliveMap[intruder->id_] = 10;
			intruder->position_old_ = intruder->position_current_;
			intruder->position_old_time_ = intruder->position_current_time_;

			intruder->position_current_ = updated_position;
			intruder->position_current_time_ = ms_since_epoch;

			decider_->Analyze(intruder);
			}
		free(buffer);
	}
	return 0;
}

DWORD Transponder::send()
{
	while (communication)
	{
		aircraft->lock_.lock();
		LLA position = aircraft->position_current_;
		aircraft->lock_.unlock();

		myLocation.set_lat(position.latitude_.to_degrees());
		myLocation.set_lon(position.longitude_.to_degrees());
		myLocation.set_alt(position.altitude_.to_meters());

		int size = myLocation.ByteSize();
		void * buffer = malloc(size);

		myLocation.SerializeToArray(buffer, size);

		sendto(outSocket, (const char *) buffer, size, 0, (struct sockaddr *) &outgoing, sinlen);

		free(buffer);
		Sleep(1000);
	}
	return 0;
}

DWORD Transponder::keepalive()
{
	while (communication)
	{
		concurrency::concurrent_unordered_map<std::string, int>::iterator &iter = keepAliveMap.begin();
		for (; iter != keepAliveMap.cend(); ++iter)
		{
			if (--(iter->second) == 0) {
				intrudersMap->unsafe_erase(iter->first);
				keepAliveMap.unsafe_erase(iter->first);
			}
		}
		Sleep(1000);
	}
	return 0;
}

static DWORD WINAPI startBroadcasting(void* param)
{
	Transponder* t = (Transponder*)param;
	return t->send();
}

static DWORD WINAPI startListening(void* param)
{
	Transponder* t = (Transponder*)param;
	return t->receive();
}

static DWORD WINAPI startKeepAliveTimer(void* param)
{
	Transponder* t = (Transponder*)param;
	return t->keepalive();
}

void Transponder::start()
{
	communication = 1;
	DWORD ThreadID;
	CreateThread(NULL, 0, startListening, (void*) this, 0, &ThreadID);
	CreateThread(NULL, 0, startBroadcasting, (void*) this, 0, &ThreadID);
	CreateThread(NULL, 0, startKeepAliveTimer, (void*) this, 0, &ThreadID);
}

std::string Transponder::getHardwareAddress()
{
	std::string hardware_address{};
	IP_ADAPTER_INFO *pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
	char mac_addr[18];

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
			pAdapter = pAdapter->Next;
		}
	}
	else {
		XPLMDebugString("Transponder::getHardwareAddress - Failed to retrieve network adapter information.\n");
	}

	free(pAdapterInfo);
	return hardware_address;
}