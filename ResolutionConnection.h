#pragma once

#ifndef _WINDOWS_
#include <ws2tcpip.h>
#include <windows.h>
#endif

#include <string>
#include <atomic>
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
	void start();
	void openNewConnection(int);
	int connectToIntruder(std::string);
	DWORD senseListener(), resolutionController, listenForRequests();
	int establishConnection(std::string,int);
protected:
	std::atomic<int> communication;
	int isConnected;
private:
	std::string mac;
	SOCKET listenSocket, sendSocket, controllerSocket;
	struct sockaddr_in my_addr, intruder_addr, controller_addr;
};

