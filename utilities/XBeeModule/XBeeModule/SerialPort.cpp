/*
 * Copyright (c) 2010-2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

 /*
	Solution to work on planning and building the xbee module for Airborne-CPS

	XPluginStart should
		- create a new menu item for XBee, to select COM Port, disable/enable serial comms
			- whatever handlers for those menu items
			- maybe enable/disable XBee<->UDP bridging?
		- init/start the XBeeModule if that is enabled

	XBeeModule
		- API for init and start/stop serial comms for XPluginStart to interact with
		- Internally create HANDLE to serial port and pass to threads
		- XBeeTXThread() and a XBeeRXThread()
			- from what I can find, as long as RX only reads, and TX only writes, it is thread safe
		- Eventually query the XBee to get its maximum payload size in bytes (NP). for now, hard code to match ours
		- Only sends frame type 0x10 for now -  trasmit request
		- Only receives frame type 0x91 for now

 */

 // use debug versions of malloc and free
#define _CRTDBG_MAP_ALLOC
#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <string>
#include <tchar.h>
#include <iostream>

// included for memory leak checking
#include <stdlib.h>
#include <crtdbg.h>



#pragma comment(lib,"WS2_32")

 // ************************
 // Macros
 // ************************

#define MAX_COMPORT 4096  // highest possible com port number for Windows
#define READ_TIMEOUT      500      // milliseconds timeout for Reading COM Port

// Defined XBee Field Values
#define XBEE_FRAME_START_DELIMITER	0x7E
#define XBEE_TX_REQUEST_FRAME_TYPE	0x10
#define XBEE_RECIEVE_FRAME_TYPE		0x91
#define XBEE_FRAME_ID				0x00
#define XBEE_16BIT_DEST_HIBYTE		0xFF
#define XBEE_16BIT_DEST_LOWBYTE		0xFE
#define XBEE_BROADCAST_RADIUS		0x00
#define XBEE_TX_OPTIONS				0x00

// Array offsets for XBee Trasmit Request API  Frame (Frame type 0x10)
#define XBEE_TXOFFSET_START_DELIM			0
#define XBEE_TXOFFSET_LENGTH_HIBYTE			1
#define XBEE_TXOFFSET_LENGTH_LOBYTE			2
#define XBEE_TXOFFSET_FRAME_TYPE			3
#define XBEE_TXOFFSET_FRAME_ID				4
#define XBEE_TXOFFSET_64BIT_DST_ADDR_START	5
#define XBEE_TXOFFSET_64BIT_DST_ADDR_END	12
#define XBEE_TXOFFSET_16BIT_DST_ADDR_HIBYTE	13
#define XBEE_TXOFFSET_16BIT_DST_ADDR_LOBYTE	14
#define XBEE_TXOFFSET_BROADCAST_RAD			15
#define XBEE_TXOFFSET_TX_OPTIONS			16
#define XBEE_TXOFFSET_PAYLOAD_START			17
// the checksum is the last byte, its offset is determined at runtime

//Maximum size that an API Frame could possibly be
#define XBEE_MAX_API_FRAME_SIZE		256

// Array offsets for XBee Trasmit Request API  Frame (Frame type 0x10)
#define XBEE_RXOFFSET_START_DELIM			0
#define XBEE_RXOFFSET_LENGTH_HIBYTE			1
#define XBEE_RXOFFSET_LENGTH_LOBYTE			2
#define XBEE_RXOFFSET_FRAME_TYPE			3
#define XBEE_RXOFFSET_PAYLOAD_START			21
// the checksum is the last byte, its offset is determined at runtime


// how many bytes an API Frame has in addition to its payload
#define REST_OF_TX_FRAME			18
#define REST_OF_RX_FRAME			22

// how many bytes are excluded from the Frame's Length field value
#define BYTES_EXCLUDED_FROM_COUNT	4

// Array offsets for Received XBee API Frame (type 0x91)
#define XBEE_RX_LENGTH_HI_BYTE		1
#define XBEE_RX_LENGTH_LO_BYTE		2
#define XBEE_RX_FRAME_TYPE			3
#define XBEE_RX_PROFILE_ID			18
#define XBEE_RX_PAYLOAD_START		21




