#include "ResolutionConnection.h"

int ResolutionConnection::number_of_connections_ = 0;

static DWORD WINAPI startResolutionReceiver(void* param)
{
	ResolutionConnection* rc = (ResolutionConnection*)param;
	return rc->senseReceiver();
}

static DWORD WINAPI startResolutionSender(void* param)
{
	ResolutionConnection* rc = (ResolutionConnection*)param;
	return rc->senseSender();
}

ResolutionConnection::ResolutionConnection(std::string const mmac, std::string const imac, std::string const ipAddr, int const portNum, Aircraft* userAc) :
	myMac(mmac), intruderMac(imac), ip(ipAddr), port(portNum)
{
	userPosition = userAc->positionCurrent;
	userPositionTime = userAc->positionCurrentTime;
	userPositionOld = userAc->positionOld;
	userPositionOldTime = userAc->positionOldTime;
	userAc->lock.unlock();

	running_ = true;
	connected_ = false;
	currentSense = Sense::UNKNOWN;
	consensusAchieved = false;

	LPTHREAD_START_ROUTINE task = strcmp(myMac.c_str(), intruderMac.c_str()) > 0 ? startResolutionSender : startResolutionReceiver;
	DWORD threadID;
	CreateThread(NULL, 0, task, (void*) this, 0, &threadID);
}

ResolutionConnection::~ResolutionConnection()
{
	running_ = false;

	// We need to ensure that the thread using the socket has stopped running before we close the socket so we wait until then
	while (!threadStopped_)
	{
		Sleep(100);
	}

	closesocket(sock_);
}

// acceptIncomingIntruder => a client type socket, used in ResolutionConnect::senseReceiver()
// TCP server
SOCKET ResolutionConnection::acceptIncomingIntruder(int port) 
{
	memset(&sock_, 0, sizeof sock_);
	memset(&myAddr_, 0, sizeof myAddr_);
	memset(&intruderAddr_, 0, sizeof intruderAddr_);

	// TCP listening/server type socket
	sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP + number_of_connections_);

	if (sock_ == INVALID_SOCKET) {
		socketDebug("ResolutionConnection::acceptIncomingIntruder - socket failed to open\n", false);
		return NULL;
	}
	// if sock_ == VALID_SOCKET, then increase number_of_connections_
	number_of_connections_++;

	myAddr_.sin_family = AF_INET;
	myAddr_.sin_addr.s_addr = INADDR_ANY;
	myAddr_.sin_port = htons(port); // param port = K_TCP_PORT = 21218

	//test binding
	int bindSuccess = bind(sock_, (struct sockaddr*)&myAddr_, sizeof(myAddr_));

	// if invalid
	if (bindSuccess < 0) {
		char theError[32];
		sprintf(theError, "ResolutionConnection::acceptIncomingIntruder - failed to bind: %d\n", GetLastError());
		XPLMDebugString(theError);
		return NULL;
	}

	SOCKET acceptSocket;

	socklen_t addrLen = sizeof(intruderAddr_);

	// test listening, BE CAREFUL HERE CUZ LISTEN MIGHT NOT RETURN INT (like in mac)
	if (listen(sock_, 24) == -1) {
		char theError[32];
		sprintf(theError, "ResolutionConnection::acceptIncomingIntruder - failed to listen: %d\n", GetLastError());
		XPLMDebugString(theError);
		return NULL;
	}

	// winsock::accept permits an incoming connection attempt on the listening sock_
	acceptSocket = accept(sock_, (struct sockaddr *)&intruderAddr_, &addrLen);
	if (acceptSocket == INVALID_SOCKET) {
		socketCloseWithError("ResolutionConnection::acceptIncomingIntruder - to accept error: %d\n", sock_);
		return NULL;
	}

	return acceptSocket;
}

// senseReceiver open up a listening/accepting socket to serve in ResolutionConnection::startResolutionReceiver()
DWORD ResolutionConnection::senseReceiver()
{
	char* ack = "ACK";
	// acceptSocket is a listening socket wait to accept client connection, is it safe to say this is server socket?
	SOCKET acceptSocket = acceptIncomingIntruder(K_TCP_PORT);

	// if accpetSpclet is true
	if (acceptSocket) {
		connected_ = true;
		openSocket_ = acceptSocket; 
		resolveSense();
	}

	return 0;
}


