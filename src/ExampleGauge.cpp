#pragma warning(disable: 4996) 

/*
NOTES:
1. On windows you will need to add these to the linker/input/AdditionalDependencies settings
glu32.lib
glaux.lib

2. If you want to have the xpl go directly to the plugin directory you need to
set path variables. Currently I set it to build in the top directory of the
project.

3. Networking might be easier to do with UDP through the menu options as it is
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

#include "XPLMDefs.h"
#include "XPLMDisplay.h"
#include "XPLMNavigation.h"
#include "XPLMDataAccess.h"

#include "component/Transponder.h"

XPLMDataRef latitudeRef, longitudeRef, altitudeRef;
XPLMDataRef headingTrueNorthDegRef, headingTrueMagDegRef;
XPLMDataRef vertSpeedRef, trueAirspeedRef, indAirspeedRef;

// These datarefs represent the RGB color of the lighting inside the cockpit
XPLMDataRef	cockpitLightingRed, cockpitLightingGreen, cockpitLightingBlue;

static XPLMWindowID	gExampleGaugePanelDisplayWindow = NULL;
static int exampleGaugeDisplayPanelWindow = 1;
static XPLMHotKeyID gExampleGaugeHotKey = NULL;

// The plugin application path
static char gPluginDataFile[255];

Aircraft* userAircraft;

GaugeRenderer* gaugeRenderer;

concurrency::concurrent_unordered_map<std::string, Aircraft*> intrudingAircraft;
concurrency::concurrent_unordered_map<std::string, ResolutionConnection*> openConnections;
Transponder* transponder;

Decider* decider;

/// Used for dragging plugin panel window.
static	int	coordInRect(int x, int y, int l, int t, int r, int b);
static int	coordInRect(int x, int y, int l, int t, int r, int b) {
	return ((x >= l) && (x < r) && (y < t) && (y >= b));
}

/// Prototypes for callbacks etc.
static void drawGLScene();

static int	gaugeDrawingCallback(XPLMDrawingPhase inPhase, int inIsBefore, void * inRefcon);
static void exampleGaugeHotKey(void * refCon);
static void exampleGaugePanelWindowCallback(XPLMWindowID inWindowID, void * inRefcon);
static void exampleGaugePanelKeyCallback(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags, char inVirtualKey, void * inRefcon, int losingFocus);
static int exampleGaugePanelMouseClickCallback(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void * inRefcon);

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
float ilatREF, ilonREF = 0;

static void myDrawWindowCallback(XPLMWindowID inWindowID, void * inRefcon);

static void myHandleKeyCallback(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags, char inVirtualKey, void * inRefcon, int losingFocus);

static int myHandleMouseClickCallback(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void * inRefcon);

PLUGIN_API int XPluginStart(char * outName, char *	outSig, char *	outDesc) {
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

	//test();

	/* Now we create a window.  We pass in a rectangle in left, top, right, bottom screen coordinates.  We pass in three callbacks. */
	gWindow = XPLMCreateWindow(50, 600, 300, 200, 1, myDrawWindowCallback, myHandleKeyCallback, myHandleMouseClickCallback, NULL);

	/// Register so that our gauge is drawing during the Xplane gauge phase
	XPLMRegisterDrawCallback(gaugeDrawingCallback, XPLM_PHASE_GAUGES, 0, NULL);

	/// Create our window, setup datarefs and register our hotkey.
	gExampleGaugePanelDisplayWindow = XPLMCreateWindow(1024, 256, 1280, 0, 1, exampleGaugePanelWindowCallback, exampleGaugePanelKeyCallback, exampleGaugePanelMouseClickCallback, NULL);

	vertSpeedRef = XPLMFindDataRef("sim/cockpit2/gauges/indicators/vvi_fpm_pilot");

	latitudeRef = XPLMFindDataRef("sim/flightmodel/position/latitude");
	longitudeRef = XPLMFindDataRef("sim/flightmodel/position/longitude");
	altitudeRef = XPLMFindDataRef("sim/flightmodel/position/elevation");

	headingTrueMagDegRef = XPLMFindDataRef("sim/flightmodel/position/mag_psi");
	headingTrueNorthDegRef = XPLMFindDataRef("sim/flightmodel/position/true_psi");

	trueAirspeedRef = XPLMFindDataRef("sim/flightmodel/position/airspeed_true");
	indAirspeedRef = XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed");

	cockpitLightingRed = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_r");
	cockpitLightingGreen = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_g");
	cockpitLightingBlue = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_b");

	gExampleGaugeHotKey = XPLMRegisterHotKey(XPLM_VK_F8, xplm_DownFlag, "F8", exampleGaugeHotKey, NULL);

	Transponder::initNetworking();
	std::string myMac = Transponder::getHardwareAddress();

	LLA currentPos = LLA::ZERO;
	userAircraft = new Aircraft(myMac, "127.0.0.1", currentPos, Angle::ZERO, Velocity::ZERO);
	std::chrono::milliseconds msSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	userAircraft->positionCurrentTime = msSinceEpoch;
	userAircraft->positionOldTime = msSinceEpoch;

	decider = new Decider(userAircraft, &openConnections);

	gaugeRenderer = new GaugeRenderer(gPluginDataFile, decider, userAircraft, &intrudingAircraft);
	gaugeRenderer->loadTextures();

	// start broadcasting location, and listening for aircraft
	transponder = new Transponder(userAircraft, &intrudingAircraft, &openConnections, decider);
	transponder->start();

	return 1;
}