bool TransmitFrame(unsigned char* lpBuf, DWORD dwToWrite, HANDLE hComm)
{
	OVERLAPPED osWrite = { 0 };
	DWORD dwWritten;
	bool fRes;

	// Create this writes OVERLAPPED structure hEvent.
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL)
		// Error creating overlapped event handle.
		return false;

	// Issue write.
	if (!WriteFile(hComm, lpBuf, dwToWrite, &dwWritten, &osWrite)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			// WriteFile failed, but it isn't delayed. Report error and abort.
			fRes = false;
		}
		else {
			// Write is pending.
			if (!GetOverlappedResult(hComm, &osWrite, &dwWritten, TRUE))
				fRes = false;
			else
				// Write operation completed successfully.
				fRes = true;
		}
	}
	else
		// WriteFile completed immediately.
		fRes = false;

	CloseHandle(osWrite.hEvent);
	return fRes;
}



DWORD XBeeTXThread(HANDLE hComm) {

	bool exit = false;
	while (exit == false) {

		// simulating what we will get from Location::getPLANE()
		std::string myLocationGetPlane = "nFF:00:00:60:53:2En192.168.0.1n47.581802n10.665801n3047.689339";


		// Determine frame size and allocate memory on the heap for it
		uint32_t PayloadSize = myLocationGetPlane.length();
		uint32_t XBeeTxFrameSize = PayloadSize + REST_OF_TX_FRAME;
		uint32_t ChecksumOffset = XBeeTxFrameSize - 1; // the last position of the array is the checksum offset. 
		unsigned char* XBeeTXFrame = (unsigned char*)malloc(XBeeTxFrameSize);   // put our working frames on the heap

		if (XBeeTXFrame != NULL)
		{
			// Fill the frame with the data 
			// [ Field (number of bytes in field) | Next field (num bytes) | etc...]
			// [ Start Delim (1) | Length (2) | Frame Type (1) | Frame ID (1) | 64bit Dst Addr (8) | 16bit Dst Addr (2) | Radius (1) | Options (1) | Payload (PayloadSize) | Checksum (1) ]
			XBeeTXFrame[XBEE_TXOFFSET_START_DELIM] = XBEE_FRAME_START_DELIMITER;
			XBeeTXFrame[XBEE_TXOFFSET_LENGTH_HIBYTE] = (char)((XBeeTxFrameSize - BYTES_EXCLUDED_FROM_COUNT) >> 8);  // Length field does not count Delim, Length, or Checksum
			XBeeTXFrame[XBEE_TXOFFSET_LENGTH_LOBYTE] = (char)((XBeeTxFrameSize - BYTES_EXCLUDED_FROM_COUNT) & 0xff);
			XBeeTXFrame[XBEE_TXOFFSET_FRAME_TYPE] = XBEE_TX_REQUEST_FRAME_TYPE;
			XBeeTXFrame[XBEE_TXOFFSET_FRAME_ID] = XBEE_FRAME_ID;

			// iterate through the 8 byte destination address backwards to store as big-endian in the API Frame
			uint64_t broadcastAddress = 0x000000000000FFFF;
			for (uint32_t i = XBEE_TXOFFSET_64BIT_DST_ADDR_END; i >= XBEE_TXOFFSET_64BIT_DST_ADDR_START; i--) {
				XBeeTXFrame[i] = (char)(broadcastAddress & 0xff);	// write the LSB to the array position
				broadcastAddress = broadcastAddress >> 8;			// shift the address one byte right
			}

			XBeeTXFrame[XBEE_TXOFFSET_16BIT_DST_ADDR_HIBYTE] = XBEE_16BIT_DEST_HIBYTE;
			XBeeTXFrame[XBEE_TXOFFSET_16BIT_DST_ADDR_LOBYTE] = XBEE_16BIT_DEST_LOWBYTE;
			XBeeTXFrame[XBEE_TXOFFSET_BROADCAST_RAD] = XBEE_BROADCAST_RADIUS;
			XBeeTXFrame[XBEE_TXOFFSET_TX_OPTIONS] = XBEE_TX_OPTIONS;


			// fill in the payload.  myLocationGetPlane is the payload
			memcpy(XBeeTXFrame + XBEE_TXOFFSET_PAYLOAD_START, myLocationGetPlane.c_str(), PayloadSize);


			//Calculate checksum  checksum = add bytes from frame type to payload, only keep LSB, subtract that from 0xFF.
			uint32_t sum = 0;

			// change this to use XBeeTxFrameSize
			for (uint32_t i = XBEE_TXOFFSET_FRAME_TYPE; i < ChecksumOffset; i++) {
				sum += XBeeTXFrame[i];
			}
			char checksum = (char)(0xFF - (sum & 0xff));

			// place the checksum at the end of the array, after the payload. 
			XBeeTXFrame[ChecksumOffset] = checksum;


			// Send the frame
			if (TransmitFrame(XBeeTXFrame, XBeeTxFrameSize, hComm)) {
				std::cout << "TransmitFrame: Success" << std::endl;
			}

			// free up memory and null the pointer
			free(XBeeTXFrame);
			XBeeTXFrame = nullptr;
			if (GetAsyncKeyState(VK_ESCAPE)) {
				exit = true;
			}

			Sleep(1000);

		}
	}
	std::cout << "TX Thread Exiting" << std::endl;
	return 0;
}

