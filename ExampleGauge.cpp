#pragma warning(disable: 4996) 

/*
NOTES:
	1. 
	On windows you will need to add these to the linker/input/AdditionalDependencies settings
       glu32.lib 
	   glaux.lib

	2. 
	If you want to have the xpl go directly to the plugin directory you need to 
	set path variables. Currently I set it to build in the top directory of the 
	project.
	
	3. 
	Networking might be easier to do with UDP through the menu options as it is
	available. There are options for things like reading inputs from the network
	and also saving to the local disk. These are found under the settings menu ->
	data input and output, and network options. This is called the Data Set in 
	x-plane. info here:
	http://www.x-plane.com/manuals/desktop/#datainputandoutputfromx-plane
	http://www.x-plane.com/?article=data-set-output-table
	http://www.nuclearprojects.com/xplane/info.shtml

	Added the define IBM 1 thing because you have to specify it before doing
	// compiling. It is system specific. For Macs you must use 'define APL 1' and
	// set the ibm to 0. Info about this is here:
	// http://www.xsquawkbox.net/xpsdk/docs/macbuild.html
	//
	// Also added the header file for using the data refs. I might need to add other
	// header files for the navigation "XPLMNavigation". "XPLMDataAccess" is to
	// read plane info and set other options. Navigation has lookups for gps and 
	// fms, while the data access is the api for reading and writing plane/sensor
	// info. DataRefs:
	// http://www.xsquawkbox.net/xpsdk/docs/DataRefs.html

*/

#if IBM
#include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "XPLMDefs.h"
#include "XPLMDisplay.h"
#include "XPLMDataAccess.h"
#include "XPLMGraphics.h"
#include "XPLMUtilities.h"
#include "XPLMNavigation.h"
#include "Transponder.h"

/// Handle cross platform differences
#if IBM
#include <gl\gl.h>
#include <gl\glu.h>
#elif LIN
#define TRUE 1
#define FALSE 0
#include <GL/gl.h>
#include <GL/glu.h>
#else
#define TRUE 1
#define FALSE 0
#if __GNUC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <gl.h>
#include <glu.h>
#endif
#include <string.h>
#include <stdlib.h>
#endif

/// Cross Platform Bitmap functions and bitmap data structures
/// These need to be aligned
#pragma pack(push, ident, 2)

typedef struct tagBMPFILEHEADER
{
    short  bfType;
    int	   bfSize;
    short  bfReserved1;
    short  bfReserved2;
    int    bfOffBits;
} BMPFILEHEADER;

typedef struct tagBMPINFOHEADER
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
} BMPINFOHEADER;

typedef struct	tagIMAGEDATA
{
	unsigned char *	pData;
	int			Width;
	int			Height;
	int			Padding;
	short		Channels;
} IMAGEDATA;

#pragma pack(pop, ident)

static int		BitmapLoader(const char *FilePath, IMAGEDATA *ImageData);
static void		SwapEndian(short *Data);
static void		SwapEndian(int *Data);
static void		SwapRedBlue(IMAGEDATA *ImageData);

/// Texture stuff
#define MAX_TEXTURES 4

#define GAUGE_FILENAME			"GaugeTex256.bmp"
#define NEEDLE_FILENAME			"Needle.bmp"
#define NEEDLE_MASK_FILENAME	"NeedleMask.bmp"

#define GAUGE_TEXTURE 1
#define NEEDLE_TEXTURE 2
#define NEEDLE_TEXTURE_MASK 3


static XPLMTextureID gTexture[MAX_TEXTURES];

static XPLMDataRef	verticalSpeed = NULL;
static XPLMDataRef	RED = NULL, GREEN = NULL, BLUE = NULL;

static XPLMWindowID	gExampleGaugePanelDisplayWindow = NULL;
static int ExampleGaugeDisplayPanelWindow = 1;
static XPLMHotKeyID gExampleGaugeHotKey = NULL;

static char gPluginDataFile[255];
static float verticalSpeed1;

#if APL && __MACH__
static int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen);
#endif

