#include "Transponder.h"



#pragma comment(lib,"WS2_32")

std::string Transponder::macAddress_ = "";
std::atomic<bool> Transponder::initialized_ = false;

Transponder::Transponder(Aircraft* ac,
	concurrency::concurrent_unordered_map<std::string, Aircraft*>* intruders,
	concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* connections,
	Decider* decider)
{
	decider_ = decider;
	aircraft_ = ac; //userAc
	intrudersMap = intruders;
	openConnections = connections;

	xb = new XBee();
	enableXBeeRouting = true;

	myLocation.setID(macAddress_);

	ip = getIpAddr(); // ip of hardware xbee
	myLocation.setIP(ip);

	sinlen = sizeof(struct sockaddr_in);
	inSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); // inSocket is the listening socket
	if (inSocket < 0) {
		XPLMDebugString("failed to open socket to listen for locations\n");
	}
	else {													
		createSocket(&inSocket, &incoming, INADDR_ANY, BROADCAST_PORT); // creat socket to bind with incoming socket
	}
	outSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); // outSocket to broadcast udp connection
	if (outSocket < 0) {
		XPLMDebugString("failed to open socket to broadcast location\n");
	}
	else {
		BOOL bOptVal = TRUE;
		setsockopt(outSocket, SOL_SOCKET, SO_BROADCAST, (char*)&bOptVal, sizeof(int)); // set options on outSocket
		outgoing.sin_addr.s_addr = htonl(-1);	//htonl converts unsigned long into network byte order
		outgoing.sin_port = htons(BROADCAST_PORT);
		outgoing.sin_family = PF_INET;
		if (bind(outSocket, (struct sockaddr*)&outgoing, sinlen) < 0) 
		{
			XPLMDebugString("Can't bind");
		};
	}
}

void Transponder::createSocket(SOCKET* s, struct sockaddr_in* socketAddr, int addr, int port)
{
	socketAddr->sin_addr.s_addr = htonl(addr);
	socketAddr->sin_port = htons(port);
	socketAddr->sin_family = AF_INET;
	int bindSuccess = bind(*s, (struct sockaddr*)socketAddr, sinlen);
	if (bindSuccess < 0) {
		char theError[32];
		sprintf(theError, "Transponder::createSocket() -> Failed to bind: %d\n", GetLastError());
		XPLMDebugString(theError);
	}
}

Transponder::~Transponder()
{
	communication = 0;
	closesocket(outSocket);
	closesocket(inSocket);
	WSACleanup();

	// Delete all of the aircraft that the transponder created
	for (std::vector<Aircraft*>::iterator iter = allocatedAircraft_.begin(); iter != allocatedAircraft_.end(); ) {
		delete* iter;
		iter = allocatedAircraft_.erase(iter);
	}
}


// receiveLocation get the userID and intruderID then apply Transponder::processIntruder if there is any intruder in the udp subnet
// receiveLocation listens to udp connections
DWORD Transponder::receiveLocation()
{

	std::string myID;
	myID = myLocation.getID();	// store myID as a C++ string

	std::string intruderID;


	while (communication)
	{


		char* buffer = (char*)malloc(MAX_RECEIVE_BUFFER_SIZE);		// allocate a buffer big enough to hold that max possible size
		memset(buffer, '\0', MAX_RECEIVE_BUFFER_SIZE);				 // fill it with null terminators



		// receive information from a intruder udp socket and store it into the buffer
		recvfrom(inSocket, buffer, MAX_RECEIVE_BUFFER_SIZE, 0, (struct sockaddr*) & incoming, (int*)&sinlen);


		char receivedIP[INET_ADDRSTRLEN];
		struct sockaddr_in* pincoming = &incoming;

		// inet_ntop converts an address from network format to presentation format
		inet_ntop(AF_INET, &(pincoming->sin_addr), receivedIP, INET_ADDRSTRLEN);


		// filter out any packets that were broadcasted from self
		if (strncmp(receivedIP, myID.c_str(), INET_ADDRSTRLEN) != 0) {

			int size = std::strlen(buffer);  // set size the actual number of characters that we received
											// even if we don't get the null sent to us, our buffer should be null filled anyways
											// which will mean its effectively null terminated as long as we receive less than MAX_RECEIVE_BUFFER_SIZE characters



			try {
				//// ******** debugging
				//std::string debugString = "receiveLocation buffer: " + std::string(buffer) + "\n";

				//XPLMDebugString(debugString.c_str());
				//// ****************
				intruderLocation.deserialize(buffer, size);
			}
			catch (...) {
				XPLMDebugString("Deserialize is not working: ");
			}

			// get the ID from xplane::Location 
			intruderID = intruderLocation.getID().c_str();

			// process any possible intruder we received via UDP
			if (strcmp(myID.c_str(), intruderID.c_str()) != 0)  // redundant but fine to keep for now
			{
				processIntruder(intruderID);

				// XBee <-> UDP Routing
				if (enableXBeeRouting) {
					std::unique_lock<std::mutex> lockXB(mQueueXB);
					condXB.wait(lockXB, [this]() { return queueXB.size() < MAX_BRIDGE_QUEUE_SIZE; });
					qPayload.assign(buffer);
					queueXB.push(qPayload);
					lockXB.unlock();
					condXB.notify_one();
				}

			}

			// repeat the same steps, but for any pending XBee data 
			memset(buffer, '\0', MAX_RECEIVE_BUFFER_SIZE);		// clean out the buffer to reuse it for xbee data


			if (xb->XBeeReceive(xbComm, buffer, MAX_RECEIVE_BUFFER_SIZE)) {
				// Debugging
				//XPLMDebugString("XB Payload: ");
				//XPLMDebugString(buffer);
				//XPLMDebugString("\n");
				//


				int size = std::strlen(buffer);
				intruderLocation.deserialize(buffer, size);
				intruderID = intruderLocation.getID().c_str();
				if (strcmp(myID.c_str(), intruderID.c_str()) != 0)
				{
					processIntruder(intruderID);  // process any possible intruder we received via XBee

					// XBee <-> UDP Routing
					if (enableXBeeRouting) {
						std::unique_lock<std::mutex> lockUDP(mQueueUDP);
						condUDP.wait(lockUDP, [this]() { return queueUDP.size() < MAX_BRIDGE_QUEUE_SIZE; });
						qPayload.assign(buffer);
						queueUDP.push(qPayload);
						lockUDP.unlock();
						condUDP.notify_one();
					}
				}

			}
			free(buffer);   // free memory on the heap
			buffer = nullptr;  // clear dangling pointer
		}
	}
	return 0;
}


