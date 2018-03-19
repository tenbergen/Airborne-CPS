#include "StringUtil.h"

#if APL && __MACH__
#include <Carbon/Carbon.h>
int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen)
{
	CFStringRef inStr = CFStringCreateWithCString(kCFAllocatorDefault, inPath, kCFStringEncodingMacRoman);
	if (inStr == NULL)
		return 0;
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLHFSPathStyle, 0);
	CFStringRef outStr = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	if (!CFStringGetCString(outStr, outPath, outPathMaxLen, kCFURLPOSIXPathStyle))
		return 0;
	CFRelease(outStr);
	CFRelease(url);
	CFRelease(inStr);
	return 1;
}
#endif

void strutil::buildFilePath(char * const inBuffer, char const * const texFname, char const * const pluginPath) {
	inBuffer[0] = '\0';
	strcat(inBuffer, pluginPath);
	strcat(inBuffer, texFname);

#if APL && __MACH__
	char catBuf2[255];
	if (ConvertPath(catBuf, catBuf2, sizeof(catBuf)))
		strcpy(catBuf, catBuf2);
	else {
		XPLMDebugString("AirborneCPS - Unable to convert path\n");
		catBuf[0] = '\0';
	}
#endif
}