/// Used for dragging plugin panel window.
static	int	CoordInRect(float x, float y, float l, float t, float r, float b);
static int	CoordInRect(float x, float y, float l, float t, float r, float b)
{	return ((x >= l) && (x < r) && (y < t) && (y >= b)); }

/// Prototypes for callbacks etc.
static void LoadTextures(void);
static int LoadGLTexture(char *pFileName, int TextureId);
static int DrawGLScene(float x, float y);
static int	ExampleGaugeDrawCallback(
                                   XPLMDrawingPhase     inPhase,
                                   int                  inIsBefore,
                                   void *               inRefcon);

static void ExampleGaugeHotKey(void * refCon);

static void ExampleGaugePanelWindowCallback(
                                   XPLMWindowID         inWindowID,
                                   void *               inRefcon);

static void ExampleGaugePanelKeyCallback(
                                   XPLMWindowID         inWindowID,
                                   char                 inKey,
                                   XPLMKeyFlags         inFlags,
                                   char                 inVirtualKey,
                                   void *               inRefcon,
                                   int                  losingFocus);

static int ExampleGaugePanelMouseClickCallback(
                                   XPLMWindowID         inWindowID,
                                   int                  x,
                                   int                  y,
                                   XPLMMouseStatus      inMouse,
                                   void *               inRefcon);

Transponder& transponder = *new Transponder;

// BEGIN STUFF I ADDED (Wesam)
static XPLMWindowID	gWindow = NULL;
static int gClicked = 0;

//Instrument data variables
float groundSpeed = 0;
float tcasBearing = 0;
float tcasDistance = 0;
float tcasAltitude = 0;
float verticalVelocity = 0;
float indAirspeed = 0;
float indAirspeed2 = 0;
float trueAirspeed = 0;
float verticalSpeedData = 0;
float latREF, lonREF = 0;

static void MyDrawWindowCallback(
	XPLMWindowID         inWindowID,
	void *               inRefcon);

static void MyHandleKeyCallback(
	XPLMWindowID         inWindowID,
	char                 inKey,
	XPLMKeyFlags         inFlags,
	char                 inVirtualKey,
	void *               inRefcon,
	int                  losingFocus);

static int MyHandleMouseClickCallback(
	XPLMWindowID         inWindowID,
	int                  x,
	int                  y,
	XPLMMouseStatus      inMouse,
	void *               inRefcon);

static void getSensorInfoPlugin(void);

// END STUFF I ADDED


PLUGIN_API int XPluginStart(
						char *		outName,
						char *		outSig,
						char *		outDesc)
{
	/// Handle cross platform differences
#if IBM
	char *pFileName = "Resources\\Plugins\\AirborneCPS\\";
#elif LIN
    char *pFileName = "Resources/plugins/AirborneCPS/";
#else
    char *pFileName = "Resources:Plugins:AirborneCPS:";
#endif
	/// Setup texture file locations
	XPLMGetSystemPath(gPluginDataFile);
	strcat(gPluginDataFile, pFileName);

	strcpy(outName, "AirborneCPS");
	strcpy(outSig, "AirborneCPS");
	strcpy(outDesc, "A plug-in for displaying a TCAS gauge.");

	//
	//START NEW MERGED CODE

	//Generate the DataRef variables.
	getSensorInfoPlugin();
	/* Now we create a window.  We pass in a rectangle in left, top,
	* right, bottom screen coordinates.  We pass in three callbacks. */
	gWindow = XPLMCreateWindow(
		50, 600, 300, 200,			/* Area of the window. */
		1,							/* Start visible. */
		MyDrawWindowCallback,		/* Callbacks */
		MyHandleKeyCallback,
		MyHandleMouseClickCallback,
		NULL);						/* Refcon - not used. */

	//END NEW MERGED CODE
	//

	/// Register so that our gauge is drawing during the Xplane gauge phase
	XPLMRegisterDrawCallback(
					ExampleGaugeDrawCallback,
					xplm_Phase_Gauges,
					0,
					NULL);

	/// Create our window, setup datarefs and register our hotkey.
	gExampleGaugePanelDisplayWindow = XPLMCreateWindow(768, 256, 1024, 0, 1, ExampleGaugePanelWindowCallback, ExampleGaugePanelKeyCallback, ExampleGaugePanelMouseClickCallback, NULL);

	verticalSpeed = XPLMFindDataRef("sim/cockpit2/gauges/indicators/vvi_fpm_pilot");

	RED = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_r");
	GREEN = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_g");
	BLUE = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_b");

	gExampleGaugeHotKey = XPLMRegisterHotKey(XPLM_VK_F8, xplm_DownFlag,   "F8",   ExampleGaugeHotKey, NULL);

	/// Load the textures and bind them etc.
	LoadTextures();
	// start broadcasting location, and listening for aircraft
	transponder.start();

	return 1;
}