int ReadSerial(unsigned char* lpBuf, DWORD dwToWrite, HANDLE hComm) {
	DWORD dwRead;
	bool fWaitingOnRead = FALSE;

	int retVal = ReadFile(hComm, lpBuf, XBEE_MAX_API_FRAME_SIZE, &dwRead, NULL);
	if (retVal) { return dwRead; }
	else { return 0; }
}


DWORD XBeeRXThread(HANDLE hComm) {

	bool exit = false;
	FILE* pPayloadFile;
	pPayloadFile = fopen("payload.txt", "wb");

	FILE* pRawDataFile;
	pRawDataFile = fopen("rawdata.hex", "wb");



	unsigned char fileDelimiter[12] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };

	while (exit == false) {

		// allocate memory on the heap to hold the received API Frame
		unsigned char* XBeeRXFrame = (unsigned char*)malloc(XBEE_MAX_API_FRAME_SIZE);

		// Blocking Wait for XBee data to be received and placed into XBeeRXFrame
		int charsRead = ReadSerial(XBeeRXFrame, XBEE_MAX_API_FRAME_SIZE, hComm);
		if (charsRead) {

			fwrite(XBeeRXFrame, sizeof(unsigned char), charsRead, pRawDataFile);
			fwrite(fileDelimiter, sizeof(unsigned char), sizeof(fileDelimiter), pRawDataFile);

			// Check for the start delimiter and that frame type is 0x91
			if ((XBeeRXFrame[XBEE_RXOFFSET_START_DELIM] == XBEE_FRAME_START_DELIMITER) &&
				(XBeeRXFrame[XBEE_RXOFFSET_FRAME_TYPE] == XBEE_RECIEVE_FRAME_TYPE)) {

				// Read in the length from the Frame (offsets 1 and 2, big-endian)
				uint16_t length = (XBeeRXFrame[XBEE_RXOFFSET_LENGTH_HIBYTE] << 8) | XBeeRXFrame[XBEE_RXOFFSET_LENGTH_LOBYTE];


				uint32_t checksumOffset = length + XBEE_RXOFFSET_FRAME_TYPE;
				uint32_t sum = 0;
				// check the checksum value to make sure the API Frame is complete/well formed
				for (unsigned int i = XBEE_RXOFFSET_FRAME_TYPE; i < checksumOffset; i++) {
					sum += XBeeRXFrame[i];
				}
				unsigned char checksum = (unsigned char)(0xFF - (sum & 0xff));
				printf("Received Checksum: %x\nCalculated Checksum: %x\n", XBeeRXFrame[checksumOffset], checksum);
				if (checksum == XBeeRXFrame[checksumOffset]) {
					// now that we have that, we can calculate the payload length
					uint16_t payloadLength = checksumOffset - XBEE_RXOFFSET_PAYLOAD_START;


					fwrite(XBeeRXFrame + XBEE_RXOFFSET_PAYLOAD_START, sizeof(unsigned char), payloadLength, pPayloadFile);
					fwrite("\n", 1, 1, pPayloadFile);
				}


			}


		}


		// free up memory and null the pointer
		free(XBeeRXFrame);
		XBeeRXFrame = nullptr;

		if (GetAsyncKeyState(VK_ESCAPE)) {
			exit = true;
		}

	}

	fclose(pPayloadFile);
	fclose(pRawDataFile);
	std::cout << "RX Thread Exiting" << std::endl;
	return 0;
}

