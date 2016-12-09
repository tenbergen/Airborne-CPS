#include "ResolutionController.h"

ResolutionController::ResolutionController(std::string mac_addr, concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* map)
{
	mac = mac_addr;
	active_connections = map;
	controllerSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (controllerSocket < 0) {
		XPLMDebugString("ResolutionController::ResolutionController - failed to open socket for resolution connections");
	}
	else {
		controller_addr.sin_family = AF_INET;
		controller_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		controller_addr.sin_port = htons(CONTROLLER_PORT);
		int bind_success = bind(controllerSocket, (struct sockaddr *)&controller_addr, sizeof sockaddr_in);
		if (bind_success < 0) {
			char the_error[32];
			sprintf(the_error, "ResolutionController::ResolutionController - failed to bind: %d\n", GetLastError());
			XPLMDebugString(the_error);
		}
	}
}


ResolutionController::~ResolutionController()
{
	communication = 0;
}

static DWORD WINAPI startController(void* param)
{
	ResolutionController* controller = (ResolutionController*)param;
	return controller->listenForRequests();
}

void ResolutionController::start()
{
	communication = 1;
	DWORD ThreadID;
	CreateThread(NULL, 0, startController, (void*)this, 0, &ThreadID);
}

DWORD ResolutionController::listenForRequests()
{
	int controllerlen = sizeof(controller_addr);

	char mac_addr[MAC_LENGTH];
	int error;
	while (communication)
	{
		error = recvfrom(controllerSocket, mac_addr, MAC_LENGTH, 0, (struct sockaddr *)&controller_addr, (int*)&controllerlen);
		if (error == SOCKET_ERROR) {
			char the_error[256];
			sprintf(the_error, "ResolutionController::listenForRequests - recvfrom error: %d\n", GetLastError());
			XPLMDebugString(the_error);
			break;
		}
		XPLMDebugString("ResolutionController::listenForRequests - recieved connection from ");
		XPLMDebugString(mac_addr);
		XPLMDebugString("\n");

		ResolutionConnection* existing_connection = (*active_connections)[mac_addr];
		if (existing_connection) {
			if (strcmp(mac.c_str(), mac_addr) > 0) {
				XPLMDebugString("ResolutionController::listenForRequests - opening existing connection\n");
				existing_connection->openNewConnectionSender(existing_connection->ip, TCP_PORT);
			} else {
				XPLMDebugString("ResolutionController::listenForRequests - continue\n");
				continue;
			}
		} else {
			XPLMDebugString("ResolutionController::listenForRequests - spawn new TCP Thread\n");
			ResolutionConnection* connection = new ResolutionConnection(mac_addr);
			(*active_connections)[mac_addr] = connection;
			connection->openNewConnectionReceiver(TCP_PORT);
		}

		char* replyPort = "21217\0";
		error = sendto(controllerSocket, replyPort, strlen(replyPort), 0, (struct sockaddr*)&controller_addr, sizeof controller_addr);
		if (error < 0) {
			char the_error[256];
			sprintf(the_error, "ResolutionController::listenForRequests - sendto error: %d\n", GetLastError());
			XPLMDebugString(the_error);
			closesocket(controllerSocket);
			break;
		}
		controller_addr.sin_port = htons(CONTROLLER_PORT);
	}
	return 0;
}