PLUGIN_API void	XPluginStop(void)
{
	/// Clean up
	XPLMUnregisterDrawCallback(
					ExampleGaugeDrawCallback,
					xplm_Phase_Gauges,
					0,
					NULL);
	//
	//BEGIN NEW MERGED CODE
	
	XPLMDestroyWindow(gWindow);
	
	//END NEW MERGED CODE
	//

	XPLMUnregisterHotKey(gExampleGaugeHotKey);
	XPLMDestroyWindow(gExampleGaugePanelDisplayWindow);
}

PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(
					XPLMPluginID	inFromWho,
					int				inMessage,
					void *			inParam)
{
}



/*
 * Convert to gauge face representation
 *
 * This converts the raw vertical speed dataref to a value that can be used as 
 * the rotational degrees in the gRotatef() function for roatating the needle on the 
 * gauge.
 */
float convertToRotation(float inputFloat, float divisor) 
{
	return ((inputFloat / divisor) * 150)-90;
}

/*
 * ExampleGaugeDrawCallback
 *
 * This will draw our gauge during the Xplane gauge drawing phase.
 */
int	ExampleGaugeDrawCallback(XPLMDrawingPhase inPhase,int inIsBefore,void * inRefcon) 
{
	float FloatVal;
	// Do the actual drawing, but only if the window is active
	if (ExampleGaugeDisplayPanelWindow) {
		FloatVal = XPLMGetDataf(verticalSpeed);
		verticalSpeed1 = convertToRotation(FloatVal, 4000);
		DrawGLScene(512, 250);
	}
	return 1;
}


/*
 * ExampleGaugePanelWindowCallback
 *
 * This callback does not do any drawing as such.
 * We use the mouse callback below to handle dragging of the window
 * X-Plane will automatically do the redraw.
 *
 */
void ExampleGaugePanelWindowCallback(
                                   XPLMWindowID         inWindowID,
                                   void *               inRefcon)
{
}

/*
 * ExampleGaugePanelKeyCallback
 *
 * Our key handling callback does nothing in this plugin.  This is ok;
 * we simply don't use keyboard input.
 *
 */
void ExampleGaugePanelKeyCallback(
                                   XPLMWindowID         inWindowID,
                                   char                 inKey,
                                   XPLMKeyFlags         inFlags,
                                   char                 inVirtualKey,
                                   void *               inRefcon,
                                   int                  losingFocus)
{
}

/*
 * ExampleGaugePanelMouseClickCallback
 *
 * Our mouse click callback updates the position that the windows is dragged to.
 *
 */
int ExampleGaugePanelMouseClickCallback(
                                   XPLMWindowID         inWindowID,
                                   int                  x,
                                   int                  y,
                                   XPLMMouseStatus      inMouse,
                                   void *               inRefcon)
{
	static	int	dX = 0, dY = 0;
	static	int	Weight = 0, Height = 0;
	int	Left, Top, Right, Bottom;

	static	int	gDragging = 0;

	if (!ExampleGaugeDisplayPanelWindow)
		return 0;

	/// Get the windows current position
	XPLMGetWindowGeometry(inWindowID, &Left, &Top, &Right, &Bottom);

	switch(inMouse) {
	case xplm_MouseDown:
		/// Test for the mouse in the top part of the window
		if (CoordInRect(x, y, Left, Top, Right, Top-15))
		{
			dX = x - Left;
			dY = y - Top;
			Weight = Right - Left;
			Height = Bottom - Top;
			gDragging = 1;
		}
		break;
	case xplm_MouseDrag:
		/// We are dragging so update the window position
		if (gDragging)
		{
			Left = (x - dX);
			Right = Left + Weight;
			Top = (y - dY);
			Bottom = Top + Height;
			XPLMSetWindowGeometry(inWindowID, Left, Top, Right, Bottom);
		}
		break;
	case xplm_MouseUp:
		gDragging = 0;
		break;
	}
	return 1;
}

