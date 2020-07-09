#ifndef WINVER
// Define windows xp version
#define WINVER 0x0501
#endif

// This include statement must be before IPHLpapi include
#include "component/Decider.h"
#include "component/XBee.h"

#pragma comment(lib, "IPHLPAPI.lib")
#include <iphlpapi.h>


#include <vector>
#include <sys/types.h>
#include <stdio.h>
#include <sstream>
#include <unordered_map>
#include <ctime>
#include "data\Location.h"
#include <queue>
#include <mutex>
#include <condition_variable>

#include "component/VSpeedIndicatorGaugeRenderer.h"
#include "data/Aircraft.h"

#define BROADCAST_PORT 21221
#define MAC_LENGTH 18
#define PORT_LENGTH 6
#define MAX_RECEIVE_BUFFER_SIZE 4096
#define MAX_BRIDGE_QUEUE_SIZE  1024


class Transponder
{
public:
	/* Calls WSAStartup, which is required to be succesfully run before any networking calls can be made.*/
	static void initNetworking();
	static std::string getHardwareAddress();

	Transponder(Aircraft*, concurrency::concurrent_unordered_map<std::string, Aircraft*>*, concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>*, Decider*);
	~Transponder();
	DWORD receiveLocation(), sendLocation(), keepalive();
	void start();

	/*Calls XBee::InitializeComPort which must be called before using the COM Port*/
	void initXBee(unsigned int portnum);

	concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* openConnections;
	bool Transponder::isXBeeRoutingEnabled();
	bool enableXBeeRouting;

protected:
	std::string ip;
	

	SOCKET outSocket;
	SOCKET inSocket;
	struct sockaddr_in incoming;
	struct sockaddr_in outgoing;

	unsigned sinlen;
	std::atomic<int> communication;
	xplane::Location intruderLocation, myLocation;


	concurrency::concurrent_unordered_map<std::string, Aircraft*>* intrudersMap;




private:
	static std::atomic<bool> initialized_;
	static std::string macAddress_;

	Decider* decider_;
	Aircraft* aircraft_;

	// added for XBee support
	XBee* xb;
	HANDLE xbComm;
	

	std::vector<Aircraft*> allocatedAircraft_;
	concurrency::concurrent_unordered_map<std::string, int> keepAliveMap_;

	std::string getIpAddr();
	void createSocket(SOCKET*, struct sockaddr_in*, int, int);

	DWORD Transponder::processIntruder(std::string intruderID);

	std::mutex mQueueXB;
	std::condition_variable condXB;
	std::queue<std::string> queueXB;

	std::mutex mQueueUDP;
	std::condition_variable condUDP;
	std::queue<std::string> queueUDP;

	std::string qPayload;


};