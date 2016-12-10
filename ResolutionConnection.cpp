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

ResolutionConnection::ResolutionConnection(std::string my_mac, std::string intruder_mac, std::string ip_addr, int port_num)
{
	mac = intruder_mac;
	ip = ip_addr;
	port = port_num;
	running = true;
	
	LPTHREAD_START_ROUTINE task;
	if (strcmp(my_mac.c_str(), intruder_mac.c_str()) > 0) {
		task = startResolutionSender;
	} else {
		task = startResolutionReceiver;
	}
	XPLMDebugString("task set\n");
	DWORD ThreadID;
	CreateThread(NULL, 0, task, (void*) this, 0, &ThreadID);
	XPLMDebugString("thread started\n");
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
		socketDebug("ResolutionConnection::openNewConnectionReceiver - socket failed to open\n", false);
		return NULL;
	}
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(TCP_PORT);
	int bind_success = bind(sock, (struct sockaddr*)&my_addr, sizeof(my_addr));
	if (bind_success < 0) {
		char the_error[32];
		sprintf(the_error, "ResolutionConnection::openNewConnectionReceiver - failed to bind: %d\n", GetLastError());
		XPLMDebugString(the_error);
		return NULL;
	}
	SOCKET accept_socket;
	bool waiting = true;
	//ULONG non_blocking = 1;
	//if (ioctlsocket(sock, FIONBIO, &non_blocking) == SOCKET_ERROR) {
	//	socketCloseWithError("ioctlsocket failed on sock with error: %d\n", GetLastError());
	//	return NULL;
	//}
	//FD_SET read_set;
	XPLMDebugString("waiting for connect\n");
	while (waiting) {
		//FD_ZERO(&read_set);
		//FD_SET(sock, &read_set);
		//timeval timeout;
		//timeout.tv_sec = 0;  // Zero timeout (poll)
		//timeout.tv_usec = 0;
		//if (select(0, &read_set, NULL, NULL, &timeout) == SOCKET_ERROR) {
		//	socketCloseWithError("select returned with error: %d\n", GetLastError());
		//	return NULL;
		//}
		//if (FD_ISSET(sock, &read_set)) {
			XPLMDebugString("FD_ISSET true\n");
			socklen_t addr_len = sizeof(intruder_addr);
			listen(sock, 1);
			XPLMDebugString("...\n");
			accept_socket = accept(sock, (struct sockaddr *)&intruder_addr, &addr_len);
			XPLMDebugString("...\n");
			if (accept_socket == INVALID_SOCKET) {
				socketCloseWithError("ResolutionConnection::senseReceiver - to accept error: %d\n", GetLastError());
				return NULL;
			}
			//if (ioctlsocket(accept_socket, FIONBIO, &non_blocking) == SOCKET_ERROR) {
			//	socketCloseWithError("ioctlsocket failed on accept_socket with error: %d\n", GetLastError());
			//	return NULL;
			//}
			waiting = false;
		//}
	}
	//SOCKET* accepted_sock = (SOCKET*)malloc(sizeof(SOCKET));
	//accepted_sock = &accept_socket;
	//return accepted_sock;
	return accept_socket;
}

DWORD ResolutionConnection::senseReceiver()
{
	XPLMDebugString("accept\n");
	char msg[256];
	SOCKET accept_socket = acceptIncomingIntruder(TCP_PORT);
	XPLMDebugString("accepted\n");
	if (accept_socket == NULL) {
		return 0;
	}
	while (running)
	{
		char* sens = "GO_UP_RECEIVER";
		if (recv(accept_socket, msg, 255, 0) < 0) {
			socketCloseWithError("ResolutionConnection::senseReceiver -  to receive error: %d\n", GetLastError());
			return 0;
		}
		XPLMDebugString("ResolutionConnection::senseReceiver - received message: ");
		XPLMDebugString(msg);
		XPLMDebugString("\n");
		if (strcmp(msg, "0") == 0) {
			socketDebug("ResolutionConnection::senseReceiver - received 0 - closing connection\n", true);
			break;
		}
		if (send(sock, sens, strlen(sens), 0) == SOCKET_ERROR) {
			socketDebug("ResolutionConnection::senseReceiver - send failed\n", true);
			return 0;
		}
	}
	//free(accept_socket);
	thread_stopped = true;
	return 0;
}

int ResolutionConnection::connectToIntruder(std::string ip, int port)
{
	XPLMDebugString("ResolutionConnection::establishConnection - attempting to establish a connection\n");
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof dest);
	dest.sin_family = AF_INET;
	dest.sin_port = htons(21217);
	dest.sin_addr.s_addr = inet_addr(ip.c_str());

	memset(&sock, 0, sizeof sock);
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connect(sock, (SOCKADDR*)&dest, sizeof(dest)) == SOCKET_ERROR) {
		socketCloseWithError("ResolutionConnection::establishConnection - unable to establish tcp connection - error: %d\n", GetLastError());
		return -1;
	}
	return 0;
}

DWORD ResolutionConnection::senseSender()
{
	char msg[256];
	if (connectToIntruder(ip, port) < 0) {
		return 0;
	}
	while (running)
	{
		char* sens = "GO_UP";
		if (send(sock, sens, strlen(sens), 0) == SOCKET_ERROR) {
			socketDebug("ResolutionConnection::senseSender - send failed\n", true);
			return 0;
		}
		if (recv(sock, msg, 255, 0) < 0) {
			socketCloseWithError("ResolutionConnection::senseSender - Failed to receive: %d\n", GetLastError());
			return 0;
		}
		XPLMDebugString("ResolutionConnection::senseSender - received message: ");
		XPLMDebugString(msg);
		XPLMDebugString("\n");
		if (strcmp(msg, "0") == 0) {
			XPLMDebugString("ResolutionConnection::senseSender - closing connection\n");
			closesocket(sock);
			break;
		}
	}
	thread_stopped = true;
	return 0;
}

void ResolutionConnection::socketDebug(char* err_msg, bool closeSocket)
{
	char debug_buf[128];
	snprintf(debug_buf, 128, err_msg);
	XPLMDebugString(debug_buf);
	if (closeSocket)
		closesocket(sock);
}

void ResolutionConnection::socketCloseWithError(char* err_msg, int error_code)
{
	char debug_buf[128];
	snprintf(debug_buf, 128, err_msg, error_code);
	XPLMDebugString(debug_buf);
	closesocket(sock);
}