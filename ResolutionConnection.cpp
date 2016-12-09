#include "ResolutionConnection.h"

ResolutionConnection::ResolutionConnection(std::string mac_addr)
{
	mac = mac_addr;
	isSender = 0;
}

ResolutionConnection::~ResolutionConnection()
{
	//int result;
	//result = shutdown(listenSocket, SD_SEND);
	//if (result == SOCKET_ERROR) {
	//	XPLMDebugString("failed to shutdown\n");
	//	closesocket(s);
	//	return;
	//}
	XPLMDebugString("ResolutionConnection::~ResolutionConnection called.\n");
	closesocket(listenSocket);
	closesocket(sendSocket);
	XPLMDebugString("ResolutionConnection::~ResolutionConnection - sockets closed.\n");
}


static DWORD WINAPI startListeningForSense(void* param)
{
	ResolutionConnection* rc = (ResolutionConnection*)param;
	return rc->senseListener();
}

void ResolutionConnection::openNewConnection(int port)
{
	memset(&listenSocket, 0, sizeof listenSocket);
	memset(&my_addr, 0, sizeof my_addr);
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		XPLMDebugString("ResolutionConnection::Socket failed to open\n");
	}
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(21218);
	int bind_success = bind(listenSocket, (struct sockaddr*)&my_addr, sizeof(my_addr));
	if (bind_success < 0) {
		char the_error[32];
		sprintf(the_error, "ResolutionConnection::Failed to bind: %d\n", GetLastError());
		XPLMDebugString(the_error);
	}
	DWORD ThreadID;
	CreateThread(NULL, 0, startListeningForSense, (void*) this, 0, &ThreadID);
}

DWORD ResolutionConnection::senseListener()
{
	char msg[256];
	listen(listenSocket, 3);
	int addr_len = sizeof(intruder_addr);
	SOCKET accept_socket = accept(listenSocket, (struct sockaddr *)&intruder_addr, &addr_len);
	if (accept_socket == INVALID_SOCKET) {
		char the_error[32];
		sprintf(the_error, "ResolutionConnection::Failed to accept: %d\n", GetLastError());
		XPLMDebugString(the_error);
		closesocket(listenSocket);
		return 0;
	}
	for (;;)
	{
		int err = recv(accept_socket, msg, 255, 0);
		if (err < 0) {
			char the_error[32];
			sprintf(the_error, "ResolutionConnection::Failed to receive: %d\n", GetLastError());
			XPLMDebugString(the_error);
			closesocket(listenSocket);
			return 0;
		}
		XPLMDebugString(msg);
		XPLMDebugString("\n");
		if (strcmp(msg, "0") == 0) {
			XPLMDebugString("closing connection\n");
			closesocket(listenSocket);
			break;
		}
		else {
			Sleep(1000);
		}
	}
	return 0;
}

int ResolutionConnection::contactIntruder(std::string ip)
{
	XPLMDebugString("attempting to connect to ");
	XPLMDebugString(ip.c_str());

	int error, port;
	char replyPort[32];
	SOCKET sock;
	struct sockaddr_in local, dest;

	memset(&sock, 0, sizeof sock);
	memset(&local, 0, sizeof local);
	memset(&dest, 0, sizeof dest);

	local.sin_family = AF_INET;
	local.sin_port = htons(21299);
	local.sin_addr.s_addr = INADDR_ANY;

	dest.sin_family = AF_INET;
	dest.sin_port = htons(CONTROLLER_PORT);
	dest.sin_addr.s_addr = inet_addr(ip.c_str());

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	error = bind(sock, (sockaddr *)&local, sizeof(struct sockaddr));
	if (error < 0) {
		char the_error[256];
		sprintf(the_error, "RC::bind error: %d\n", GetLastError());
		XPLMDebugString(the_error);
		closesocket(sock);
		return -1;
	}

	error = sendto(sock, mac.c_str(), MAC_LENGTH, 0, (struct sockaddr *) &dest, sizeof(struct sockaddr));
	if (error < 0) {
		char the_error[256];
		sprintf(the_error, "RC::sendto error: %d\n", GetLastError());
		XPLMDebugString(the_error);
		closesocket(sock);
		return -1;
	}

	int locallen = sizeof(local);
	error = recvfrom(sock, replyPort, MAC_LENGTH, 0, (struct sockaddr *)&local, (int*)&locallen);
	if (error == SOCKET_ERROR) {
		char the_error[256];
		sprintf(the_error, "RC:: recvfrom error: %d\n", GetLastError());
		XPLMDebugString(the_error);
		return -1;
	}

	closesocket(sock);
	
	port = atoi(replyPort);
	char debug_info[128];
	sprintf(debug_info, "recv port#%d\n", port);
	XPLMDebugString(debug_info);
	return port;
}

int ResolutionConnection::establishConnection(std::string ip, int port)
{
	XPLMDebugString("attempting to establish a connection");
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof dest);
	dest.sin_family = AF_INET;
	dest.sin_port = htons(21218);
	dest.sin_addr.s_addr = inet_addr(ip.c_str());

	memset(&sendSocket, 0, sizeof sendSocket);
	sendSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	int result;
	result = connect(sendSocket, (SOCKADDR*)&dest, sizeof(dest));
	if (result == SOCKET_ERROR) {
		XPLMDebugString("unable to establish tcp connection\n");
		closesocket(sendSocket);
		return GetLastError() * -1;
	}
	isSender = 1;
	return 0;
}

void ResolutionConnection::sendSense(Sense sense)
{
	if (isSender) {
		int result;
		char* sens = "GO_DOWN";
		result = send(sendSocket, sens, strlen(sens), 0);
		if (result == SOCKET_ERROR) {
			XPLMDebugString("SendSense::send failed\n");
			closesocket(sendSocket);
			return;
		}
	} else {
		XPLMDebugString("ResolutionConnection::Trying to send sense without establishing connection (or you're the receiver)\n");
	}
}