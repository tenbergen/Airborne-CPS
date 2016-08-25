#pragma warning(disable: 4996)
/*
* HellWorld.c
*
* This plugin implements the canonical first program.  In this case, we will
* create a window that has the text hello-world in it.  As an added bonus
* the  text will change to 'This is a plugin' while the mouse is held down
* in the window.
*
* This plugin demonstrates creating a window and writing mouse and drawing
* callbacks for that window.
*
*/

// BUGS:
// currently can't figure out why it wil not run on other windows machines with
// the same architecture. 
//
// Networking might be easier to do with UDP through the menu options as it is
// available. There are options for things like reading inputs from the network
// and also saving to the local disk. These are found under the settings menu ->
// data input and output, and network options. This is called the Data Set in 
// x-plane. info here:
// http://www.x-plane.com/manuals/desktop/#datainputandoutputfromx-plane
// http://www.x-plane.com/?article=data-set-output-table
// http://www.nuclearprojects.com/xplane/info.shtml


// Added the define IBM 1 thing because you have to specify it before doing
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define APL 0
#define IBM 1
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMDataAccess.h"
#include "XPLMNavigation.h"

/*
* Global Variables.  We will store our single window globally.  We also record
* whether the mouse is down from our mouse handler.  The drawing handler looks
* at this information and draws the appropriate display.
*
*/

static XPLMWindowID	gWindow = NULL;
static int				gClicked = 0;

//Instrument data variables
float groundSpeed		= 0;
float tcasBearing		= 0;
float tcasDistance		= 0;
float tcasAltitude		= 0;
float verticalVelocity	= 0;
float indAirspeed		= 0;
float indAirspeed2		= 0;
float trueAirspeed		= 0;

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

/*
* XPluginStart
*
* Our start routine registers our window and does any other initialization we
* must do.
*
*/
PLUGIN_API int XPluginStart(
	char *		outName,
	char *		outSig,
	char *		outDesc)
{
	/* First we must fill in the passed in buffers to describe our
	* plugin to the plugin-system. */

	strcpy(outName, "Airborne Cyber Physical Systems");
	strcpy(outSig, "xplanesdk.examples.airbornecps");
	strcpy(outDesc, "Build a test and development software platform for Cyber Physical System on the example of simulated airborne safety critical systems.");

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

									/* We must return 1 to indicate successful initialization, otherwise we
									* will not be called back again. */

	return 1;
}

/*
* XPluginStop
*
* Our cleanup routine deallocates our window.
*
*/
PLUGIN_API void	XPluginStop(void)
{
	XPLMDestroyWindow(gWindow);
}

/*
* XPluginDisable
*
* We do not need to do anything when we are disabled, but we must provide the handler.
*
*/
PLUGIN_API void XPluginDisable(void)
{
}

/*
* XPluginEnable.
*
* We don't do any enable-specific initialization, but we must return 1 to indicate
* that we may be enabled at this time.
*
*/
PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

/*
* XPluginReceiveMessage
*
* We don't have to do anything in our receive message handler, but we must provide one.
*
*/
PLUGIN_API void XPluginReceiveMessage(
	XPLMPluginID	inFromWho,
	int				inMessage,
	void *			inParam)
{
}

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

	/* Finally we draw the text into the window, also using XPLMGraphics
	* routines.  The NULL indicates no word wrapping. */
	XPLMDrawString(color, left + 5, top - 20,
		(char*)(gClicked ? "Altitude (m): %d\nIndicated Airspeed: %f\nIndicated Airspeed 2: %f\nTrue Airspeed: %f", altitudeInt0Char,indAirspeedChar,indAirspeed2Char,trueAirspeedChar : verticalVelocityChar),NULL, xplmFont_Basic);

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

}
