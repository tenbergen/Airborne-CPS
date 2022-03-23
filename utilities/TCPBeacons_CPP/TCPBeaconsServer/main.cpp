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
#include <thread>         
#include <chrono>       

#pragma comment (lib, "ws2_32.lib")
#define LISTENING_PORT 1901


void sendBeacons(SOCKET sock_, std::vector<std::string> beacons, int innerDelay, int outerDelay)
{
    bool exit = false;
    while (exit == false)
    {
        for (std::size_t i = 0; i < beacons.size(); i++)
        {
            if (sock_ != SOCKET_ERROR)
            {
                // send beacons to client
                send(sock_, beacons[i].c_str(), beacons[i].size() + 1, 0);

                // exit when user hit esc
                if (GetAsyncKeyState(VK_ESCAPE)) {
                    exit = true;
                    break;
                }
                Sleep(innerDelay);

            }
        }
        std::cout << "Stop sending beacons..." << std::endl;
        Sleep(300); 
    }
}

void receiveBeacons(SOCKET sock_, std::vector<std::string> beacons, int innerDelay, int outerDelay)
{
    bool exit = false;
    char buf[4096];

    while (exit == false)
    {
        for (std::size_t i = 0; i < beacons.size(); i++)
        {
            

                // echo beacons sent from client
                ZeroMemory(buf, 4096);
                int byteRecv = recv(sock_, buf, 4096, 0);
                if (byteRecv == 0)
                {
                    std::cout << "Client disconnected" << std::endl;
                    break;
                }
                // echo the message
                std::cout << std::string(buf, 0, byteRecv) << std::endl;
                // exit when user hit esc
                if (GetAsyncKeyState(VK_ESCAPE)) {
                    exit = true;
                    break;
                }
                Sleep(innerDelay);

        }
        std::cout << "" << std::endl;
        std::cout << "Client disconnected..." << std::endl;
        Sleep(300);
    }
}

int __cdecl main(int argc, char* argv[])
{
    // >>> ATTRIBUTES <<<
    std::string fileName, mode1, mode2;
    int innerDelay = 10, outerDelay = 1000, wsOk;
    std::vector<std::string> beacons;
    sockaddr_in hint;
    fd_set master; // select()::fd_set master is a set of 1 listening value and multiple client values 
    std::vector<std::thread> threads;



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
                std::cout << "Unknown argument!! Please use the syntax : \nTCPBeaconsServer <filename> slow\nif you wish to invoke slow mode" << std::endl;
                return 0;
            }
        }

    }


    // After the file is inputed, parse the file => push it to a vector
    std::ifstream infile(fileName);
    std::string line;
    std::getline(infile, line);

    while (std::getline(infile, line))
    {
        beacons.push_back(line); // push to beacons vector
    }


	// >>> TCP/IP SERVER <<<
	// Initialize winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	wsOk = WSAStartup(ver, &wsData);

	if (wsOk != 0) {
		std::cerr << "Can't Init winsock! Quitting" << std::endl;
		return -1;
	}

	// Create a listening socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0); // in UNIX, socket() returns an int, winsock returns a type SOCKET
	if (listening == INVALID_SOCKET)
	{
		std::cerr << "Can't create a socket! Quitting" << std::endl;
		return -1;
	}


	// Bind the ip address and port to a socket 
	hint.sin_family = AF_INET;
	hint.sin_port = htons(LISTENING_PORT); // network is big-endian and the computer is little-endian
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // bind to any address, Could also use inet_pton

	bind(listening, (sockaddr*)&hint, sizeof(hint)); // bind the socket to the ip and port to send and receive connection


	// Tell winsock the socket is for listening
	listen(listening, SOMAXCONN);


    // >>> MULTI-CLIENT <<<
    FD_ZERO(&master); // clear the master set
    FD_SET(listening, &master); // add listening to the master set, position 0 in the array

    std::cout << "The server is listening on port " << LISTENING_PORT << "..." << std::endl;

    // Create a running server to accept multiple clients
    while (true)
    {
        // make a copy of master set because the set can be destroyed after select() is called
        fd_set clonedMaster = master;

        /*
            Learn more about select: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select
            The select function determines the status of one or more sockets, waiting if necessary, to perform synchronous I/O.
        */
        int socketCount = select(0, &clonedMaster, nullptr, nullptr, nullptr);  // if theres any sockets, it will be stored in the &clonedMaster set

        for (int i = 0; i < socketCount; i++)
        {
            SOCKET sock_ = clonedMaster.fd_array[i]; // sock_ is a client value in position i of the array

            if (sock_ == listening)
            {
                // Accept a new connection
                sockaddr_in client;
                int clientSize = sizeof(client);
                //SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
                SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

                // Add the new connection to the list of connected clients (master set)
                FD_SET(clientSocket, &master);


                // Prompt a message to inform new connected client
                char host[NI_MAXHOST]; // Client's remote name
                char service[NI_MAXHOST]; // Service (i.e. port) the clinet is connect on
                ZeroMemory(host, NI_MAXHOST); // like memset in UNIX
                ZeroMemory(host, NI_MAXHOST); // clean up garbage
                 
                if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
                {
                    std::cout << host << " connected on port " << service << std::endl;
                }
                else
                {
                    inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
                    std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
                }

                // Send a message to client to inform connected
                std::string welcomeMsg = "Connected to the listening server on port " + std::to_string(LISTENING_PORT);
                send(clientSocket, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
                


                std::this_thread::sleep_for(std::chrono::seconds(2));


                // Send beacons to clients
                std::cout << std::endl;
                std::cout << "Sending TCP/IP beacons to " << host << "! Start now!" << std::endl;
                
                char buf[4096];

                // Server sends beacons to clients through multi threads
                std::thread sendThread(sendBeacons, clientSocket, beacons, innerDelay, outerDelay);
                sendThread.detach();
                std::thread recvThread(receiveBeacons, clientSocket, beacons, innerDelay, outerDelay);
                recvThread.detach();
            }
        }

        
        
    }
    
	// Cleanup winsock
	WSACleanup();
    system("pause");
    return 1;

}