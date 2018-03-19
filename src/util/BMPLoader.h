#pragma once
/// Cross Platform Bitmap functions and bitmap data structures

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "XPLMUtilities.h"

// @author nstemmle - original code was not written by me but modifications (added alpha channel support)
// and refactoring changes were made by me
class BmpLoader {

public:
// These need to be aligned
#pragma pack(push, ident, 2)
	typedef struct TagBmpFileHeader
	{
		short  bfType;
		int	   bfSize;
		short  bfReserved1;
		short  bfReserved2;
		int    bfOffBits;
	} BmpFileHeader;

	typedef struct TagBmpInfoHeader
	{
		int     biSize;
		int     biWidth;
		int     biHeight;
		short   biPlanes;
		short   biBitCount;
		int     biCompression;
		int     biSizeImage;
		int     biXPelsPerMeter;
		int     biYPelsPerMeter;
		int     biClrUsed;
		int     biClrImportant;
	} BmpInfoHeader;

	typedef struct	TagImageData
	{
		unsigned char *	pData;
		int			width;
		int			height;
		int			padding;
		short		channels;
	} ImageData;

#pragma pack(pop, ident)

	static int		loadBmp(const char *filePath, ImageData *imageData);
	static void		swapRedBlue(ImageData *imageData);

	/// Cross Platform Bitmap functions
	/// Functions to handle endian differeneces between windows, linux and mac.
#if APL
	static short Endian(short Data)
	{
		unsigned char *pBuffer = (unsigned char *)&Data;
		short Result = (short)(pBuffer[0] & 0xff) + ((short)(pBuffer[1] & 0xff) << 8);
		return(Result);
	}

	static int Endian(int Data)
	{
		unsigned char *pBuffer = (unsigned char *)&Data;

		int Result = (int)(pBuffer[0] & 0xff)
			+ ((int)(pBuffer[1] & 0xff) << 8)
			+ ((int)(pBuffer[2] & 0xff) << 16)
			+ ((int)(pBuffer[3] & 0xff) << 24);

		return(Result);
	}

	static void SwapEndian(short *Data)
	{
		*Data = Endian(*Data);
	}

	static void SwapEndian(int *Data)
	{
		*Data = Endian(*Data);
	}
#else
	/// Only the mac needs these so dummy functions for windows and linux.
	static void swapEndian(short *data) {}
	static void swapEndian(int *data) {}
#endif
};