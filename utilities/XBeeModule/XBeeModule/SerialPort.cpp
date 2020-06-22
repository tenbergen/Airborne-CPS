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

#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <string>
#include <tchar.h>
#include <iostream>


 // ************************
 // Macros
 // ************************

#define MAX_COMPORT 4096  // highest possible com port number for Windows

 // can't actually do it this way. 
 // keeping these defines for now, but will probably be
 // removing all the multibyte ones
#define XBEE_FRAME_START_DELIMITER	0x7E
#define XBEE_TX_REQUEST_FRAME		0x10
#define XBEE_FRAME_ID				0x00
#define XBEE_16BIT_DEST_HIBYTE		0xFF
#define XBEE_16BIT_DEST_LOWBYTE		0xFE;
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


// ************************
// Function Prototypes
// ************************
BOOL TransmitFrame(char* lpBuf, DWORD dwToWrite, HANDLE hComm);
DWORD XBeeTXThread(HANDLE hComm);


// ************************
// Global Data
// ************************
uint32_t XBeeNP = 49;			// XBee Maximum Packet Payload Bytes
								// ultimately we should query the XBEE for it
								// Shouldn't be a "global" either, but for now lets just put it here


// transmit request (0x10), to broadcast address, frame ID 0(no ack response requested) payload 'Hello' 
char testHelloFrame[23] = { 0x7E, 0x00, 0x13, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, \
							0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x48, \
							0x65, 0x6C, 0x6C, 0x6F, 0x00 };


int main(int argc, char* argv[])
{




	uint32_t XBeeRxFrameMaxSize = XBeeNP + REST_OF_RX_FRAME;   // maximum possible size for XBee API Frame Type 0x91


	char* XBEEReceiveFrame = (char*)malloc(XBeeRxFrameMaxSize);



	// a lot of the serial port code comes from the following MS Code examples
	// https://docs.microsoft.com/en-us/previous-versions/ff802693(v=msdn.10)


	// This part should fill in menu options for picking com port(advanced)
	// or ommitted if we just want the user to enter in a text field.
	// XBee/Serial Port Initialization - Probably will go in Airborne-CPS.cpp::XPluginStart(), XPluginEnable() or menu handlers
	TCHAR path[10000];

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
	unsigned int comPortNum = 3;
	std::cout << "Enter COM Port Number(enter 0 to abort): ";
	std::cin >> comPortNum;
	std::cout << std::endl;

	if (comPortNum == 0) {
		exit(0);  // get out of here, heap be damned. this should not stay in production code
	}

	TCHAR gszPort[10];
	_stprintf(gszPort, L"COM%u", comPortNum);


	HANDLE hComm;
	hComm = CreateFile(gszPort,
		GENERIC_READ | GENERIC_WRITE,
		0,   // must be 0 for serial ports
		0,
		OPEN_EXISTING,    // must be OPEN_EXISTING for serial ports
		FILE_FLAG_OVERLAPPED,
		0);  // must be 0 for serial ports
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

			// Set new state.
			if (!SetCommState(hComm, &dcb))
			{
				// Error in SetCommState. Possibly a problem with the communications 
				// port handle or a problem with the DCB structure itself.
				std::cout << "Unable to set com port settings" << std::endl;

			}
			else {


				std::cout << "Sending testHelloFrame" << std::endl;

				XBeeTXThread(hComm);


				//BOOL success = TransmitFrame(testHelloFrame, sizeof(testHelloFrame), hComm);
				//std::cout << "Packet Status = " + success << std::endl;
			}
		}
	}

	// Free heap and null the pointers

	free(XBEEReceiveFrame);
	XBEEReceiveFrame = nullptr;

	return 0;
}



BOOL TransmitFrame(char* lpBuf, DWORD dwToWrite, HANDLE hComm)
{
	OVERLAPPED osWrite = { 0 };
	DWORD dwWritten;
	BOOL fRes;

	// Create this writes OVERLAPPED structure hEvent.
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL)
		// Error creating overlapped event handle.
		return FALSE;

	// Issue write.
	if (!WriteFile(hComm, lpBuf, dwToWrite, &dwWritten, &osWrite)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			// WriteFile failed, but it isn't delayed. Report error and abort.
			fRes = FALSE;
		}
		else {
			// Write is pending.
			if (!GetOverlappedResult(hComm, &osWrite, &dwWritten, TRUE))
				fRes = FALSE;
			else
				// Write operation completed successfully.
				fRes = TRUE;
		}
	}
	else
		// WriteFile completed immediately.
		fRes = TRUE;

	CloseHandle(osWrite.hEvent);
	return fRes;
}