PLUGIN_API void	XPluginStop(void) {
	/// Clean up
	XPLMUnregisterDrawCallback(gaugeDrawingCallback, XPLM_PHASE_GAUGES, 0, NULL);
	XPLMDestroyWindow(gWindow);
	XPLMUnregisterHotKey(gExampleGaugeHotKey);
	XPLMDestroyWindow(gExampleGaugePanelDisplayWindow);

	delete gaugeRenderer;
	delete transponder;
}

PLUGIN_API void XPluginDisable(void) {}

PLUGIN_API int XPluginEnable(void) { return 1; }

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID	inFromWho, int	inMessage, void * inParam) {}

/* The callback responsible for drawing the gauge during the X-Plane gauge drawing phase. */
int	gaugeDrawingCallback(XPLMDrawingPhase inPhase, int inIsBefore, void * inRefcon) {
	// Do the actual drawing, but only if the window is active
	if (exampleGaugeDisplayPanelWindow) {
		LLA updated = { Angle{ XPLMGetDatad(latitudeRef), Angle::AngleUnits::DEGREES },
			Angle{ XPLMGetDatad(longitudeRef), Angle::AngleUnits::DEGREES },
			Distance{ XPLMGetDatad(altitudeRef), Distance::DistanceUnits::METERS } };
		Velocity updatedVvel = Velocity(XPLMGetDataf(vertSpeedRef), Velocity::VelocityUnits::FEET_PER_MIN);
		std::chrono::milliseconds msSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		userAircraft->lock.lock();
		userAircraft->positionOld = userAircraft->positionCurrent;
		userAircraft->positionOldTime = userAircraft->positionCurrentTime;

		userAircraft->positionCurrent = updated;
		userAircraft->positionCurrentTime = msSinceEpoch;

		userAircraft->verticalVelocity = updatedVvel;
		userAircraft->heading = Angle(XPLMGetDataf(headingTrueMagDegRef), Angle::AngleUnits::DEGREES);
		userAircraft->trueAirspeed = Velocity(XPLMGetDataf(trueAirspeedRef), Velocity::VelocityUnits::METERS_PER_S);

		userAircraft->lock.unlock();

		drawGLScene();
	}
	return 1;
}


/* This callback does not do any drawing as such.
* We use the mouse callback below to handle dragging of the window
* X-Plane will automatically do the redraw. */
void exampleGaugePanelWindowCallback(XPLMWindowID inWindowID, void* inRefcon) {}

/* Our key handling callback does nothing in this plugin.  This is ok;
* we simply don't use keyboard input.*/
void exampleGaugePanelKeyCallback(XPLMWindowID inWindowID, char inKey,
	XPLMKeyFlags inFlags, char inVirtualKey, void * inRefcon, int losingFocus) {}

/* Our mouse click callback updates the position that the windows is dragged to. */
int exampleGaugePanelMouseClickCallback(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void * inRefcon) {
	int	dX = 0, dY = 0;
	int	weight = 0, height = 0;
	int	left, top, right, bottom;

	int	gDragging = 0;

	if (!exampleGaugeDisplayPanelWindow)
		return 0;

	/// Get the windows current position
	XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);

	switch (inMouse) {
	case XPLM_MOUSE_DOWN:
		/// Test for the mouse in the top part of the window
		if (coordInRect(x, y, left, top, right, top - 15)) {
			dX = x - left;
			dY = y - top;
			weight = right - left;
			height = bottom - top;
			gDragging = 1;
		}
		break;
	case XPLM_MOUSE_DRAG:
		/// We are dragging so update the window position
		if (gDragging) {
			left = (x - dX);
			right = left + weight;
			top = (y - dY);
			bottom = top + height;
			XPLMSetWindowGeometry(inWindowID, left, top, right, bottom);
		}
		break;
	case xplm_MouseUp:
		gDragging = 0;
		break;
	}
	return 1;
}

