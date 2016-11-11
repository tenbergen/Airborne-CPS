#include "BMPLoader.h"

///// Generic bitmap loader to handle all platforms
//int BmpLoader::LoadBmp(const char * FilePath, IMAGEDATA * ImageData)
//{
//	BMPFILEHEADER   Header;
//	BMPINFOHEADER	ImageInfo;
//	int						Padding;
//	int success = 0;
//
//	ImageData->pData = NULL;
//	FILE * BitmapFile = fopen(FilePath, "rb");
//
//	if (BitmapFile != NULL)
//	{
//		if (fread(&Header, sizeof(Header), 1, BitmapFile) == 1)
//		{
//			if (fread(&ImageInfo, sizeof(ImageInfo), 1, BitmapFile) == 1)
//			{
//				/// Handle Header endian.
//				SwapEndian(&Header.bfSize);
//				SwapEndian(&Header.bfOffBits);
//
//				/// Handle ImageInfo endian.
//				SwapEndian(&ImageInfo.biWidth);
//				SwapEndian(&ImageInfo.biHeight);
//				SwapEndian(&ImageInfo.biBitCount);
//
//				short channels = ImageInfo.biBitCount / 8;
//
//				/// Make sure that it is a bitmap.
//#if APL && defined(__POWERPC__)
//				if (((Header.bfType & 0xff) == 'M') &&
//					(((Header.bfType >> 8) & 0xff) == 'B') &&
//#else
//				if (((Header.bfType & 0xff) == 'B') &&
//					(((Header.bfType >> 8) & 0xff) == 'M') &&
//#endif
//					(ImageInfo.biBitCount == 24 || ImageInfo.biBitCount == 32) &&
//					(ImageInfo.biWidth > 0) &&
//					(ImageInfo.biHeight > 0))
//				{
//					/// "Header.bfSize" does not always agree
//					/// with the actual file size and can sometimes be "ImageInfo.biSize"	 smaller.
//					/// So add it in for good measure
//					if ((Header.bfSize + ImageInfo.biSize - Header.bfOffBits) >= (ImageInfo.biWidth * ImageInfo.biHeight * channels))
//					{
//						Padding = (ImageInfo.biWidth * channels + channels) & ~channels;
//						Padding -= ImageInfo.biWidth * channels;
//
//						ImageData->Width = ImageInfo.biWidth;
//						ImageData->Height = ImageInfo.biHeight;
//						ImageData->Padding = Padding;
//
//						/// Allocate memory for the actual image.
//						ImageData->Channels = channels;
//						unsigned short img_data_size = ImageInfo.biWidth * ImageInfo.biHeight * channels + ImageInfo.biHeight * Padding;
//						ImageData->pData = (unsigned char *)malloc(img_data_size);
//
//						if (ImageData->pData)
//						{
//							success = fread(ImageData->pData, img_data_size, 1, BitmapFile) != 0;
//						}
//					}
//				}
//			}
//		}
//	}
//
//	if (BitmapFile)
//		fclose(BitmapFile);
//
//	return success;
//}

int BmpLoader::LoadBmp(const char * FilePath, IMAGEDATA * ImageData)
{
	char debugStringBuf[256];
	sprintf(debugStringBuf, "ExampleGuage::BitmapLoader - FilePath: %s\n", FilePath);
	XPLMDebugString(debugStringBuf);
	BMPFILEHEADER   Header;
	BMPINFOHEADER	ImageInfo;
	int						Padding;
	FILE *					BitmapFile = NULL;
	int RetCode = 0;

	ImageData->pData = NULL;

	BitmapFile = fopen(FilePath, "rb");
	if (BitmapFile != NULL)
	{
		if (fread(&Header, sizeof(Header), 1, BitmapFile) == 1)
		{
			if (fread(&ImageInfo, sizeof(ImageInfo), 1, BitmapFile) == 1)
			{
				/// Handle Header endian.
				SwapEndian(&Header.bfSize);
				SwapEndian(&Header.bfOffBits);

				/// Handle ImageInfo endian.
				SwapEndian(&ImageInfo.biWidth);
				SwapEndian(&ImageInfo.biHeight);
				SwapEndian(&ImageInfo.biBitCount);

				short channels = ImageInfo.biBitCount / 8;

				/// Make sure that it is a bitmap.
#if APL && defined(__POWERPC__)
				if (((Header.bfType & 0xff) == 'M') &&
					(((Header.bfType >> 8) & 0xff) == 'B') &&
#else
				if (((Header.bfType & 0xff) == 'B') &&
					(((Header.bfType >> 8) & 0xff) == 'M') &&
#endif
					(ImageInfo.biBitCount == 24 || ImageInfo.biBitCount == 32) &&
					(ImageInfo.biWidth > 0) &&
					(ImageInfo.biHeight > 0))
				{
					/// "Header.bfSize" does not always agree
					/// with the actual file size and can sometimes be "ImageInfo.biSize"	 smaller.
					/// So add it in for good measure
					if ((Header.bfSize + ImageInfo.biSize - Header.bfOffBits) >= (ImageInfo.biWidth * ImageInfo.biHeight * channels))
					{
						Padding = (ImageInfo.biWidth * channels + channels) & ~channels;
						Padding -= ImageInfo.biWidth * channels;

						char padInfoBuf[128];
						snprintf(padInfoBuf, 128, "ImageInfo.biWidth: %d, channels: %d, padding: %d\n", ImageInfo.biWidth, channels, Padding);
						XPLMDebugString(padInfoBuf);

						ImageData->Width = ImageInfo.biWidth;
						ImageData->Height = ImageInfo.biHeight;
						ImageData->Padding = Padding;

						/// Allocate memory for the actual image.
						ImageData->Channels = channels;
						ImageData->pData = (unsigned char *)malloc(ImageInfo.biWidth * ImageInfo.biHeight * ImageData->Channels + ImageInfo.biHeight * Padding);

						if (ImageData->pData != NULL)
						{
							/// Get the actual image.
							if (fread(ImageData->pData, ImageInfo.biWidth * ImageInfo.biHeight * ImageData->Channels + ImageInfo.biHeight * Padding, 1, BitmapFile) == 1)
							{
								RetCode = 1;
							}
							else {
								XPLMDebugString("Failed to load bitmap - failed to read image data\n");
							}
						}
						else {
							XPLMDebugString("Failed to load bitmap - ImageData->pdata was null\n");
						}
					}
					else {
						XPLMDebugString("Failed to load bitmap - header.bfSize + ...\n");
					}
				}
				else {
					XPLMDebugString("Failed to load bitmap - header is not declared as bitmap\n");
				}
			}
			else {
				XPLMDebugString("Failed to read bitmap info.\n");
			}
		}
		else {
			XPLMDebugString("Failed to read bitmap header\n");
		}
	}
	else {
		XPLMDebugString("Bitmap file was null\n");
	}
	if (BitmapFile != NULL)
		fclose(BitmapFile);
	return RetCode;
}
/// Swap the red and blue pixels.
void BmpLoader::SwapRedBlue(IMAGEDATA *ImageData)
{
	unsigned char  * srcPixel;
	int		x, y;
	unsigned char sTemp;
	unsigned char channels = ImageData->Channels;

	/// Do the swap
	srcPixel = ImageData->pData;

	for (y = 0; y < ImageData->Height; ++y) {
		for (x = 0; x < ImageData->Width; ++x)
		{
			sTemp = srcPixel[0];
			srcPixel[0] = srcPixel[2];
			srcPixel[2] = sTemp;

			srcPixel += channels;
			if (x == (ImageData->Width - 1))
				srcPixel += ImageData->Padding;
		}
	}
}