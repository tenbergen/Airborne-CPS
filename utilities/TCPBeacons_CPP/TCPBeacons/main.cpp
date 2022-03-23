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
#include <string>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>       

#pragma comment (lib, "Ws2_32.lib")


#define MAC_LENGTH 18
#define PORT_LENGTH 6
#define MAX_RECEIVE_BUFFER_SIZE 4096
// #define BROADCAST_PORT 21221
#define LISTENING_PORT 1901

int __cdecl main(int argc, char* argv[])
{
    // >>>>>>> FILE HANDLER <<<<<<<

     // init attributes
    std::string fileName, mode1, mode2;
    int innerDelay = 10;
    int outerDelay = 1000;

    // >>> FILE HANDLER <<<
    if (argc < 2) // if user doesn't specify filename => prompt them
    {
        std::cout << "Enter the path/filename to the beacon file: ";
        std::cin >> fileName;
        std::cout << "Parsing File: " + fileName << std::endl;
    }
    else if (argc == 2 || argc == 3 || argc == 4) {
        fileName = argv[1];
        std::cout << "Parsing File: " + fileName << std::endl;
        if (argc == 3) {
            mode1 = argv[2];
            if (mode1 == "slow")
            {
                innerDelay = 100;
                outerDelay = 10;
            }
            else
            {
                std::cout << "Unknown argument!! Please use the syntax : \nTCPBeaconsServer <filename> slow\nif you wish to invoke slow mode" << std::endl;
                return 0;
            }

        }
        else if (argc == 4) {
            mode2 = argv[3];
            if (mode2 == "slow")
            {
                innerDelay = 500;
                outerDelay = 10;
            }
            else
            {
                std::cout << "Unknown argument!! Please use the syntax : \nTCPBeaconsServer <filename> slow\nif you wish to invoke slow slow mode" << std::endl;
                return 0;
            }
        }

    }

    //// After the file is inputed, parse the file => push it to a vector
    std::ifstream infile(fileName);
    std::string line;
    std::vector<std::string> beacons;
    std::getline(infile, line);


    while (std::getline(infile, line))
    {
        beacons.push_back(line); // push to beacons vector
    }


    // >>>>>>> TCP/IP HANDLER <<<<<<<

    std::string ipAddress = "127.0.0.1"; // ip of the server

    // init winSock
    WSAData data;
    WORD ver = MAKEWORD(2, 2);

    int wsResult = WSAStartup(ver, &data);

    if (wsResult != 0)
    {
        std::cerr << "Can't start Winsock, Err #" << wsResult << std::endl;
        return -1;
    }


    // Create socket
    SOCKET sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == INVALID_SOCKET)
    {
        std::cerr << "Can't create socket, Err #" << WSAGetLastError;
        WSACleanup();
        return -1;
    }

    // Fill in a outgoing structure - this structure tells winSock that what server and what port we want to connect to
    sockaddr_in outgoing;
    outgoing.sin_family = AF_INET;
    outgoing.sin_port = htons(LISTENING_PORT);
    inet_pton(AF_INET, ipAddress.c_str(), &outgoing.sin_addr); // change this to any


    // Connect to server
    int connRes = connect(sock_, (sockaddr*)&outgoing, sizeof(outgoing));
    if (connRes == SOCKET_ERROR)
    {
        std::cerr << "Can't connect to server, Err #" << WSAGetLastError() << std::endl;
        closesocket(sock_);
        WSACleanup();
        return -1;

    }

    // Send Beacons to TCP server then echo the beacons from server 

    bool exit = false;
    char buf[4096];

    while (exit == false)
    {
        for (std::size_t i = 0; i < beacons.size(); i++)
        {
            int sendRes = sendto(sock_, beacons[i].c_str(), beacons[i].length() + 1, 0, (struct sockaddr*)&outgoing, sizeof(struct sockaddr_in));
            if (sendRes != SOCKET_ERROR)
            {
                //std::cout << beacons[i] << std::endl;
                // Wait for response from server
                ZeroMemory(buf, 4096);
                int byteRecv = recv(sock_, buf, 4096, 0);
                if (byteRecv > 0)
                {
                    // Echo response to console
                    std::cout << std::string(buf, 0, byteRecv) << std::endl;
                }

                // exit when user hit esc
                if (GetAsyncKeyState(VK_ESCAPE)) {
                    exit = true;
                    break;
                }
                Sleep(innerDelay);
            }
        }

        Sleep(outerDelay);

    }

    std::cout << "Stopping TCP/IP Connection to the Server." << std::endl;
    closesocket(sock_);
    WSACleanup();
}