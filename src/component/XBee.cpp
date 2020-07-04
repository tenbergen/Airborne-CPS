#include "component/XBee.h"

#pragma comment(lib,"WS2_32")


XBee::XBee()
{
	bool isXBeeEnabled = false;
	this->SetPortnum(3);

}

XBee::~XBee()
{
	// Do something useful
}

void XBee::SetPortnum(unsigned int pnum)
{
	portnum = pnum;
}

unsigned int XBee::GetPortNum() {
	return portnum;
}



HANDLE XBee::InitializeComPort(unsigned int portnum) {
	TCHAR gszPort[10];
	_stprintf(gszPort, "COM%u", portnum);


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

bool XBee::XBeeReceive(HANDLE hComm, char* buf, int len) {

	bool retVal = false;

	FILE* pPayloadFile;
	pPayloadFile = fopen("payload.txt", "a+b");

	FILE* pRawDataFile;
	pRawDataFile = fopen("rawdata.hex", "a+b");

	unsigned char fileDelimiter[12] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };

	//while (exit == false) {

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


				if (checksum == XBeeRXFrame[checksumOffset]) {
					// now that we have that, we can calculate the payload length
					uint16_t payloadLength = checksumOffset - XBEE_RXOFFSET_PAYLOAD_START;

					if (payloadLength < len) { 
						retVal = true;
						memcpy(buf, XBeeRXFrame + XBEE_RXOFFSET_PAYLOAD_START, payloadLength);
					}

					//fwrite(XBeeRXFrame + XBEE_RXOFFSET_PAYLOAD_START, sizeof(unsigned char), payloadLength, pPayloadFile);
					fwrite(buf, sizeof(unsigned char), payloadLength, pPayloadFile);

					fwrite("\n", 1, 1, pPayloadFile);
					fflush(pPayloadFile);
					fflush(pRawDataFile);
				}
			}
		}

		// this whole plan is the problem. I'm returning

		// free up memory and null the pointer
		free(XBeeRXFrame);
		XBeeRXFrame = nullptr;

	fclose(pPayloadFile);
	fclose(pRawDataFile);
	return retVal;
}

int XBee::ReadSerial(unsigned char* lpBuf, DWORD dwToWrite, HANDLE hComm) {
	DWORD dwRead;
	bool fWaitingOnRead = FALSE;

	int retVal = ReadFile(hComm, lpBuf, XBEE_MAX_API_FRAME_SIZE, &dwRead, NULL);
	if (retVal) { return dwRead; }
	else { return 0; }
}

DWORD XBee::XBeeBroadcast(std::string payload, HANDLE hComm) {

	//bool exit = false;
	//while (exit == false) {

		// simulating what we will get from Location::getPLANE()
	std::string myLocationGetPlane = payload;


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
			//if (GetAsyncKeyState(VK_ESCAPE)) {
			//	exit = true;
			//}

			//Sleep(1000);

		}
	//}
	//std::cout << "TX Thread Exiting" << std::endl;
	return 0;
}

bool XBee::TransmitFrame(unsigned char* lpBuf, DWORD dwToWrite, HANDLE hComm)
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