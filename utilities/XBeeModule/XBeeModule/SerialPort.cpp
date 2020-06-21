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
	 List all COM ports available via the efficient QueryDosDevice() method.
 */

#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <string>
#include <tchar.h>
#include <iostream>

#define MAX_COMPORT 4096
#define XBEE_FRAME_START_DELIMITER 0x7E
#define XBEE_TX_REQUEST_FRAME 0x10
#define XBEE_FRAME_ID 0x00
#define XBEE_64BIT_DEST_ADDR 0x000000000000FFFF
#define XBEE_16BIT_DEST_ADDR 0xFFFE;
#define XBEE_BROADCAST_RADIUS 0x00
#define XBEE_TX_OPTIONS 0x00

BOOL WriteABuffer(char* lpBuf, DWORD dwToWrite, HANDLE hComm);

TCHAR path[10000];
//std::string path;

// transmit request (0x10), to broadcast address, frame ID 0(no ack response requested) payload 'Hello' 
char testHelloFrame[23] = { 0x7E, 0x00, 0x13, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, \
							0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x48, \
							0x65, 0x6C, 0x6C, 0x6F, 0x00 };


int main(int argc, char* argv[])

{
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
			_tprintf(L"%s:\t%s\n", buffer, "(error, path buffer too small)");
			//std::wcout << L""
		}
	}


	// https://docs.microsoft.com/en-us/previous-versions/ff802693(v=msdn.10)


		// create and fill a Device Control Block to set the COM Port settings
		//DCB dcb;

		//FillMemory(&dcb, sizeof(dcb), 0);
		//dcb.DCBlength = sizeof(dcb);
		//if (!BuildCommDCB(L"115200,n,8,1", &dcb)) {
		//    // Couldn't build the DCB. Usually a problem
		//    // with the communications specification string.
		//    std::cout << "Unable to initialize Device Control Block struct" << std::endl;
		//    exit(0);
		//}

			// DCB is ready for use.

		// just simulating getting the number 3 from the above loop
	TCHAR gszPort[10];
	_stprintf(gszPort, L"COM%u", 3);


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
			exit(0);
		}

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
			exit(0);
		}


		std::cout << "Sending testHelloFrame" << std::endl;
		BOOL success = WriteABuffer(testHelloFrame, sizeof(testHelloFrame), hComm);
		std::cout << "Packet Status = " + success << std::endl;
	}


	return 0;
}

BOOL WriteABuffer(char* lpBuf, DWORD dwToWrite, HANDLE hComm)
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