// processIntruder analyze intruder and user
DWORD Transponder::processIntruder(std::string intruderID)
{

	std::chrono::milliseconds msSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());



	LLA updatedPosition = { intruderLocation.getLAT(), intruderLocation.getLON(), intruderLocation.getALT(), Angle::AngleUnits::DEGREES, Distance::DistanceUnits::METERS };

	Aircraft* intruder = (*intrudersMap)[intruderLocation.getID()];
	if (!intruder) {

		// if we don't have this intruder already, create it

		// Debug Statements to output Intruder MAC and IP addresses to Log
		std::string debugString = "Intruder MAC : " + intruderLocation.getID() + "\nIntruder IP : " + intruderLocation.getIP() + "\n";
		XPLMDebugString(debugString.c_str());


		intruder = new Aircraft(intruderLocation.getID(), intruderLocation.getIP());
		
		allocatedAircraft_.push_back(intruder); // add the intruder at the end

		// Fill in the current values so that the aircraft will not have two wildly different position values
		// If the position current is not set, position old will get set to LLA::ZERO while position current will
		// be some real value, so setting the position current here prevents the LLAs from being radically different
		intruder->positionCurrent = updatedPosition;
		intruder->positionCurrentTime = msSinceEpoch;

		(*intrudersMap)[intruder->id] = intruder;
		aircraft_->lock.lock();

		// tcp/ip connection
		ResolutionConnection* connection = new ResolutionConnection(macAddress_, intruder->id, intruder->ip, ResolutionConnection::K_TCP_PORT, aircraft_);
		(*openConnections)[intruder->id] = connection;
	}

	keepAliveMap_[intruder->id] = 10;  // what does 10 mean here? Why 10? 10 what? Why not 9 or 11000000? Magic number alert!

	// tcp/ip connection
	ResolutionConnection* conn = (*openConnections)[intruder->id];

	// lock the aircrafts before manipulate the values
	intruder->lock.lock();
	aircraft_->lock.lock();
	conn->lock.lock();

	// old time
	intruder->positionOld = intruder->positionCurrent;
	conn->userPositionOld = conn->userPosition;
	intruder->positionOldTime = intruder->positionCurrentTime;
	conn->userPositionOldTime = conn->userPositionTime;

	//current time
	intruder->positionCurrent = updatedPosition;
	conn->userPosition = aircraft_->positionCurrent;
	intruder->positionCurrentTime = msSinceEpoch;
	conn->userPositionTime = msSinceEpoch;

	// unlock
	intruder->lock.unlock();
	aircraft_->lock.unlock();
	conn->lock.unlock();

	// analyze intruder to determine which threat class intruder in
	decider_->analyze(intruder);
	return 0;
}