/// Toggle between display and non display
void ExampleGaugeHotKey(void * refCon)
{
	ExampleGaugeDisplayPanelWindow = !ExampleGaugeDisplayPanelWindow;
}

/// Loads all our textures
void LoadTextures(void)
{

	if (!LoadGLTexture(GAUGE_FILENAME, GAUGE_TEXTURE))
		XPLMDebugString("Gauge texture failed to load\n");
	if (!LoadGLTexture(NEEDLE_FILENAME, NEEDLE_TEXTURE))
		XPLMDebugString("Needle texture failed to load\n");
	if (!LoadGLTexture(NEEDLE_MASK_FILENAME, NEEDLE_TEXTURE_MASK))
		XPLMDebugString("Needle texture mask failed to load\n");

}

/// Loads one texture
int LoadGLTexture(char *pFileName, int TextureId)
{
	int Status=FALSE;
	char TextureFileName[255];
	#if APL && __MACH__
	char TextureFileName2[255];
	int Result = 0;
	#endif

	/// Need to get the actual texture path and append the filename to it.
	strcpy(TextureFileName, gPluginDataFile);
	strcat(TextureFileName, pFileName);

	#if APL && __MACH__
	Result = ConvertPath(TextureFileName, TextureFileName2, sizeof(TextureFileName));
	if (Result == 0)
		strcpy(TextureFileName, TextureFileName2);
	else
		XPLMDebugString("AirborneCPS - Unable to convert path\n");
	#endif

	void *pImageData = 0;
	int sWidth, sHeight;
	IMAGEDATA sImageData;
	/// Get the bitmap from the file
	if (BitmapLoader(TextureFileName, &sImageData))
	{
		Status=TRUE;

		SwapRedBlue(&sImageData);
		pImageData = sImageData.pData;

		/// Do the opengl stuff using XPLM functions for a friendly Xplane existence.
		sWidth=sImageData.Width;
		sHeight=sImageData.Height;
		XPLMGenerateTextureNumbers(&gTexture[TextureId], 1);
		XPLMBindTexture2d(gTexture[TextureId], 0);
	   
		GLenum type = sImageData.Channels == 4 ? GL_RGBA : GL_RGB;
		gluBuild2DMipmaps(GL_TEXTURE_2D, sImageData.Channels, sWidth, sHeight, type, GL_UNSIGNED_BYTE, pImageData);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	}
	if (pImageData != NULL)
		free(pImageData);

	return Status;
}

