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

void str_util::BuildFilePath(char * const in_buffer, char const * const tex_fname, char const * const plugin_path) {
	in_buffer[0] = '\0';
	strcat(in_buffer, plugin_path);
	strcat(in_buffer, tex_fname);

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