// sendLocation is used in Transponder::startBroadcasting
// sendLocation sends out udp conection
DWORD Transponder::sendLocation()
{
	while (communication)
	{
		// lock ACs before manipulate data
		aircraft_->lock.lock();
		LLA position = aircraft_->positionCurrent; //get user location
		aircraft_->lock.unlock();

		// setup xplane:Location mylocation to get the values from userAC
		myLocation.setLAT(position.latitude.toDegrees());
		myLocation.setLON(position.longitude.toDegrees());
		myLocation.setALT(position.altitude.toMeters());


		// build the ac
		myLocation.BuildPlane(); 

		int size = myLocation.getPLANE().length() + 1;  // length of the C++ string, plus 1 for the null terminator

		char* buffer = (char*)malloc(size);  // to be consistent with how its done in receiveLocation()
		// clean the buffer
		memset(buffer, '\0', size);

		// copy mylocation plane to buffer
		std::strcpy(buffer, myLocation.getPLANE().c_str());


		// UDP Broadcast
		sendto(outSocket, (const char*)buffer, size, 0, (struct sockaddr*) & outgoing, sinlen);

		// XBee Broadcast
		xb->XBeeBroadcast(myLocation.getPLANE(), xbComm);


		// XBee <-> UDP Routing
		if (enableXBeeRouting) {
			// process any UDP -> XBee forwarding that is queued
			std::unique_lock<std::mutex> lockXB(mQueueXB);
			while (!queueXB.empty()) {
				condXB.wait(lockXB, [this]() { return !queueXB.empty(); });
				xb->XBeeBroadcast(queueXB.front(), xbComm);
				queueXB.pop();
				condXB.notify_one();
			}
			lockXB.unlock();


			std::unique_lock<std::mutex> lockUDP(mQueueUDP);
			std::string outpayload;
			while (!queueUDP.empty()) {
				condUDP.wait(lockUDP, [this]() { return !queueUDP.empty(); });
				// send the udp packect
				//outpayload = queueUDP.front();
				int qsize = strlen(queueUDP.front().c_str());
				sendto(outSocket, (const char*)queueUDP.front().c_str(), qsize, 0, (struct sockaddr*) & outgoing, sinlen);
				queueUDP.pop();
				condUDP.notify_one();
			}
			lockUDP.unlock();

		}

		free(buffer);
		buffer = nullptr;
		Sleep(1000);

	}
	return 0;
}

DWORD Transponder::keepalive()
{
	while (communication)
	{
		concurrency::concurrent_unordered_map<std::string, int>::iterator& iter = keepAliveMap_.begin();
		for (; iter != keepAliveMap_.cend(); ++iter)
		{
			if (--(iter->second) == 0) {
				intrudersMap->unsafe_erase(iter->first);
				keepAliveMap_.unsafe_erase(iter->first);
				openConnections->unsafe_erase(iter->first);
			}
		}
		Sleep(1000);
	}
	return 0;
}

std::string Transponder::getHardwareAddress()
{
	if (macAddress_.empty()) {
		std::string hardwareAddress{};
		IP_ADAPTER_INFO* pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
		char macAddr[MAC_LENGTH];

		DWORD dwRetVal;
		ULONG outBufLen = sizeof(IP_ADAPTER_INFO);

		if (GetAdaptersInfo(pAdapterInfo, &outBufLen) != ERROR_SUCCESS) {
			free(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO*)malloc(outBufLen);
		}

		if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &outBufLen)) == NO_ERROR) {
			PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
			while (pAdapter && hardwareAddress.empty()) {
				sprintf(macAddr, "%02X:%02X:%02X:%02X:%02X:%02X",
					pAdapterInfo->Address[0], pAdapterInfo->Address[1],
					pAdapterInfo->Address[2], pAdapterInfo->Address[3],
					pAdapterInfo->Address[4], pAdapterInfo->Address[5]);

				hardwareAddress = macAddr;
				pAdapter = pAdapter->Next;
			}
		}
		else {
			XPLMDebugString("Transponder::getHardwareAddress - Failed to retrieve network adapter information.\n");
		}

		free(pAdapterInfo);
		macAddress_ = hardwareAddress;
	}
	return macAddress_;
}

// IP adress from UDP
std::string Transponder::getIpAddr()
{
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
	std::string googleDnsIp = "8.8.8.8";
	uint16_t dnsPort = 53;

	// udp server
	struct sockaddr_in server = { 0 };
	// clean server
	memset(&server, 0, sizeof(server));
	// add sin_fam to server, sin_fam is always AF_INET
	server.sin_family = AF_INET;
	// convert ipaddr to binary
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
	CreateThread(NULL, 0, startListening, (void*)this, 0, &ThreadID);
	CreateThread(NULL, 0, startBroadcasting, (void*)this, 0, &ThreadID);
	CreateThread(NULL, 0, startKeepAliveTimer, (void*)this, 0, &ThreadID);
}

void Transponder::initNetworking()
{
	if (!initialized_) {
		initialized_ = true;
		WSADATA w;
		if (WSAStartup(0x0101, &w) != 0) {
			exit(0);
		}
	}
}

void Transponder::initXBee(unsigned int portnum) {
	xb->SetPortnum(portnum);
	xbComm = xb->InitializeComPort(portnum);
}

bool Transponder::isXBeeRoutingEnabled() {
	return enableXBeeRouting;
}