/// Draws the textures that make up the gauge
int DrawGLScene(float x1, float y1)
{
	float Red = XPLMGetDataf (RED);
	float Green = XPLMGetDataf (GREEN);
	float Blue = XPLMGetDataf (BLUE);
	int	PanelWindowLeft, PanelWindowRight, PanelWindowBottom, PanelWindowTop;

    float PanelWidth, PanelHeight;
    float GaugeWidth, GaugeHeight, GaugeWidthRatio, GaugeHeightRatio;
	float PanelLeft, PanelRight, PanelBottom, PanelTop;
	float GaugeLeft, GaugeRight, GaugeBottom, GaugeTop;
	float NeedleLeft, NeedleRight, NeedleBottom, NeedleTop;
	float NeedleTranslationX, NeedleTranslationY;

	/// Setup sizes for panel and gauge
    PanelWidth = 256;
    PanelHeight = 256;
	GaugeWidth = 256;
	GaugeHeight = 256;

	GaugeWidthRatio = GaugeWidth / 256.0;
    GaugeHeightRatio = GaugeHeight / 256.0;

	/// Need to find out where our window is
	XPLMGetWindowGeometry(gExampleGaugePanelDisplayWindow, &PanelWindowLeft, &PanelWindowTop, &PanelWindowRight, &PanelWindowBottom);

	/// Setup our panel and gauge relative to our window
	PanelLeft = PanelWindowLeft; PanelRight = PanelWindowRight; PanelBottom = PanelWindowBottom; PanelTop = PanelWindowTop;
	GaugeLeft = PanelWindowLeft; GaugeRight = PanelWindowRight; GaugeBottom = PanelWindowBottom; GaugeTop = PanelWindowTop;
	
	/// Setup our needle relative to the gauge
	NeedleLeft = GaugeLeft + 125.0 * GaugeWidthRatio;
    NeedleRight = NeedleLeft + 8.0 * GaugeWidthRatio;
    NeedleBottom = GaugeBottom + 120.0 * GaugeHeightRatio;
    NeedleTop = NeedleBottom + 80.0 * GaugeWidthRatio;;
    NeedleTranslationX = NeedleLeft + ((NeedleRight - NeedleLeft) / 2);
    NeedleTranslationY = NeedleBottom+(5*GaugeHeightRatio);

	/// Turn on Alpha Blending and turn off Depth Testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 1/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);

	/// Handle day/night
	glColor3f(Red, Green, Blue);

	// Draw Panel
    glPushMatrix();

	XPLMBindTexture2d(gTexture[GAUGE_TEXTURE], 0);

	glBegin(GL_QUADS);
	glTexCoord2f(0.5f, 0.0f); glVertex2f(GaugeRight, GaugeBottom);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(GaugeLeft, GaugeBottom);
	glTexCoord2f(0.0f, 0.5f); glVertex2f(GaugeLeft, GaugeTop);
	glTexCoord2f(0.5f, 0.5f); glVertex2f(GaugeRight, GaugeTop);
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_QUADS);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(GaugeRight, GaugeBottom);
	glTexCoord2f(0.5f, 0.0f); glVertex2f(GaugeLeft, GaugeBottom);
	glTexCoord2f(0.5f, 0.5f); glVertex2f(GaugeLeft, GaugeTop);
	glTexCoord2f(1.0f, 0.5f); glVertex2f(GaugeRight, GaugeTop);
	glEnd();

	// Turn on Alpha Blending and turn off Depth Testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 1/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);

    glTranslatef(NeedleTranslationX, NeedleTranslationY, 0.0f);

	if(verticalSpeed1 > 60) glRotatef(60, 0.0f , 0.0f, -1.0f);
	else if (verticalSpeed1 < -240) glRotatef(-240, 0.0f, 0.0f, -1.0f);
	else glRotatef(verticalSpeed1, 0.0f, 0.0f, -1.0f);

    glTranslatef(-NeedleTranslationX, -NeedleTranslationY, 0.0f);

    glBlendFunc(GL_DST_COLOR, GL_ZERO);

	// Draw Needle Mask
	XPLMBindTexture2d(gTexture[NEEDLE_TEXTURE_MASK], 0);
    glBegin(GL_QUADS);
			glTexCoord2f(1, 0.0f); glVertex2f(NeedleRight, NeedleBottom);	// Bottom Right Of The Texture and Quad
			glTexCoord2f(0, 0.0f); glVertex2f(NeedleLeft, NeedleBottom);	// Bottom Left Of The Texture and Quad
			glTexCoord2f(0, 1.0f); glVertex2f(NeedleLeft, NeedleTop);	// Top Left Of The Texture and Quad
			glTexCoord2f(1, 1.0f); glVertex2f(NeedleRight, NeedleTop);	// Top Right Of The Texture and Quad
	glEnd();

    glBlendFunc(GL_ONE, GL_ONE);

	// Draw Needle
	XPLMBindTexture2d(gTexture[NEEDLE_TEXTURE], 0);
	glBegin(GL_QUADS);
			glTexCoord2f(1, 0.0f); glVertex2f(NeedleRight, NeedleBottom);	// Bottom Right Of The Texture and Quad
			glTexCoord2f(0, 0.0f); glVertex2f(NeedleLeft, NeedleBottom);	// Bottom Left Of The Texture and Quad
			glTexCoord2f(0, 1.0f); glVertex2f(NeedleLeft, NeedleTop);	// Top Left Of The Texture and Quad
			glTexCoord2f(1, 1.0f); glVertex2f(NeedleRight, NeedleTop);	// Top Right Of The Texture and Quad
	glEnd();

	// Turn off Alpha Blending and turn on Depth Testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 0/*AlphaBlending*/, 1/*DepthTesting*/, 0/*DepthWriting*/);
    glPopMatrix();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glFlush();

	return TRUE;
}

