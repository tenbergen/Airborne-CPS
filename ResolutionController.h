#pragma once

#include "ResolutionConnection.h"
#include <concurrent_unordered_map.h>

class ResolutionController
{
public:
	ResolutionController(std::string, concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>*);
	~ResolutionController();
	void start();
	DWORD listenForRequests();
protected:
	std::atomic<int> communication;
private:
	std::string mac;
	SOCKET controllerSocket;
	struct sockaddr_in controller_addr;
	concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* active_connections;
};