static DWORD WINAPI startXBeeBroadcasting(void* param) {
	HANDLE hComm = (HANDLE)param;
	return XBeeTXThread(hComm);
}

static DWORD WINAPI startXBeeListening(void* param) {
	HANDLE hComm = (HANDLE)param;
	return XBeeRXThread(hComm);
}

HANDLE InitializeComPort(unsigned int portnum) {
	TCHAR gszPort[10];
	_stprintf(gszPort, L"COM%u", portnum);


	HANDLE hComm;
	hComm = CreateFile(gszPort,
		GENERIC_READ | GENERIC_WRITE,
		0,   // must be 0 for serial ports
		NULL,
		OPEN_EXISTING,    // must be OPEN_EXISTING for serial ports
		0,
		NULL);  // must be 0 for serial ports
	if (hComm == INVALID_HANDLE_VALUE) {
		std::cout << "Invalid Handle Value. Cannot Open port" << std::endl;// error opening port; abort
	}
	else {

		// get current com port settings
		DCB dcb = { 0 };
		FillMemory(&dcb, sizeof(dcb), 0);
		if (!GetCommState(hComm, &dcb))
		{       // Error getting current DCB settings
			std::cout << "Unable to get com port settings" << std::endl;

		}
		else {

			// Modify DCB 
			dcb.BaudRate = CBR_115200;
			dcb.ByteSize = 8;
			dcb.Parity = NOPARITY;
			dcb.StopBits = ONESTOPBIT;
			dcb.fOutxCtsFlow = 1;
			dcb.fRtsControl = 2;

			// Set new state.
			if (!SetCommState(hComm, &dcb))
			{
				// Error in SetCommState. Possibly a problem with the communications 
				// port handle or a problem with the DCB structure itself.
				std::cout << "Unable to set RX com port settings" << std::endl;

			}
			else {
				COMMTIMEOUTS timeouts;
				// set short timeouts on the comm port.
				timeouts.ReadIntervalTimeout = 1;
				timeouts.ReadTotalTimeoutMultiplier = 1;
				timeouts.ReadTotalTimeoutConstant = 1;
				timeouts.WriteTotalTimeoutMultiplier = 1;
				timeouts.WriteTotalTimeoutConstant = 1;
				if (!SetCommTimeouts(hComm, &timeouts)) {
					std::cout << "Error setting port time-outs." << std::endl;
				}
			}
		}
	}
	return hComm;
}