#if APL && __MACH__
#include <Carbon/Carbon.h>
int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen)
{
	CFStringRef inStr = CFStringCreateWithCString(kCFAllocatorDefault, inPath ,kCFStringEncodingMacRoman);
	if (inStr == NULL)
		return -1;
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLHFSPathStyle,0);
	CFStringRef outStr = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	if (!CFStringGetCString(outStr, outPath, outPathMaxLen, kCFURLPOSIXPathStyle))
		return -1;
	CFRelease(outStr);
	CFRelease(url);
	CFRelease(inStr);
	return 0;
}
#endif


/// Cross Platform Bitmap functions
/// Functions to handle endian differeneces between windows, linux and mac.
#if APL
    short Endian(short Data)
    {
        unsigned char *pBuffer = (unsigned char *)&Data;
        short Result = (short)(pBuffer[0] & 0xff)	+ ( (short)(pBuffer[1] & 0xff) << 8) ;
  	    return(Result);
    }

    int Endian(int Data)
    {
   	    unsigned char *pBuffer = (unsigned char *)&Data;

		int Result = 		(int)(pBuffer[0] & 0xff)
						+ ( (int)(pBuffer[1] & 0xff) << 8)
						+ ( (int)(pBuffer[2] & 0xff) << 16)
						+ ( (int)(pBuffer[3] & 0xff) << 24);

		return(Result);
	}

	void SwapEndian(short *Data)
	{
		*Data = Endian(*Data);
	}

	void SwapEndian(int *Data)
	{
		*Data = Endian(*Data);
	}
#else
	/// Only the mac needs these so dummy functions for windows and linux.
	void SwapEndian(short *Data){}
	void SwapEndian(int *Data){}
#endif

/// Swap the red and blue pixels.
void SwapRedBlue(IMAGEDATA *ImageData)
{
	unsigned char  * srcPixel;
	int 	count;
	int		x,y;
	unsigned char sTemp;

	/// Does not support 4 channels.
	if (ImageData->Channels == 4)
		return;

	/// Do the swap
	srcPixel = ImageData->pData;
	count = ImageData->Width * ImageData->Height;
	for (y = 0; y < ImageData->Height; ++y)
		for (x = 0; x < ImageData->Width; ++x)
		{
			sTemp = srcPixel[0];
			srcPixel[0] = srcPixel[2];
			srcPixel[2] = sTemp;

			srcPixel += 3;
			if (x == (ImageData->Width - 1))
				srcPixel += ImageData->Padding;
		}
}


/// Generic bitmap loader to handle all platforms
int BitmapLoader(const char * FilePath, IMAGEDATA * ImageData)
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
						/*Padding = (ImageInfo.biWidth * 3 + 3) & ~3;
						Padding -= ImageInfo.biWidth * 3;*/

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
//
//BEGIN NEW MERGED CODE

