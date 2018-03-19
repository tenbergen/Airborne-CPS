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

SOCKET ResolutionConnection::acceptIncomingIntruder(int port)
{
	memset(&sock_, 0, sizeof sock_);
	memset(&myAddr_, 0, sizeof myAddr_);
	memset(&intruderAddr_, 0, sizeof intruderAddr_);

	sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP + number_of_connections_);
	if (sock_ == INVALID_SOCKET) {
		socketDebug("ResolutionConnection::acceptIncomingIntruder - socket failed to open\n", false);
		return NULL;
	}
	number_of_connections_++;

	myAddr_.sin_family = AF_INET;
	myAddr_.sin_addr.s_addr = INADDR_ANY;
	myAddr_.sin_port = htons(K_TCP_PORT);

	int bindSuccess = bind(sock_, (struct sockaddr*)&myAddr_, sizeof(myAddr_));

	if (bindSuccess < 0) {
		char theError[32];
		sprintf(theError, "ResolutionConnection::acceptIncomingIntruder - failed to bind: %d\n", GetLastError());
		XPLMDebugString(theError);
		return NULL;
	}
	SOCKET acceptSocket;
	socklen_t addrLen = sizeof(intruderAddr_);
	listen(sock_, 24);

	acceptSocket = accept(sock_, (struct sockaddr *)&intruderAddr_, &addrLen);
	if (acceptSocket == INVALID_SOCKET) {
		socketCloseWithError("ResolutionConnection::acceptIncomingIntruder - to accept error: %d\n", sock_);
		return NULL;
	}
	return acceptSocket;
}

DWORD ResolutionConnection::senseReceiver()
{
	char* ack = "ACK";
	SOCKET acceptSocket = acceptIncomingIntruder(K_TCP_PORT);

	if (acceptSocket) {
		connected_ = true;
		openSocket_ = acceptSocket;
		resolveSense();
	}

	return 0;
}

int ResolutionConnection::connectToIntruder(std::string ip, int port)
{
	struct sockaddr_in dest;
	// sockaddr structs need to be initialized to 0
	memset(&dest, 0, sizeof dest);

	dest.sin_family = AF_INET;
	dest.sin_port = htons(K_TCP_PORT);
	dest.sin_addr.s_addr = inet_addr(ip.c_str());

	memset(&sock_, 0, sizeof sock_);
	sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connect(sock_, (SOCKADDR*)&dest, sizeof(dest)) == SOCKET_ERROR) {
		socketCloseWithError("ResolutionConnection::connectToIntruder - unable to establish tcp connection - error: %d\n", sock_);
		return -1;
	}
	return 0;
}

DWORD ResolutionConnection::senseSender()
{
	if (connectToIntruder(ip, port) < 0) {
		XPLMDebugString("ResolutionConnection::senseSender - failed to establish connection\n");
	} else {
		connected_ = true;
		openSocket_ = sock_;
		resolveSense();
	}
	return 0;
}

void ResolutionConnection::resolveSense()
{
	char msg[256];
	static char* ack = "ACK";

	while (running_)
	{
		if (recv(openSocket_, msg, 255, 0) < 0) {
			socketCloseWithError("ResolutionConnection::resolveSense - Failed to receive: %d\n", openSocket_);
		} else {

			lock.lock();
			if (strcmp(msg, ack) == 0) {
				XPLMDebugString("ResolutionConnection::resolveSense - received ack and setting consensus to true\n");
				consensusAchieved = true;
				lock.unlock();
			} else {
				// received a sense
				if (currentSense == Sense::UNKNOWN) {
					currentSense = stringToSense(msg);

					if (send(openSocket_, ack, strlen(ack) + 1, 0) == SOCKET_ERROR) {
						lock.unlock();
						socketCloseWithError("ResolutionConnection::resolveSense - failed to send ack after receiving intruder sense with user_sense unknown\n", openSocket_);
					} else {
						consensusAchieved = true;
						lock.unlock();
						XPLMDebugString("ResolutionConnection::resolveSense - achieved consensus in case where intruder sent sense first\n");
					}
				} else {
					XPLMDebugString("ResolutionConnection::resolveSense - edge case entered - current sense != unknown and received intruder sense\n");
					Sense senseCurrent = currentSense;
					lock.unlock();

					char debugBuf[256];
					if (strcmp(myMac.c_str(), intruderMac.c_str()) > 0) {
						snprintf(debugBuf, 256, "ResolutionConnection::resolveSense - edge case with my mac > intr_mac; sending sense: %s\n", senseToString(senseCurrent));
						XPLMDebugString(debugBuf);
						sendSense(senseCurrent);

						if (recv(openSocket_, msg, 255, 0) < 0) {
							socketCloseWithError("ResolutionConnection::resolveSense - edge case - failed to receive: %d\n", openSocket_);
						} else {
							if (strcmp(msg, ack) == 0) {
								lock.lock();
								consensusAchieved = true;
								lock.unlock();

								XPLMDebugString("ResolutionConnection::resolveSense - achieved consensus for edge case with user_mac > intr_mac\n");

							} else {
								socketCloseWithError("ResolutionConnection::resolveSense - Failed to receive ack in edge case with user_mac > intr_mac: %d\n", openSocket_);
							}
						}
					} else {
						snprintf(debugBuf, 256, "ResolutionConnection::resolveSense - edge case with my mac < intr_mac with sense: %s; waiting to receive sense.\n", senseToString(senseCurrent));
						XPLMDebugString(debugBuf);
						if (recv(openSocket_, msg, 255, 0) < 0) {
							socketCloseWithError("ResolutionConnection::resolveSense - failed to receive sense from intr in edge case with user_mac < intr_mac: %d\n", openSocket_);
						} else {
							Sense senseFromIntruder = stringToSense(msg);
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

int ResolutionConnection::sendSense(Sense s)
{
	if (s == Sense::UPWARD) {
		s = Sense::DOWNWARD;
	} else if (s == Sense::DOWNWARD) {
		s = Sense::UPWARD;
	} else {
		// Sending a sense of unknown or maintained will cause undefined behavior
		XPLMDebugString("ResolutionConnection::sendSense - Sense is UNKNOWN or MAINTAINED\n");
	}

	if (connected_) {
		char* msg = senseToString(s);
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