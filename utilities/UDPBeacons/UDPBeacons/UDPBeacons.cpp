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
    int innerDelay = 10;
    int outerDelay = 1000;
    std::string secondArg;
    std::string thirdArg;

    if (argc < 2) {
        std::cout << "Enter the path/filename to the beacon file: ";
        std::cin >> fileName;
        std::cout << "Parsing File: " + fileName << std::endl;
    }
    else if (argc == 2 || argc == 3 || argc == 4) {
        fileName = argv[1];
        std::cout << "Parsing File: " + fileName << std::endl;
        if (argc == 3 || argc == 4) {
            secondArg = argv[2];
            if (secondArg == "slow") {
                innerDelay = 100;
                outerDelay = 10;
            }
            else {
                std::cout << "Unknown argument. Please use the syntax:\nUDPBeacons.exe <filename> slow\n if you wish to invoke slow mode" << std::endl;
                return 0;
            }
            if (argc == 4) {
                thirdArg = argv[3];
                if (thirdArg == "slow") {
                    innerDelay = 500;
                    outerDelay = 10;
                }
                else {
                    std::cout << "Unknown argument. Please use the syntax:\nUDPBeacons.exe <filename> slow slow\n if you wish to invoke slow slow mode" << std::endl;
                    return 0;
                }
            }

        } 
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
            if (GetAsyncKeyState(VK_ESCAPE)) {
                exit = true;
                break;
            }
            Sleep(innerDelay);
        }

        Sleep(outerDelay);
    }
    std::cout << "Stopped UDP Broadcaset." << std::endl;
    closesocket(outSocket);
    WSACleanup();
}


