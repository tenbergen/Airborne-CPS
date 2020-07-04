

/*

XBee configuration:

All XBee's set to the same baud rate. I used 115200 BD = 115200 [7]
AP = 1    API Mode Without Escapes
AO = 1    API Explicit RX Indicator 0x91
D6 = 1    RTS Flow Control
D7 = 1    CTS Flow Control


*/


#pragma once

#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <string>
#include <tchar.h>
#include <iostream>

// included for memory leak checking
#include <stdlib.h>
#include <crtdbg.h>

// throwing the kitchen sink at this to get XPLMDebugString to work
#include "util/StringUtil.h"
#include "component/VSpeedIndicatorGaugeRenderer.h"



 // ************************
 // Macros
 // ************************

#define MAX_COMPORT     4096     // highest possible com port number for Windows
#define READ_TIMEOUT    500      // milliseconds timeout for Reading COM Port

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
#define REST_OF_TX_FRAME	18
#define REST_OF_RX_FRAME	22

// how many bytes are excluded from the Frame's Length field value
#define BYTES_EXCLUDED_FROM_COUNT	4

// Array offsets for Received XBee API Frame (type 0x91)
#define XBEE_RX_LENGTH_HI_BYTE		1
#define XBEE_RX_LENGTH_LO_BYTE		2
#define XBEE_RX_FRAME_TYPE			3
#define XBEE_RX_PROFILE_ID			18
#define XBEE_RX_PAYLOAD_START		21




class XBee
{
public:

    bool XBeeReceive(HANDLE hComm, char* buf, int len);
    DWORD XBeeBroadcast(std::string payload, HANDLE hComm);    // //
    XBee(); //
    ~XBee(); //
    HANDLE InitializeComPort(unsigned int portnum);  //
    void XBee::SetPortnum(unsigned int pnum);
    unsigned int XBee::GetPortNum();

private:
    unsigned int portnum;
	bool isXBeeEnabled;
    int ReadSerial(unsigned char* lpBuf, DWORD dwToWrite, HANDLE hComm);  //
    bool TransmitFrame(unsigned char* lpBuf, DWORD dwToWrite, HANDLE hComm);   // 
    

};