//  TCP client
//  connects intruder tcp connection to the listening socket in ResolutionConnection::acceptIncomingIntruder()
int ResolutionConnection::connectToIntruder(std::string ip, int port)
{
	struct sockaddr_in dest;
	// sockaddr structs need to be initialized to 0
	memset(&dest, 0, sizeof dest);

	dest.sin_family = AF_INET;
	dest.sin_port = htons(port); // using the same port to connect with tcp server
	dest.sin_addr.s_addr = inet_addr(ip.c_str());

	// reset SOCKET sock_
	memset(&sock_, 0, sizeof sock_);
	
	// client socket
	sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// connect the intruder to tcp listening socket defined in ResolutionConnection::acceptIncomingIntruder()
	if (connect(sock_, (SOCKADDR*)&dest, sizeof(dest)) == SOCKET_ERROR) {
		socketCloseWithError("ResolutionConnection::connectToIntruder - unable to establish tcp connection - error: %d\n", sock_);
		return -1;
	}
	return 0;
}

// senseSender fire up ResConn::connectToIntruder()
DWORD ResolutionConnection::senseSender()
{
	// testing connecting to Intruder
	if (connectToIntruder(ip, port) < 0) {
		std::string dbgstring = "ResolutionConnection::senseSender - failed to establish connection to " + ip + "\n";
		XPLMDebugString(dbgstring.c_str());
	} else {
		connected_ = true;
		openSocket_ = sock_;
		resolveSense();
	}
	return 0;
}


// resolveSense 
void ResolutionConnection::resolveSense()
{
	char msg[256]; // buffer
	static char* ack = "ACK"; // ack = acknowledgement packet is sent back to a client after a server received a message to let the client know the server got the package

	while (running_) // running_ is set true in constructor and set false when it's closed
	{
		// recv wait for a message, if (message) then write message into "msg" buffer
		if (recv(openSocket_, msg, 255, 0) < 0) { // if fail
			socketCloseWithError("ResolutionConnection::resolveSense - Failed to receive: %d\n", openSocket_);
		} else {

			// lock before manipulation
			lock.lock();

			if (strcmp(msg, ack) == 0) { // if msg = ARK that means the server got the message 
				XPLMDebugString("ResolutionConnection::resolveSense - received ack and setting consensus to true\n");
				consensusAchieved = true; 
				lock.unlock();
			} else {
				// received a sense which is not UNKNOWN, store in msg buffer
				
				// if the current sense is unknown
				if (currentSense == Sense::UNKNOWN) {
					// assign new sense in msg buffer to currentSense
					currentSense = stringToSense(msg);

					// send to server the ack confirmation
					if (send(openSocket_, ack, strlen(ack) + 1, 0) == SOCKET_ERROR) { //if fail
						lock.unlock();
						socketCloseWithError("ResolutionConnection::resolveSense - failed to send ack after receiving intruder sense with user_sense unknown\n", openSocket_);
					} else { // if succeed that means server got the ack package confirming server got the message
						consensusAchieved = true;
						lock.unlock();
						XPLMDebugString("ResolutionConnection::resolveSense - achieved consensus in case where intruder sent sense first\n");
					}

				// if the current sense is not unknown that means there is an intruder
				} else {
					XPLMDebugString("ResolutionConnection::resolveSense - edge case entered - current sense is not unknown and received intruder sense\n");
					Sense senseCurrent = currentSense;
					lock.unlock();

					char debugBuf[256];
					char debuggingsense[512];

					// start comparing myMac and intrMac
					snprintf(debuggingsense, 512, "ResolutionConnection::resolveSense Compare \nmyMac: %s\ninteruderMac: %s\n", myMac.c_str(), intruderMac.c_str());
					XPLMDebugString(debuggingsense);

					// if (myMac > intruMac) then send sense (SENDER)
					if (strcmp(myMac.c_str(), intruderMac.c_str()) > 0) {
						snprintf(debugBuf, 256, "ResolutionConnection::resolveSense - edge case with my mac > intr_mac; sending sense: %s\n", senseToString(senseCurrent));
						XPLMDebugString(debugBuf);

						// send sense 
						sendSense(senseCurrent);

						// recv wait for a message, if (message) then write message into "msg" buffer
						if (recv(openSocket_, msg, 255, 0) < 0) {
							socketCloseWithError("ResolutionConnection::resolveSense - edge case - failed to receive: %d\n", openSocket_);
						} else { // if recv is successful
							if (strcmp(msg, ack) == 0) { // server got the message
								lock.lock();
								consensusAchieved = true;
								lock.unlock();


								XPLMDebugString("ResolutionConnection::resolveSense - achieved consensus for edge case with user_mac > intr_mac\n");

							} else {
								socketCloseWithError("ResolutionConnection::resolveSense - Failed to receive ack in edge case with user_mac > intr_mac: %d\n", openSocket_);
							}
						}

					// if (myMac < intrMac) then wait to receive sense (RECEIVER)
					} else {
						snprintf(debugBuf, 256, "ResolutionConnection::resolveSense - edge case with my mac < intr_mac with sense: %s; waiting to receive sense.\n", senseToString(senseCurrent));
						XPLMDebugString(debugBuf);

						
						if (recv(openSocket_, msg, 255, 0) < 0) {
							socketCloseWithError("ResolutionConnection::resolveSense - failed to receive sense from intr in edge case with user_mac < intr_mac: %d\n", openSocket_);
						
						// if recv from openSocket is successful 
						} else {
							Sense senseFromIntruder = stringToSense(msg); // assign new sense from Intrudeer because we're going to be a receiver now
							debugBuf[0] = '\0'; 

							snprintf(debugBuf, 256, "ResolutionConnection::resolveSense - received sense %s from intruder.\n", msg);
							XPLMDebugString(debugBuf);


							if (send(openSocket_, ack, strlen(ack) + 1, 0) == SOCKET_ERROR) {
								socketCloseWithError("ResolutionConnection::resolveSense - Failed to send ack in edge case with user_mac < intr_mac\n", openSocket_);
							} else {
								lock.lock();
								consensusAchieved = true;
								currentSense = senseutil::oppositeFromSense(senseFromIntruder);
								lock.unlock();

								XPLMDebugString("ResolutionConnection::resolveSense - Achieved consensus in edge case with user_mac < intr_mac\n");
							}
						}
					}
				}
			}
		}
	}
	threadStopped_ = true;
}

