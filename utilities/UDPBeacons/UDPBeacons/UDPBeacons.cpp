//#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define MAC_LENGTH 18
#define PORT_LENGTH 6
#define MAX_RECEIVE_BUFFER_SIZE 4096
#define BROADCAST_PORT 21221

int __cdecl main(int argc, char* argv[])
{
    std::string fileName;
    if (argc < 2) {
        std::cout << "Enter the path/filename to the beacon file:" << std::endl;
        std::cin >> fileName;
        std::cout << "Parsing File: " + fileName << std::endl;
    }
    else {
        fileName = argv[1];
        std::cout << "Parsing File: " + fileName << std::endl;
    }

    std::ifstream infile(fileName);
    std::string line;
    std::vector<std::string> beacons;
    while (std::getline(infile, line))
    {

        beacons.push_back(line);
    }

    WSADATA wsaData;
    struct sockaddr_in outgoing;


    if (WSAStartup(0x0101, &wsaData) != 0) {
        exit(0);
    }
    SOCKET outSocket;
    outSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (outSocket < 0) {
        std::cout << "failed to open socket to broadcast location\n" << std::endl;
    }
    else {
        BOOL bOptVal = TRUE;
        setsockopt(outSocket, SOL_SOCKET, SO_BROADCAST, (char*)&bOptVal, sizeof(int));
        outgoing.sin_addr.s_addr = htonl(-1);
        outgoing.sin_port = htons(BROADCAST_PORT);
        outgoing.sin_family = PF_INET;
        bind(outSocket, (struct sockaddr*) & outgoing, sizeof(struct sockaddr_in));
    }
    std::string buffer = "n00:00:00:00:00:01n192.168.0.0n47.519961n10.698863n3050.078383";
    bool exit = false;
    std::cout << "Starting UDP Broadcaset. Press esc to exit! " << std::endl;
    while (exit == false) {


        for (int i = 0; i < beacons.size(); i++) {
            sendto(outSocket, (const char*)beacons[i].c_str(), beacons[i].length() + 1, 0, (struct sockaddr*) & outgoing, sizeof(struct sockaddr_in));
            std::cout << beacons[i] << std::endl;
            Sleep(10);
        }
        if (GetAsyncKeyState(VK_ESCAPE)) {
            exit = true;
        }
        Sleep(1000);
    }
    std::cout << "Stopped UDP Broadcaset." << std::endl;
    closesocket(outSocket);
    WSACleanup();
}


