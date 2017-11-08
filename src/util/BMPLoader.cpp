#include "BMPLoader.h"

int BmpLoader::loadBmp(const char * filePath, ImageData * imageData)
{
	char debugStringBuf[256];
	sprintf(debugStringBuf, "ExampleGuage::BitmapLoader - FilePath: %s\n", filePath);
	XPLMDebugString(debugStringBuf);
	BmpFileHeader   header;
	BmpInfoHeader	imageInfo;
	int						padding;
	FILE *					bitmapFile = NULL;
	int retCode = 0;

	imageData->pData = NULL;

	bitmapFile = fopen(filePath, "rb");
	if (bitmapFile != NULL)
	{
		if (fread(&header, sizeof(header), 1, bitmapFile) == 1)
		{
			if (fread(&imageInfo, sizeof(imageInfo), 1, bitmapFile) == 1)
			{
				/// Handle Header endian.
				swapEndian(&header.bfSize);
				swapEndian(&header.bfOffBits);

				/// Handle ImageInfo endian.
				swapEndian(&imageInfo.biWidth);
				swapEndian(&imageInfo.biHeight);
				swapEndian(&imageInfo.biBitCount);

				short channels = imageInfo.biBitCount / 8;

				/// Make sure that it is a bitmap.
#if APL && defined(__POWERPC__)
				if (((Header.bfType & 0xff) == 'M') &&
					(((Header.bfType >> 8) & 0xff) == 'B') &&
#else
				if (((header.bfType & 0xff) == 'B') &&
					(((header.bfType >> 8) & 0xff) == 'M') &&
#endif
					(imageInfo.biBitCount == 24 || imageInfo.biBitCount == 32) &&
					(imageInfo.biWidth > 0) &&
					(imageInfo.biHeight > 0))
				{
					/// "Header.bfSize" does not always agree
					/// with the actual file size and can sometimes be "ImageInfo.biSize"	 smaller.
					/// So add it in for good measure
					if ((header.bfSize + imageInfo.biSize - header.bfOffBits) >= (imageInfo.biWidth * imageInfo.biHeight * channels))
					{
						padding = (imageInfo.biWidth * channels + channels) & ~channels;
						padding -= imageInfo.biWidth * channels;

						imageData->width = imageInfo.biWidth;
						imageData->height = imageInfo.biHeight;
						imageData->padding = padding;

						/// Allocate memory for the actual image.
						imageData->channels = channels;
						imageData->pData = (unsigned char *)malloc(imageInfo.biWidth * imageInfo.biHeight * imageData->channels + imageInfo.biHeight * padding);

						if (imageData->pData != NULL)
						{
							/// Get the actual image.
							if (fread(imageData->pData, imageInfo.biWidth * imageInfo.biHeight * imageData->channels + imageInfo.biHeight * padding, 1, bitmapFile) == 1)
							{
								retCode = 1;
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
	if (bitmapFile != NULL)
		fclose(bitmapFile);
	return retCode;
}
/// Swap the red and blue pixels.
void BmpLoader::swapRedBlue(ImageData *imageData)
{
	unsigned char  * srcPixel;
	int		x, y;
	unsigned char sTemp;
	short channels = imageData->channels;

	/// Do the swap
	srcPixel = imageData->pData;

	for (y = 0; y < imageData->height; ++y) {
		for (x = 0; x < imageData->width; ++x)
		{
			sTemp = srcPixel[0];
			srcPixel[0] = srcPixel[2];
			srcPixel[2] = sTemp;

			srcPixel += channels;
			if (x == (imageData->width - 1))
				srcPixel += imageData->padding;
		}
	}
}