/*
* MyDrawingWindowCallback
*
* This callback does the work of drawing our window once per sim cycle each time
* it is needed.  It dynamically changes the text depending on the saved mouse
* status.  Note that we don't have to tell X-Plane to redraw us when our text
* changes; we are redrawn by the sim continuously.
*
*/
void MyDrawWindowCallback(
	XPLMWindowID         inWindowID,
	void *               inRefcon)
{
	int		left, top, right, bottom;
	float	color[] = { 1.0, 1.0, 1.0 }; 	/* RGB White */

											/* Initialize the variables using the datarefs from the x-plane system.
											* These are found in the XPLMDataAccess api. */
	getSensorInfoPlugin();

	/* Getting the altitude from the dataref in X-Plane. This is just testing
	* local initialization vs the global way.
	* http://forums.x-plane.org/index.php?/forums/topic/82150-how-to-get-wind-datarefs/ */
	XPLMDataRef datarefWindAltitude0 = NULL;
	int altitudeInt0 = 0;
	datarefWindAltitude0 = XPLMFindDataRef("sim/weather/wind_altitude_msl_m[0]");
	altitudeInt0 = XPLMGetDatai(datarefWindAltitude0);

	/* First we get the location of the window passed in to us. */
	XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);

	/* We now use an XPLMGraphics routine to draw a translucent dark
	* rectangle that is our window's shape. */
	XPLMDrawTranslucentDarkBox(left, top, right, bottom);

	/* These lines convert the types to char arrays to be used as strings. */
	char verticalVelocityChar[128];
	snprintf(verticalVelocityChar, 128, "%f", verticalVelocity);

	char indAirspeedChar[128];
	snprintf(indAirspeedChar, 128, "%f", indAirspeed);

	char indAirspeed2Char[128];
	snprintf(indAirspeed2Char, 128, "%f", indAirspeed2);

	char trueAirspeedChar[128];
	snprintf(trueAirspeedChar, 128, "%f", trueAirspeed);

	char altitudeInt0Char[128];
	snprintf(altitudeInt0Char, 128, "%d", altitudeInt0);

	char verticalSpeedDataChar[128];
	snprintf(verticalSpeedDataChar, 128, "XPLMGetDataf(vvi_fpm_pilot): %f", verticalSpeedData);

	char calcVSChar[128];
	snprintf(calcVSChar, 128, "verticalSpeedCalc: %f", (verticalSpeed1+90));

	char lat[128];
	snprintf(lat, 128, "lat: %f", latREF);

	char lon[128];
	snprintf(lon, 128, "lon: %f", lonREF);




	/* Finally we draw the text into the window, also using XPLMGraphics
	* routines.  The NULL indicates no word wrapping. */
	//XPLMDrawString(color, left + 5, top - 20, (char*)(gClicked ? trueAirspeedChar : verticalVelocityChar), NULL, xplmFont_Basic);
	//(char*)(gClicked ? "Altitude (m): %d\nIndicated Airspeed: %f\nIndicated Airspeed 2: %f\nTrue Airspeed: %f", altitudeInt0Char, indAirspeedChar, indAirspeed2Char, trueAirspeedChar : verticalVelocityChar), NULL, xplmFont_Basic);
	XPLMDrawString(color, left + 5, top - 40, (char*)(verticalSpeedDataChar), NULL, xplmFont_Basic);
	XPLMDrawString(color, left + 5, top - 60, (char*)(calcVSChar), NULL, xplmFont_Basic);

	XPLMDrawString(color, left + 5, top - 80, (char*)(lat), NULL, xplmFont_Basic);
	XPLMDrawString(color, left + 5, top - 100, (char*)(lon), NULL, xplmFont_Basic);
	XPLMDrawString(color, left + 5, top - 120, transponder.msg, NULL, xplmFont_Basic);
}

/*
* MyHandleKeyCallback
*
* Our key handling callback does nothing in this plugin.  This is ok;
* we simply don't use keyboard input.
*
*/
void MyHandleKeyCallback(
	XPLMWindowID         inWindowID,
	char                 inKey,
	XPLMKeyFlags         inFlags,
	char                 inVirtualKey,
	void *               inRefcon,
	int                  losingFocus)
{
}

