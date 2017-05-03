#include "Transponder.h"

#pragma comment(lib,"WS2_32")

std::string Transponder::mac_address = "";
std::atomic<bool> Transponder::initialized = false;

Transponder::Transponder(Aircraft* ac, concurrency::concurrent_unordered_map<std::string, Aircraft*>* intruders, concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* connections, Decider* decider)
{
	decider_ = decider;
	aircraft = ac;
	intrudersMap = intruders;
	open_connections = connections;

	myLocation.set_id(mac_address);
	ip = getIpAddr();
	myLocation.set_ip(ip);

	sinlen = sizeof(struct sockaddr_in);
	inSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (inSocket < 0) {
		XPLMDebugString("failed to open socket to listen for locations\n");
	} else {
		createSocket(&inSocket, &incoming, INADDR_ANY, BROADCAST_PORT);
	}
	outSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (outSocket < 0) {
		XPLMDebugString("failed to open socket to broadcast location\n");
	} else {
		BOOL bOptVal = TRUE;
		setsockopt(outSocket, SOL_SOCKET, SO_BROADCAST, (char *)&bOptVal, sizeof(int));
		//createSocket(&outSocket, &outgoing, -1, BROADCAST_PORT);
		outgoing.sin_addr.s_addr = htonl(-1);
		outgoing.sin_port = htons(BROADCAST_PORT);
		outgoing.sin_family = PF_INET;
		bind(outSocket, (struct sockaddr*)&outgoing, sinlen);
	}
}

void Transponder::createSocket(SOCKET* s, struct sockaddr_in* socket_addr, int addr, int port)
{
	socket_addr->sin_addr.s_addr = htonl(addr);
	socket_addr->sin_port = htons(port);
	socket_addr->sin_family = AF_INET;
	int bind_success = bind(*s, (struct sockaddr *)socket_addr, sinlen);
	if (bind_success < 0) {
		char the_error[32];
		sprintf(the_error, "Transponder::Failed to bind: %d\n", GetLastError());
		XPLMDebugString(the_error);
	}
}

Transponder::~Transponder()
{
	communication = 0;
	closesocket(outSocket);
	closesocket(inSocket);
	WSACleanup();

	// Delete all of the aircraft that the transponder created
	for (std::vector<Aircraft*>::iterator iter = allocated_aircraft.begin(); iter != allocated_aircraft.end(); ) {
		delete *iter;
		iter = allocated_aircraft.erase(iter);
	}
}

DWORD Transponder::receiveLocation()
{
	char const * intruderID;
	char const * myID;

	while (communication)
	{
		int size = myLocation.ByteSize();
		char* buffer = (char*)malloc(size);
		myID = myLocation.id().c_str();
		recvfrom(inSocket, buffer, size, 0, (struct sockaddr *)&incoming, (int *)&sinlen);

		std::chrono::milliseconds ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		intruderLocation.ParseFromArray(buffer, size);
		intruderID = intruderLocation.id().c_str();

		if (strcmp(myID, intruderID) != 0) {
			Angle latitude = { intruderLocation.lat(), Angle::AngleUnits::DEGREES };
			Angle longitude = { intruderLocation.lon(), Angle::AngleUnits::DEGREES };
			Distance altitude = { intruderLocation.alt(), Distance::DistanceUnits::METERS };
			printf("Transponder::recieveLocation - altitude = %d\n", altitude.to_meters());
			LLA updated_position = { intruderLocation.lat(), intruderLocation.lon(), intruderLocation.alt(), Angle::AngleUnits::DEGREES, Distance::DistanceUnits::METERS };

			Aircraft* intruder = (*intrudersMap)[intruderLocation.id()];
			if (!intruder) {
				intruder = new Aircraft(intruderLocation.id(), intruderLocation.ip());
				allocated_aircraft.push_back(intruder);

				// Fill in the current values so that the aircraft will not have two wildly different position values
				// If the position current is not set, position old will get set to LLA::ZERO while position current will
				// be some real value, so setting the position current here prevents the LLAs from being radically different
				intruder->position_current_ = updated_position;
				intruder->position_current_time_ = ms_since_epoch;

				(*intrudersMap)[intruder->id_] = intruder;
				aircraft->lock_.lock();
				ResolutionConnection* connection = new ResolutionConnection(mac_address, intruder->id_, intruder->ip_, ResolutionConnection::kTcpPort_, aircraft);
				(*open_connections)[intruder->id_] = connection;
			}

			keepAliveMap[intruder->id_] = 10;

			ResolutionConnection* conn = (*open_connections)[intruder->id_];

			intruder->lock_.lock();
			aircraft->lock_.lock();
			conn->lock.lock();
			intruder->position_old_ = intruder->position_current_;
			conn->user_position_old = conn->user_position;
			intruder->position_old_time_ = intruder->position_current_time_;
			conn->user_position_old_time = conn->user_position_time;

			intruder->position_current_ = updated_position;
			conn->user_position = aircraft->position_current_;
			intruder->position_current_time_ = ms_since_epoch;
			conn->user_position_time = ms_since_epoch;
			intruder->lock_.unlock();
			aircraft->lock_.unlock();
			conn->lock.unlock();

			decider_->Analyze(intruder);
		}
		free(buffer);
	}
	return 0;
}

DWORD Transponder::sendLocation()
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

		sendto(outSocket, (const char *)buffer, size, 0, (struct sockaddr *) &outgoing, sinlen);

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
				open_connections->unsafe_erase(iter->first);
			}
		}
		Sleep(1000);
	}
	return 0;
}

std::string Transponder::getHardwareAddress()
{
	if (mac_address.empty()) {
		std::string hardware_address{};
		IP_ADAPTER_INFO *pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
		char mac_addr[MAC_LENGTH];

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
		} else {
			XPLMDebugString("Transponder::getHardwareAddress - Failed to retrieve network adapter information.\n");
		}

		free(pAdapterInfo);
		mac_address = hardware_address;
	}
	return mac_address;
}

std::string Transponder::getIpAddr()
{
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	std::string googleDnsIp = "8.8.8.8";
	uint16_t dnsPort = 53;
	struct sockaddr_in server = { 0 };
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	InetPton(AF_INET, googleDnsIp.c_str(), &server.sin_addr.s_addr);
	server.sin_port = htons(dnsPort);

	int result;
	result = connect(sock, (const sockaddr*)&server, sizeof(server));
	if (result == SOCKET_ERROR) {
		XPLMDebugString("Transponder::getIpAddr - unable to connect to \"8.8.8.8\"\n");
		closesocket(sock);
		return "error";
	}

	sockaddr_in name;
	int namelen = sizeof(name);
	char addr[16];
	result = getsockname(sock, (sockaddr*)&name, (int*)&namelen);
	if (result == SOCKET_ERROR) {
		XPLMDebugString("Transponder::getIpAddr - could not get socket info\n");
		closesocket(sock);
		return "error";
	}
	InetNtop(AF_INET, &name.sin_addr.s_addr, addr, 16);
	closesocket(sock);
	std::string ip(addr);
	XPLMDebugString(ip.c_str());
	return ip;
}

static DWORD WINAPI startBroadcasting(void* param)
{
	Transponder* t = (Transponder*)param;
	return t->sendLocation();
}

static DWORD WINAPI startListening(void* param)
{
	Transponder* t = (Transponder*)param;
	return t->receiveLocation();
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

void Transponder::initNetworking()
{
	if (!initialized) {
		initialized = true;
		WSADATA w;
		if (WSAStartup(0x0101, &w) != 0) {
			exit(0);
		}
	}
}

