#include "ResolutionConnection.h"

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

ResolutionConnection::ResolutionConnection(std::string const mmac, std::string const imac, std::string const ip_addr, int const port_num) : 
	my_mac(mmac), intruder_mac(imac), ip(ip_addr), port(port_num)
{
	running = true;
	connected = false;
	current_sense = Sense::UNKNOWN;
	
	LPTHREAD_START_ROUTINE task;
	if (strcmp(my_mac.c_str(), intruder_mac.c_str()) > 0) {
		task = startResolutionSender;
	} else {
		task = startResolutionReceiver;
	}
	XPLMDebugString("ResolutionConnection::ResolutionConnection - task set\n");
	DWORD ThreadID;
	CreateThread(NULL, 0, task, (void*) this, 0, &ThreadID);
	XPLMDebugString("ResolutionConnection::ResolutionConnection - thread started\n");
}

ResolutionConnection::~ResolutionConnection()
{
	running = false;
	while (!thread_stopped)
	{
		Sleep(100);
	}
	XPLMDebugString("ResolutionConnection::~ResolutionConnection - attempting to close socket\n");
	closesocket(sock);
	XPLMDebugString("ResolutionConnection::~ResolutionConnection - closed socket\n");
}

SOCKET ResolutionConnection::acceptIncomingIntruder(int port)
{
	memset(&sock, 0, sizeof sock);
	memset(&my_addr, 0, sizeof my_addr);
	memset(&intruder_addr, 0, sizeof intruder_addr);
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		socketDebug("ResolutionConnection::acceptIncomingIntruder - socket failed to open\n", false);
		return NULL;
	}
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(kTcpPort_);
	int bind_success = bind(sock, (struct sockaddr*)&my_addr, sizeof(my_addr));
	if (bind_success < 0) {
		char the_error[32];
		sprintf(the_error, "ResolutionConnection::acceptIncomingIntruder - failed to bind: %d\n", GetLastError());
		XPLMDebugString(the_error);
		return NULL;
	}
	SOCKET accept_socket;
	XPLMDebugString("ResolutionConnection::acceptIncomingIntruder - waiting for connect\n");
	socklen_t addr_len = sizeof(intruder_addr);
	listen(sock, 24);
	XPLMDebugString("...\n");
	accept_socket = accept(sock, (struct sockaddr *)&intruder_addr, &addr_len);
	XPLMDebugString("...\n");
	if (accept_socket == INVALID_SOCKET) {
		socketCloseWithError("ResolutionConnection::acceptIncomingIntruder - to accept error: %d\n", sock);
		return NULL;
	}
	return accept_socket;
}

DWORD ResolutionConnection::senseReceiver()
{
	char* ack = "ACK";
	XPLMDebugString("ResolutionConnection::senseReceiver - accept\n");
	SOCKET accept_socket = acceptIncomingIntruder(kTcpPort_);
	XPLMDebugString("ResolutionConnection::senseReceiver - accepted\n");
	if (accept_socket) {
		connected = true;
		open_socket = accept_socket;
		resolveSense();
	}

	return 0;
}

int ResolutionConnection::connectToIntruder(std::string ip, int port)
{
	XPLMDebugString("ResolutionConnection::connectToIntruder - attempting to establish a connection\n");
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof dest);
	dest.sin_family = AF_INET;
	dest.sin_port = htons(kTcpPort_);
	dest.sin_addr.s_addr = inet_addr(ip.c_str());

	memset(&sock, 0, sizeof sock);
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connect(sock, (SOCKADDR*)&dest, sizeof(dest)) == SOCKET_ERROR) {
		socketCloseWithError("ResolutionConnection::connectToIntruder - unable to establish tcp connection - error: %d\n", sock);
		return -1;
	}
	return 0;
}

DWORD ResolutionConnection::senseSender()
{
	if (connectToIntruder(ip, port) >= 0) {
		return 0;
	}
	else {
		connected = true;
		open_socket = sock;
		resolveSense();
	}
	return 0;
}

void ResolutionConnection::resolveSense()
{
	char msg[256];
	char* ack = "ACK";
	Sense agreed_upon_sense = Sense::UNKNOWN;

	while (running)
	{
		if (recv(open_socket, msg, 255, 0) < 0) {
			socketCloseWithError("ResolutionConnection::resolveSense - Failed to receive: %d\n", open_socket);
		} else {
			XPLMDebugString("ResolutionConnection::resolveSense - received message: ");
			XPLMDebugString(msg);
			XPLMDebugString("\n");

			if (strcmp(msg, ack) == 0) {
				// if sense is unknown, then our Decider hasn't attempted to send yet
				if (current_sense == Sense::UNKNOWN) {
					consensusAchieved = true;
					current_sense = agreed_upon_sense;
					XPLMDebugString("ResolutionConnection::resolveSense - achieved consensus - closing connection\n");
					closesocket(open_socket);
					running = false;
				} else {
					if (strcmp(my_mac.c_str(), intruder_mac.c_str()) > 0) {
						sendSense();
						if (recv(open_socket, msg, 255, 0) < 0) {
							socketCloseWithError("ResolutionConnection::resolveSense - Failed to receive: %d\n", open_socket);
						} else {
							if (strcmp(msg, ack) == 0) {
								consensusAchieved = true;
								agreed_upon_sense = current_sense;
								XPLMDebugString("ResolutionConnection::resolveSense - achieved consensus - closing connection\n");
								closesocket(open_socket);
								running = false;
								break;
							} else {
								socketCloseWithError("ResolutionConnection::resolveSense - Failed to receive ack in race condition: %d\n", open_socket);
							}
						}
					} else {
						consensusAchieved = true;
						agreed_upon_sense = stringToSense(msg);
						if (send(open_socket, ack, strlen(ack) + 1, 0) == SOCKET_ERROR) {
							socketCloseWithError("ResolutionConnection::resolveSense - ack failed\n", open_socket);
						}
						XPLMDebugString("ResolutionConnection::resolveSense - achieved consensus - closing connection\n");
						closesocket(open_socket);
						running = false;
					}
				}
			} else {
				current_sense = stringToSense(msg);
				if (send(open_socket, ack, strlen(ack) + 1, 0) == SOCKET_ERROR) {
					socketCloseWithError("ResolutionConnection::resolveSense - ack failed\n", open_socket);
				}
			}
		}
	}
	thread_stopped = true;
}

int ResolutionConnection::sendSense()
{
	if (connected) {
		char* msg = senseToString(current_sense);
		if (send(open_socket, msg, strlen(msg) + 1, 0) == SOCKET_ERROR) {
			socketCloseWithError("ResolutionConnection::resolveSense - ack failed: %d\n", open_socket);
			return -1;
		}
		else {
			return 0; // success
		}
	} else {
		XPLMDebugString("ResolutionConnection::sendSense - attempting to send on an unconnected socket\n");
		return -2;
	}
}

void ResolutionConnection::socketDebug(char* err_msg, bool closeSocket)
{
	running = false;

	char debug_buf[128];
	snprintf(debug_buf, 128, "ResolutionConnection::socketDebug - %s\n", err_msg);
	XPLMDebugString(debug_buf);
	if (closeSocket)
		closesocket(sock);
}

void ResolutionConnection::socketCloseWithError(char* err_msg, SOCKET open_sock)
{
	running = false;

	char debug_buf[128];
	snprintf(debug_buf, 128, err_msg, GetLastError());
	XPLMDebugString(debug_buf);
	closesocket(open_socket);
}