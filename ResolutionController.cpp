#include "ResolutionController.h"

ResolutionController::ResolutionController(std::string mac_addr)
{
	mac = mac_addr;
	controllerSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (controllerSocket < 0) {
		XPLMDebugString("failed to open socket for resolution connections");
	}
	else {
		controller_addr.sin_family = AF_INET;
		controller_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		controller_addr.sin_port = htons(CONTROLLER_PORT);
		int bind_success = bind(controllerSocket, (struct sockaddr *)&controller_addr, sizeof sockaddr_in);
		if (bind_success < 0) {
			char the_error[32];
			sprintf(the_error, "ResolutionConnection::Failed to bind: %d\n", GetLastError());
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
			sprintf(the_error, "Controller: recvfrom error: %d\n", GetLastError());
			XPLMDebugString(the_error);
			break;
		}
		ResolutionConnection connection = { mac_addr };
		connection.openNewConnection(TCP_PORT);

		char* replyPort = "21218";
		error = sendto(controllerSocket, replyPort, strlen(replyPort), 0, (struct sockaddr*)&controller_addr, sizeof controller_addr);
		if (error < 0) {
			char the_error[256];
			sprintf(the_error, "Controller: sendto error: %d\n", GetLastError());
			XPLMDebugString(the_error);
			closesocket(controllerSocket);
			break;
		}
		controller_addr.sin_port = htons(CONTROLLER_PORT);
	}
	return 0;
}