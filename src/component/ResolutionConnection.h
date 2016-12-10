#pragma once

#ifndef _WINDOWS_
#include <ws2tcpip.h>
#include <windows.h>
#endif

#include <atomic>
#include <string>
#include "XPLMUtilities.h"

#include "data/Sense.h"

#define MAC_LENGTH 18
#define TCP_PORT 21218

class ResolutionConnection
{
public:
	ResolutionConnection(std::string my_mac, std::string intruder_mac, std::string ip, int port);
	~ResolutionConnection();

	std::string intruder_mac;
	std::string ip;
	
	int connectToIntruder(std::string, int);
	SOCKET acceptIncomingIntruder(int);

	DWORD senseSender();
	DWORD senseReceiver();
private:
	static unsigned short const kTcpPort_ = 21217;

	std::string mac;
	int port;

	std::atomic<bool> running;
	std::atomic<bool> thread_stopped;

	SOCKET sock;
	struct sockaddr_in my_addr;
	struct sockaddr_in intruder_addr;

	void socketDebug(char*, bool);
	void socketCloseWithError(char*, int);
};