DWORD XBeeTXThread(HANDLE hComm) {
	// build the TX Frame

	// simulating what we will get from Location::getPLANE()
	// this data shoul yeild a API Frame that is 80 bytes long, and has a legth field of 0x004C (decimal 76)

	std::string myLocationGetPlane = "n4C:ED:FB:59:53:00n192.168.0.3n47.519961n10.698863n3050.078383";
	//std::string myLocationGetPlane = "";

	// Determine frame size and allocate memory on the heap for it
	uint32_t PayloadSize = myLocationGetPlane.length();
	uint32_t XBeeTxFrameSize = PayloadSize + REST_OF_TX_FRAME;
	uint32_t ChecksumOffset = XBeeTxFrameSize - 1; // the last position of the array is the checksum offset. 
	char* XBeeTXFrame = (char*)malloc(XBeeTxFrameSize);   // put our working frames on the heap

	if (XBeeTXFrame != NULL)
	{
		// Fill the frame with the data 
		// [ Field (number of bytes in field) | Next field (num bytes) | etc...]
		// [ Start Delim (1) | Length (2) | Frame Type (1) | Frame ID (1) | 64bit Dst Addr (8) | 16bit Dst Addr (2) | Radius (1) | Options (1) | Payload (PayloadSize) | Checksum (1) ]
		XBeeTXFrame[XBEE_TXOFFSET_START_DELIM] =	XBEE_FRAME_START_DELIMITER;
		XBeeTXFrame[XBEE_TXOFFSET_LENGTH_HIBYTE] =	(char)(XBeeTxFrameSize - BYTES_EXCLUDED_FROM_COUNT >> 8);  // Length field does not count Delim, Length, or Checksum
		XBeeTXFrame[XBEE_TXOFFSET_LENGTH_LOBYTE] =	(char)(XBeeTxFrameSize - BYTES_EXCLUDED_FROM_COUNT & 0xff);
		XBeeTXFrame[XBEE_TXOFFSET_FRAME_TYPE] =		XBEE_TX_REQUEST_FRAME;
		XBeeTXFrame[XBEE_TXOFFSET_FRAME_ID] =		XBEE_FRAME_ID;
		
		// iterate through the 8 byte destination address backwards to store as big-endian in the API Frame
		uint64_t broadcastAddress = 0x000000000000FFFF;
		for (uint32_t i = XBEE_TXOFFSET_64BIT_DST_ADDR_END; i >= XBEE_TXOFFSET_64BIT_DST_ADDR_START; i--) {
			XBeeTXFrame[i] = (char)(broadcastAddress & 0xff);	// write the LSB to the array position
			broadcastAddress = broadcastAddress >> 8;			// shift the address one byte right
		}
		
		XBeeTXFrame[XBEE_TXOFFSET_16BIT_DST_ADDR_HIBYTE] =	XBEE_16BIT_DEST_HIBYTE;
		XBeeTXFrame[XBEE_TXOFFSET_16BIT_DST_ADDR_LOBYTE] =	XBEE_16BIT_DEST_LOWBYTE;
		XBeeTXFrame[XBEE_TXOFFSET_BROADCAST_RAD] =			XBEE_BROADCAST_RADIUS;
		XBeeTXFrame[XBEE_TXOFFSET_TX_OPTIONS] = XBEE_TX_OPTIONS;


		// fill in the payload.  myLocationGetPlane is the payload
		memcpy(XBeeTXFrame + XBEE_TXOFFSET_PAYLOAD_START, myLocationGetPlane.c_str(), PayloadSize);


		//Calculate checksum  checksum = add bytes from frame type to payload, only keep LSB, subtract that from 0xFF.
		uint32_t sum = 0;
		
		// change this to use XBeeTxFrameSize
		for (uint32_t i = XBEE_TXOFFSET_FRAME_TYPE; i < XBeeTxFrameSize - 1; i++) { 
			sum += XBeeTXFrame[i];
		}
		char checksum = (char)(0xFF - (sum & 0xff));

		// place the checksum at the end of the array, after the payload. 
		XBeeTXFrame[ChecksumOffset] = checksum;
		

		// Send the frame
		TransmitFrame(XBeeTXFrame, XBeeTxFrameSize, hComm);

		// free up memory and null the pointer
		free(XBeeTXFrame);
		XBeeTXFrame = nullptr;

		Sleep(1000);
		
	}
	return 0;
}

//BOOL ReadSerial(char* lpBuf, DWORD dwToWrite, HANDLE hComm) {
//	DWORD dwRead;
//	BOOL fWaitingOnRead = FALSE;
//	OVERLAPPED osReader = { 0 };
//
//	// Create the overlapped event. Must be closed before exiting
//	// to avoid a handle leak.
//	osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//
//	if (osReader.hEvent == NULL)
//		// Error creating overlapped event; abort.
//
//		if (!fWaitingOnRead) {
//			// Issue read operation.
//			if (!ReadFile(hComm, lpBuf, READ_BUF_SIZE, &dwRead, &osReader)) {
//				if (GetLastError() != ERROR_IO_PENDING)     // read not delayed?
//				   // Error in communications; report it.
//				else
//					fWaitingOnRead = TRUE;
//			}
//			else {
//				// read completed immediately
//				HandleASuccessfulRead(lpBuf, dwRead);
//			}
//		}
//}
