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
	Transponder(Aircraft*, concurrency::concurrent_unordered_map<std::string, Aircraft*>*, Decider*);
	~Transponder();
	DWORD receiveLocation(), sendLocation(), keepalive();
	void start();

protected:
	std::string ip, mac;
	WSADATA w;
	SOCKET outSocket, inSocket;
	struct sockaddr_in incoming, outgoing;
	unsigned sinlen;
	std::atomic<int> communication;
	xplane::Location intruderLocation, myLocation;
	concurrency::concurrent_unordered_map<std::string, Aircraft*>* intrudersMap;

private:
	Decider * decider_;
	Aircraft* aircraft;
	std::vector<Aircraft*> allocated_aircraft;
	concurrency::concurrent_unordered_map<std::string, int> keepAliveMap;
	static std::string getHardwareAddress();
	std::string getIpAddr();
	void createSocket(SOCKET*, struct sockaddr_in*, int, int);
	int establishResolutionConnection(Aircraft& intruding_aircraft, char* port);
};