int main(int argc, char* argv[])
{

	bool isRXenabled = false;
	bool isTXenabled = false;
	bool isSingleXBDuplex = false;

	HANDLE hRXThread = 0;
	HANDLE hTXThread = 0;

	// a lot of the serial port code comes from the following MS Code examples
	// https://docs.microsoft.com/en-us/previous-versions/ff802693(v=msdn.10)


	// This part should fill in menu options for picking com port(advanced)
	// or ommitted if we just want the user to enter in a text field.
	// XBee/Serial Port Initialization - Probably will go in Airborne-CPS.cpp::XPluginStart(), XPluginEnable() or menu handlers


	// *********************************************************************
	//  End User Input Section
	// *********************************************************************

	TCHAR path[5000];

	printf("Checking COM1 through COM%d:\n", MAX_COMPORT);
	for (unsigned i = 1; i <= MAX_COMPORT; ++i) {

		// iterate through possible com ports by name

		TCHAR buffer[10];
		_stprintf(buffer, L"COM%u", i);



		DWORD result = QueryDosDevice(buffer, path, (sizeof path) / 2);
		if (result != 0) {
			_tprintf(L"%s:\t%s\n", buffer, path);
		}
		else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			_tprintf(L"%s:\t%s\n", buffer, L"(error, path buffer too small)");

		}
	}

	// comPortNum should get filled by menu item 
	unsigned int TXcomPortNum = 0;
	std::cout << "Enter COM Port Number for TX(enter 0 for no TX): ";
	std::cin >> TXcomPortNum;
	std::cout << std::endl;

	if (TXcomPortNum == 0) {
		std::cout << "No TX Port Selected" << std::endl;  // get out of here, heap be damned. this should not stay in production code
	}
	else {
		isTXenabled = true;
	}

	unsigned int RXcomPortNum = 0;
	std::cout << "Enter COM Port Number for RX(enter 0 for no RX): ";
	std::cin >> RXcomPortNum;
	std::cout << std::endl;

	if (RXcomPortNum == 0) {
		std::cout << "No RX Port Selected" << std::endl;
		isRXenabled = false;
		if (!isTXenabled) {
			std::cout << "No COM Ports Selected. Nothing to do but exit." << std::endl;
			exit(0);
		}
	}
	else {
		if (RXcomPortNum == TXcomPortNum) {
			std::cout << "COM " << RXcomPortNum << " selected for both TX and RX. Entering Single XBee Duplex Mode." << std::endl;
			isSingleXBDuplex = true;
			isTXenabled = false;
			isRXenabled = false;

		}
		else {
			isRXenabled = true;
		}

	}

	// *********************************************************************
	//  End User Input Section
	// *********************************************************************

	//
	// Single XBee Duplex Mode Configuration
	//

	if (isSingleXBDuplex) {
		unsigned int comPortNum = TXcomPortNum;
		HANDLE hComm = InitializeComPort(comPortNum);
		if (hComm == INVALID_HANDLE_VALUE) {
			std::cout << "Invalid COM Port Number. COM: " << comPortNum << " Cannot Continue." << std::endl;
		}
		else {
			std::cout << "Starting TX Thread on COM Port " << comPortNum << std::endl;
			DWORD TXThreadID;
			hTXThread = CreateThread(NULL, 0, startXBeeBroadcasting, (void*)hComm, 0, &TXThreadID);

			std::cout << "Starting RX Thread on COM Port " << comPortNum << std::endl;
			DWORD RXThreadID;
			hRXThread = CreateThread(NULL, 0, startXBeeListening, (void*)hComm, 0, &RXThreadID);
		}
	}
	else if (isTXenabled) {

		HANDLE hTXComm = InitializeComPort(TXcomPortNum);
		if (hTXComm == INVALID_HANDLE_VALUE) {
			std::cout << "Invalid TX COM Port Number. COM: " << TXcomPortNum << " Cannot Continue." << std::endl;

		}
		else {
			std::cout << "Starting TX Thread on COM Port " << TXcomPortNum << std::endl;
			DWORD TXThreadID;
			hTXThread = CreateThread(NULL, 0, startXBeeBroadcasting, (void*)hTXComm, 0, &TXThreadID);
		}
	}
	else if (isRXenabled) {

		HANDLE hRXComm = InitializeComPort(RXcomPortNum);
		if (hRXComm == INVALID_HANDLE_VALUE) {
			std::cout << "Invalid RX COM Port Number. COM: " << RXcomPortNum << " Cannot Continue." << std::endl;
		}
		else {
			std::cout << "Starting RX Thread on COM Port " << RXcomPortNum << std::endl;
			DWORD RXThreadID;
			hRXThread = CreateThread(NULL, 0, startXBeeListening, (void*)hRXComm, 0, &RXThreadID);
		}
	}

	std::cout << "Hit Esc a few times to exit." << std::endl;

	if (hTXThread) {
		WaitForSingleObject(hTXThread, INFINITE);
		CloseHandle(hTXThread);
		//CloseHandle(hTXComm);
	}
	if (hRXThread) {
		WaitForSingleObject(hRXThread, INFINITE);
		CloseHandle(hRXThread);
		//CloseHandle(hRXComm);
	}

	_CrtDumpMemoryLeaks();
	return 0;
}


