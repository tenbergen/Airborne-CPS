#pragma once

#ifndef _WINDOWS_
#include <ws2tcpip.h>
#include <windows.h>
#endif

#include <atomic>
#include <mutex>
#include <string>
#include "XPLMUtilities.h"

#include "data/Sense.h"

#define MAC_LENGTH 18

class ResolutionConnection
{
public:
	static unsigned short const kTcpPort_ = 21218;

	ResolutionConnection(std::string const user_mac, std::string const intruder_mac, std::string const ip, int const port);
	~ResolutionConnection();

	std::string const intruder_mac;
	std::string const ip;
	std::string const my_mac;
	int const port;

	bool consensusAchieved;
	Sense current_sense;
	std::mutex lock;
	std::chrono::milliseconds last_analyzed;
	
	int connectToIntruder(std::string, int);
	SOCKET acceptIncomingIntruder(int);

	DWORD senseSender();
	DWORD senseReceiver();
	int sendSense(Sense);
private:
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
		default: return "UNKNOWN";
	}
}

static inline Sense const stringToSense(char* str)
{
	if (strcmp(str, "UPWARD") == 0) {
		return Sense::UPWARD;
	} else if (strcmp(str, "DOWNWARD") == 0) {
		return Sense::DOWNWARD;
	} else {
		return Sense::UNKNOWN;
	}
}