#ifndef WINVER
#define WINVER 0x0501
#endif

#include "Decider.h"

#pragma comment(lib, "IPHLPAPI.lib")
#include <iphlpapi.h>

#include <vector>
#include <sys/types.h>
#include <stdio.h>
#include <sstream>
#include <unordered_map>
#include "location.pb.h"
#include "Aircraft.h"
#include "GaugeRenderer.h"

#define BROADCAST_PORT 21221
#define MAC_LENGTH 18
#define PORT_LENGTH 6

class Transponder
{
public:
	Transponder(Aircraft*, concurrency::concurrent_unordered_map<std::string, Aircraft*>*, concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>*, Decider*);
	~Transponder();
	DWORD receiveLocation(), sendLocation(), keepalive();
	void start();

	static void initNetworking();
	static std::string getHardwareAddress();

protected:
	std::string ip, mac;
	SOCKET outSocket, inSocket;
	struct sockaddr_in incoming, outgoing;
	unsigned sinlen;
	std::atomic<int> communication;
	xplane::Location intruderLocation, myLocation;
	concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* open_connections;
	concurrency::concurrent_unordered_map<std::string, Aircraft*>* intrudersMap;

private:
	Decider * decider_;
	Aircraft* aircraft;
	std::vector<Aircraft*> allocated_aircraft;
	concurrency::concurrent_unordered_map<std::string, int> keepAliveMap;
	
	std::string getIpAddr();
	void createSocket(SOCKET*, struct sockaddr_in*, int, int);

	static std::atomic<bool> initialized;
	static std::string mac_address;
};