// sendSense is executed in ResConn::ResolveSense()
int ResolutionConnection::sendSense(Sense currSense)
{
	if (currSense == Sense::UPWARD) {
		currSense = Sense::DOWNWARD;
	} else if (currSense == Sense::DOWNWARD) {
		currSense = Sense::UPWARD;
	} else {
		// Sending a sense of unknown or maintained will cause undefined behavior
		XPLMDebugString("ResolutionConnection::sendSense - Sense is UNKNOWN or MAINTAINED\n");
	}

	// if userAC and intrAC connected 
	if (connected_) {
		char* msg = senseToString(currSense); // store currSense in msg

		// send to openSocket (server) the sense (msg)
		if (send(openSocket_, msg, strlen(msg) + 1, 0) == SOCKET_ERROR) {
			socketCloseWithError("ResolutionConnection::resolveSense - ack failed: %d\n", openSocket_);
			return -1;
		} else {
			return 0; // success
		}
	} else {
		XPLMDebugString("ResolutionConnection::sendSense - attempting to send on an unconnected socket\n");
		return -2;
	}
}

void ResolutionConnection::socketDebug(char* errMsg, bool closeSocket)
{
	running_ = false;

	char debugBuf[128];
	snprintf(debugBuf, 128, "ResolutionConnection::socketDebug - %s\n", errMsg);
	XPLMDebugString(debugBuf);
	if (closeSocket)
		closesocket(sock_);
}

void ResolutionConnection::socketCloseWithError(char* errMsg, SOCKET openSock)
{
	running_ = false;

	char debugBuf[128];
	snprintf(debugBuf, 128, errMsg, GetLastError());
	XPLMDebugString(debugBuf);
	closesocket(openSocket_);
}