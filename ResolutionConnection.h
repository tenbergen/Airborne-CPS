#pragma once

#ifndef _WINDOWS_
#include <ws2tcpip.h>
#include <windows.h>
#endif

#include <string>
#include "XPLMUtilities.h"
#include "Sense.h"
#include <atomic>

#define CONTROLLER_PORT 21219
#define MAC_LENGTH 18
#define TCP_PORT 21218

class ResolutionConnection
{
public:
	std::string ip;
	void sendSense(Sense);
	ResolutionConnection(std::string);
	ResolutionConnection(std::string, int);
	~ResolutionConnection();
	void openNewConnectionReceiver(int);
	void openNewConnectionSender(std::string, int);
	int contactIntruder(std::string);
	DWORD senseReceiver();
	DWORD senseSender();
	int establishConnection(std::string,int);
private:
	std::string mac;
	int port;
	std::atomic<bool> running;
	std::atomic<bool> thread_stopped;

	SOCKET listenSocket;
	SOCKET sendSocket;

	struct sockaddr_in my_addr;
	struct sockaddr_in intruder_addr;
};

