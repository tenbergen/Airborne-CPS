#pragma once

#include "ResolutionConnection.h"
#include <atomic>

class ResolutionController
{
public:
	ResolutionController(std::string);
	~ResolutionController();
	void start();
	DWORD listenForRequests();
protected:
	std::atomic<int> communication;
private:
	std::string mac;
	SOCKET controllerSocket;
	struct sockaddr_in controller_addr;
};

