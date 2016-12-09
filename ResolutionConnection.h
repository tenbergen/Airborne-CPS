#pragma once

#ifndef _WINDOWS_
#include <ws2tcpip.h>
#include <windows.h>
#endif

#include <string>
#include "XPLMUtilities.h"
#include "Sense.h"

#define CONTROLLER_PORT 21219
#define MAC_LENGTH 18
#define TCP_PORT 21218

class ResolutionConnection
{
public:
	void sendSense(Sense);
	ResolutionConnection(std::string);
	~ResolutionConnection();
	void openNewConnection(int);
	int connectToIntruder(std::string);
	DWORD senseListener();
	int establishConnection(std::string,int);
protected:
	int isConnected;
private:
	std::string mac;

	SOCKET listenSocket;
	SOCKET sendSocket;

	struct sockaddr_in my_addr;
	struct sockaddr_in intruder_addr;
};