/*
* MyHandleMouseClickCallback
*
* Our mouse click callback toggles the status of our mouse variable
* as the mouse is clicked.  We then update our text on the next sim
* cycle.
*
*/
int MyHandleMouseClickCallback(
	XPLMWindowID         inWindowID,
	int                  x,
	int                  y,
	XPLMMouseStatus      inMouse,
	void *               inRefcon)
{
	/* If we get a down or up, toggle our status click.  We will
	* never get a down without an up if we accept the down. */
	if ((inMouse == xplm_MouseDown) || (inMouse == xplm_MouseUp))
		gClicked = 1 - gClicked;

	/* Returning 1 tells X-Plane that we 'accepted' the click; otherwise
	* it would be passed to the next window behind us.  If we accept
	* the click we get mouse moved and mouse up callbacks, if we don't
	* we do not get any more callbacks.  It is worth noting that we
	* will receive mouse moved and mouse up even if the mouse is dragged
	* out of our window's box as long as the click started in our window's
	* box. */
	return 1;
}


/*
* getSensorInfoPlugin
*
* This function is primarily used to get the information from the datarefs api.
*
*/
void getSensorInfoPlugin(void)
{
	// this should lookup the airspeed of the plane and set it as the value of
	// airSpeed.
	XPLMDataRef groundspeedDataref = NULL;
	groundspeedDataref = XPLMFindDataRef("sim/flightmodel/position/groundspeed");
	groundSpeed = XPLMGetDataf(groundspeedDataref);


	// TCAS DataRef information: bearing (degrees), float[20].
	// Relative bearing of each other plane in degrees for TCAS
	XPLMDataRef tcasBearingDataref = NULL;
	tcasBearingDataref = XPLMFindDataRef("sim/cockpit2/tcas/indicators/relative_bearing_degs");
	tcasBearing = XPLMGetDataf(tcasBearingDataref);
	// TCAS DataRef information: distance (meters), float[20]
	// Distance to each other plane in meters for TCAS
	XPLMDataRef tcasDistanceDataref = NULL;
	tcasDistanceDataref = XPLMFindDataRef("sim/cockpit2/tcas/indicators/relative_distance_mtrs");
	tcasDistance = XPLMGetDataf(tcasDistanceDataref);
	// TCAS DataRef information: altitude (meters), float[20].
	// Relative altitude (positive means above us) for TCAS
	XPLMDataRef tcasAltitudeDataref = NULL;
	tcasAltitudeDataref = XPLMFindDataRef("sim/cockpit2/tcas/indicators/relative_altitude_mtrs");
	tcasAltitude = XPLMGetDataf(tcasAltitudeDataref);

	//vertical velocity as indicated within the sims instruments.
	XPLMDataRef verticalVelocityDataref = NULL;
	verticalVelocityDataref = XPLMFindDataRef("sim/flightmodel/position/vh_ind");
	verticalVelocity = XPLMGetDataf(verticalVelocityDataref);

	//Air speed indicated - this takes into account air density and wind direction.
	XPLMDataRef indAirspeedDataref = NULL;
	indAirspeedDataref = XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed");
	indAirspeed = XPLMGetDataf(indAirspeedDataref);

	//Air speed indicated - this takes into account air density and wind direction.
	XPLMDataRef indAirspeed2Dataref = NULL;
	indAirspeed2Dataref = XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed2");
	indAirspeed2 = XPLMGetDataf(indAirspeed2Dataref);

	//Air speed true - this does not take into account air density at altitude!
	XPLMDataRef trueAirspeedDataref = NULL;
	trueAirspeedDataref = XPLMFindDataRef("sim/flightmodel/position/true_airspeed");
	trueAirspeed = XPLMGetDataf(trueAirspeedDataref);

	XPLMDataRef verticalSpeedDataref = NULL;
	verticalSpeedDataref = XPLMFindDataRef("sim/cockpit2/gauges/indicators/vvi_fpm_pilot");
	verticalSpeedData = XPLMGetDataf(verticalSpeedDataref);

	/*
	Tesing Location Detection 

	1nmi = 1852m
	30nmi = 55560m

	*/

	latREF = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/latitude"));
	lonREF = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/longitude"));
	//TESTING
}

//END NEW MERGED CODE
//
