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
	std::atomic<bool> consensusAchieved;
	std::atomic<Sense> current_sense;
	
	int connectToIntruder(std::string, int);
	SOCKET acceptIncomingIntruder(int);

	DWORD senseSender();
	DWORD senseReceiver();
	int sendSense();
private:
	static unsigned short const kTcpPort_ = 21218;

	std::string my_mac;
	int port;

	std::atomic<bool> running;
	std::atomic<bool> thread_stopped;
	std::atomic<bool> connected;

	SOCKET sock, open_socket;
	struct sockaddr_in my_addr;
	struct sockaddr_in intruder_addr;

	void resolveSense();
	void socketDebug(char*, bool);
	void socketCloseWithError(char*, SOCKET);
};

static inline char* const senseToString(Sense s)
{
	switch (s)
	{
		case Sense::UPWARD: return "UPWARD";
		case Sense::DOWNWARD: return "DOWNWARD";
		case Sense::MAINTAIN: return "MAINTAIN";
		default: return "UNKNOWN";
	}
}

static inline Sense const stringToSense(char* str)
{
	if (strcmp(str, "UPWARD") == 0) {
		return Sense::UPWARD;
	} else if (strcmp(str, "DOWNWARD") == 0) {
		return Sense::DOWNWARD;
	} else if (strcmp(str, "MAINTAIN") == 0) {
		return Sense::MAINTAIN;
	} else {
		return Sense::UNKNOWN;
	}
}