/// Toggle between display and non display
void exampleGaugeHotKey(void * refCon) {
	exampleGaugeDisplayPanelWindow = !exampleGaugeDisplayPanelWindow;
}

/// Draws the textures that make up the gauge
void drawGLScene() {
	textureconstants::GlRgb8Color cockpit_lighting = { XPLMGetDataf(cockpitLightingRed), XPLMGetDataf(cockpitLightingGreen), XPLMGetDataf(cockpitLightingBlue) };
	gaugeRenderer->render(cockpit_lighting);
}

/* This callback does the work of drawing our window once per sim cycle each time
* it is needed.  It dynamically changes the text depending on the saved mouse
* status.  Note that we don't have to tell X-Plane to redraw us when our text
* changes; we are redrawn by the sim continuously. */
/// This function draws the window that is currently being used for debug text rendering
void myDrawWindowCallback(XPLMWindowID inWindowID, void * inRefcon) {
	int		left, top, right, bottom;
	static float color[] = { 1.0, 1.0, 1.0 }; 	/* RGB White */

												/* First we get the location of the window passed in to us. */
	XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);

	/* We now use an XPLMGraphics routine to draw a translucent dark rectangle that is our window's shape. */
	XPLMDrawTranslucentDarkBox(left, top, right, bottom);

	/* Finally we draw the text into the window, also using XPLMGraphics routines.  The NULL indicates no word wrapping. */
	char positionBuf[128];
	snprintf(positionBuf, 128, "Position: (%.3f, %.3f, %.3f)", XPLMGetDataf(latitudeRef), XPLMGetDataf(longitudeRef), XPLMGetDataf(altitudeRef));
	XPLMDrawString(color, left + 5, top - 20, positionBuf, NULL, XPLM_FONT_BASIC);

	/* Drawing the LLA for each intruder aircraft in the intruding_aircraft set */
	int offsetYPxls = 40;

	for (auto & iter = intrudingAircraft.cbegin(); iter != intrudingAircraft.cend(); ++iter) {
		Aircraft* intruder = iter->second;

		intruder->lock.lock();
		LLA const intruderPos = intruder->positionCurrent;
		LLA const intruderPosOld = intruder->positionOld;
		Aircraft intrCopy = *intruder;
		intruder->lock.unlock();
		ResolutionConnection* conn = (*transponder->openConnections)[intrCopy.id];
		conn->lock.lock();
		LLA const userPosition = conn->userPosition;
		LLA const userPositionOld = conn->userPositionOld;
		std::chrono::milliseconds userPositionTime = conn->userPositionTime;
		std::chrono::milliseconds userPositionOldTime = conn->userPositionOldTime;
		conn->lock.unlock();

		double slantRangeNmi = abs(userPosition.range(&intrCopy.positionCurrent).toUnits(Distance::DistanceUnits::NMI));
		double deltaDistanceM = abs(userPositionOld.range(&intrCopy.positionOld).toUnits(Distance::DistanceUnits::METERS))
			- abs(userPosition.range(&intrCopy.positionCurrent).toUnits(Distance::DistanceUnits::METERS));
		double elapsedTimeS = (double)(intrCopy.positionCurrentTime - intrCopy.positionOldTime).count() / 1000;
		double closingSpeedKnots = Velocity(deltaDistanceM / elapsedTimeS, Velocity::VelocityUnits::METERS_PER_S).toUnits(Velocity::VelocityUnits::KNOTS);
		double altSepFt = abs(intrCopy.positionCurrent.altitude.toUnits(Distance::DistanceUnits::FEET) -
			userPosition.altitude.toUnits(Distance::DistanceUnits::FEET));
		double deltaDistance2Ft = abs(intrCopy.positionOld.altitude.toUnits(Distance::DistanceUnits::FEET) -
			userPositionOld.altitude.toUnits(Distance::DistanceUnits::FEET)) -
			abs(intrCopy.positionCurrent.altitude.toUnits(Distance::DistanceUnits::FEET) -
				userPosition.altitude.toUnits(Distance::DistanceUnits::FEET));
		double elapsedTimeMin = elapsedTimeS / 60;
		double vertClosingSpdFtPerMin = deltaDistance2Ft / elapsedTimeMin;
		double rangeTauS = slantRangeNmi / closingSpeedKnots * 3600;
		double verticalTauS = altSepFt / vertClosingSpdFtPerMin * 60;
		Velocity userVelocity = Velocity(userPosition.range(&userPositionOld).toMeters() / ((userPositionTime.count() - userPositionOldTime.count()) / 1000), Velocity::VelocityUnits::METERS_PER_S);
		Velocity intrVelocity = Velocity(intruderPos.range(&intruderPosOld).toMeters() / ((intrCopy.positionCurrentTime.count() - intrCopy.positionOldTime.count()) / 1000), Velocity::VelocityUnits::METERS_PER_S);
		Distance userDistanceByCpa = Distance(userVelocity.toMetersPerS() * rangeTauS, Distance::DistanceUnits::METERS);
		Distance intrDistanceByCpa = Distance(intrVelocity.toMetersPerS() * rangeTauS, Distance::DistanceUnits::METERS);
		LLA userPositionAtCpa = userPosition.translate(&userPositionOld.bearing(&userPosition), &userDistanceByCpa);
		LLA intrPositionAtCpa = intruderPos.translate(&intruderPosOld.bearing(&intruderPos), &intrDistanceByCpa);
		double distanceAtCpaFt = userPositionAtCpa.range(&intrPositionAtCpa).toFeet();
		double taModTauS = Decider::getModTauS(slantRangeNmi, closingSpeedKnots, Decider::getTADmodNmi(userPosition.altitude.toFeet()));
		double raModTauS = Decider::getModTauS(slantRangeNmi, closingSpeedKnots, Decider::getRADmodNmi(userPosition.altitude.toFeet()));

		positionBuf[0] = '\0';
		snprintf(positionBuf, 128, "intr_pos: (%.3f, %.3f, %3f)", intruderPos.latitude.toDegrees(), intruderPos.longitude.toDegrees(), intruderPos.altitude.toMeters());
		XPLMDrawString(color, left + 5, top - offsetYPxls, (char*)positionBuf, NULL, XPLM_FONT_BASIC);
		offsetYPxls += 20;

		if (rangeTauS > 0) {
			positionBuf[0] = '\0';
			snprintf(positionBuf, 128, "range_tau_s: %.3f", rangeTauS);
			XPLMDrawString(color, left + 5, top - offsetYPxls, (char*)positionBuf, NULL, XPLM_FONT_BASIC);
			offsetYPxls += 20;
		}

		if (taModTauS > 0) {
			positionBuf[0] = '\0';
			snprintf(positionBuf, 128, "ta_mod_tau_s: %.3f", taModTauS);
			XPLMDrawString(color, left + 5, top - offsetYPxls, (char*)positionBuf, NULL, XPLM_FONT_BASIC);
			offsetYPxls += 20;
		}

		if (raModTauS > 0) {
			positionBuf[0] = '\0';
			snprintf(positionBuf, 128, "ra_mod_tau_s: %.3f", raModTauS);
			XPLMDrawString(color, left + 5, top - offsetYPxls, (char*)positionBuf, NULL, XPLM_FONT_BASIC);
			offsetYPxls += 20;
		}

		if (verticalTauS > 0) {
			positionBuf[0] = '\0';
			snprintf(positionBuf, 128, "vertical_tau_s: %.3f", verticalTauS);
			XPLMDrawString(color, left + 5, top - offsetYPxls, (char*)positionBuf, NULL, XPLM_FONT_BASIC);
			offsetYPxls += 20;
		}

		if (rangeTauS > 0 && distanceAtCpaFt > 0) {
			positionBuf[0] = '\0';
			snprintf(positionBuf, 128, "distance_at_cpa_ft: %.3f", distanceAtCpaFt);
			XPLMDrawString(color, left + 5, top - offsetYPxls, (char*)positionBuf, NULL, XPLM_FONT_BASIC);
			offsetYPxls += 20;
		}
	}
}

/* Our key handling callback does nothing in this plugin.  This is ok; we simply don't use keyboard input. */
void myHandleKeyCallback(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags,
	char inVirtualKey, void * inRefcon, int losingFocus) {}

/*Our mouse click callback toggles the status of our mouse variable
* as the mouse is clicked.  We then update our text on the next sim
* cycle. */
int myHandleMouseClickCallback(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void * inRefcon) {
	/* If we get a down or up, toggle our status click.  We will
	* never get a down without an up if we accept the down. */
	if ((inMouse == XPLM_MOUSE_DOWN) || (inMouse == xplm_MouseUp))
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