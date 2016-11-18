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

#define TRUE 1
#define FALSE 0

#include "XPLMDefs.h"
#include "XPLMDisplay.h"
#include "XPLMDataAccess.h"
#include "XPLMNavigation.h"

#include "Transponder.h"

static XPLMDataRef verticalSpeed = NULL;
XPLMDataRef latitude_ref, longitude_ref, altitude_ref;
static XPLMDataRef	RED = NULL, GREEN = NULL, BLUE = NULL;

static XPLMWindowID	gExampleGaugePanelDisplayWindow = NULL;
static int ExampleGaugeDisplayPanelWindow = 1;
static XPLMHotKeyID gExampleGaugeHotKey = NULL;

static char gPluginDataFile[255];
static float verticalSpeed1;

Vec2 user_ac_vel = { 30.0, 30.0 };
Vec2 intr_ac_vel = { -30.0, 30.0 };

LLA user_ac_pos = { 43.0, -76.0, 10000.0, Angle::DEGREES, Distance::FEET };
LLA intr_ac_pos = { 43.6, -75.685, 10000.0, Angle::DEGREES, Distance::FEET };

Aircraft user_aircraft = {"user", user_ac_pos, user_ac_vel, 10000.0};
Aircraft test_intruder = { "intruder", intr_ac_pos, intr_ac_vel, 10000.0 };

GaugeRenderer* gauge_renderer;

concurrency::concurrent_unordered_map<std::string, Aircraft*> intruding_aircraft;
Transponder transponder = { &user_aircraft, &intruding_aircraft };

RecommendationRange pos_rec_range = {0.0, GaugeRenderer::kMaxVertSpeed_, true};
RecommendationRange neg_rec_range = {GaugeRenderer::kMinVertSpeed_, 0.0, false};

static void UpdateFromDataRefs();

/// Used for dragging plugin panel window.
static	int	CoordInRect(int x, int y, int l, int t, int r, int b);
static int	CoordInRect(int x, int y, int l, int t, int r, int b)
{	return ((x >= l) && (x < r) && (y < t) && (y >= b)); }

/// Prototypes for callbacks etc.
static void DrawGLScene();

static int	ExampleGaugeDrawCallback(XPLMDrawingPhase inPhase, int inIsBefore, void * inRefcon);
static void ExampleGaugeHotKey(void * refCon);
static void ExampleGaugePanelWindowCallback(XPLMWindowID inWindowID, void * inRefcon);
static void ExampleGaugePanelKeyCallback(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags, char inVirtualKey, void * inRefcon, int losingFocus);
static int ExampleGaugePanelMouseClickCallback(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void * inRefcon);

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
float I_latREF, I_lonREF = 0;

static void MyDrawWindowCallback(XPLMWindowID inWindowID, void * inRefcon);

static void MyHandleKeyCallback(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags, char inVirtualKey, void * inRefcon, int losingFocus);

static int MyHandleMouseClickCallback(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void * inRefcon);

PLUGIN_API int XPluginStart(char * outName, char *	outSig, char *	outDesc)
{
	XPLMDebugString("ExampleGauge::XPluginStart");
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

	/* Simple insert */
	//intruding_aircraft[test_intruder.id_] = &test_intruder;

	UpdateFromDataRefs();

	/* Now we create a window.  We pass in a rectangle in left, top, right, bottom screen coordinates.  We pass in three callbacks. */
	gWindow = XPLMCreateWindow(50, 600, 300, 200, 1, MyDrawWindowCallback, MyHandleKeyCallback, MyHandleMouseClickCallback, NULL);

	/// Register so that our gauge is drawing during the Xplane gauge phase
	XPLMRegisterDrawCallback(ExampleGaugeDrawCallback, xplm_Phase_Gauges, 0, NULL);

	/// Create our window, setup datarefs and register our hotkey.
	gExampleGaugePanelDisplayWindow = XPLMCreateWindow(768, 256, 1024, 0, 1, ExampleGaugePanelWindowCallback, ExampleGaugePanelKeyCallback, ExampleGaugePanelMouseClickCallback, NULL);

	verticalSpeed = XPLMFindDataRef("sim/cockpit2/gauges/indicators/vvi_fpm_pilot");

	latitude_ref = XPLMFindDataRef("sim/flightmodel/position/latitude");
	longitude_ref = XPLMFindDataRef("sim/flightmodel/position/longitude");
	altitude_ref = XPLMFindDataRef("sim/flightmodel/position/elevation");

	RED = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_r");
	GREEN = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_g");
	BLUE = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_b");

	gExampleGaugeHotKey = XPLMRegisterHotKey(XPLM_VK_F8, xplm_DownFlag,   "F8",   ExampleGaugeHotKey, NULL);

	/* Concurrent map usage */

	/* Simple insert */
	//intruding_aircraft[test_intruder.id_] = &test_intruder;

	/* Simple find */
	//Aircraft * aircraft = intruding_aircraft[aircraft.id_];

	/* Determining if present 
		// this line can be equivalently written auto where = ... and the compiler will figure out the type
		concurrency::concurrent_unordered_map<std::string, Aircraft*>::const_iterator where = intruding_aircraft.find(test_intruder.id_);
		
		// A value is specified as not being contained in the map if the iterator returned by find is equal to the iterator returned by end
		if (where == intruding_aircraft.end()) {
			intruding_aircraft.insert(where, std::make_pair(test_intruder.id_, &test_intruder));
		}
	*/

	//UpdateFromDataRefs();

	// Load the textures and bind them etc.
	gauge_renderer = new GaugeRenderer(gPluginDataFile, &user_aircraft, &intruding_aircraft);
	gauge_renderer->LoadTextures();

	// start broadcasting location, and listening for aircraft
	transponder.start();

	return 1;
}

PLUGIN_API void	XPluginStop(void)
{
	/// Clean up
	XPLMUnregisterDrawCallback(ExampleGaugeDrawCallback, xplm_Phase_Gauges, 0, NULL);
	XPLMDestroyWindow(gWindow);
	XPLMUnregisterHotKey(gExampleGaugeHotKey);
	XPLMDestroyWindow(gExampleGaugePanelDisplayWindow);

	delete gauge_renderer;
}

PLUGIN_API void XPluginDisable(void) {}

PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID	inFromWho, int	inMessage, void * inParam){}

/* This will draw our gauge during the Xplane gauge drawing phase. */
int	ExampleGaugeDrawCallback(XPLMDrawingPhase inPhase,int inIsBefore,void * inRefcon) 
{
	// Do the actual drawing, but only if the window is active
	if (ExampleGaugeDisplayPanelWindow) {
		LLA updated ={ Angle {XPLMGetDatad(latitude_ref), Angle::DEGREES}, 
			Angle{XPLMGetDatad(longitude_ref), Angle::DEGREES}, 
			Distance {XPLMGetDatad(altitude_ref), Distance::METERS} };
		double updated_vvel = XPLMGetDataf(verticalSpeed);

		user_aircraft.lock_.lock();

		user_aircraft.position_old_ = user_aircraft.position_current_;
		LLA current_pos = user_aircraft.position_current_;
		double current_vvel = user_aircraft.vertical_velocity_;

		user_aircraft.vertical_velocity_ = updated_vvel;
		user_aircraft.position_current_ = updated;

		LLA updated_pos_set = user_aircraft.position_current_;
		double updated_vvel_set = user_aircraft.vertical_velocity_;

		user_aircraft.lock_.unlock();

		char buf[512];
		snprintf(buf, 512, "ExampleGauge::ExampleGaugeDrawCallback - current_pos: (%f, %f), current_vvel: %f, new_pos: (%f, %f), new_vvel: %f, updated_pos: (%f, %f), updated_vvel: %f\n", current_pos.latitude_.to_degrees(), current_pos.longitude_.to_degrees(), current_vvel, updated.latitude_.to_degrees(), updated.longitude_.to_degrees(), updated_vvel, updated_pos_set.latitude_.to_degrees(), updated_pos_set.longitude_.to_degrees(), updated_vvel_set);
		XPLMDebugString(buf);

		DrawGLScene();
	}
	return 1;
}


/* This callback does not do any drawing as such.
 * We use the mouse callback below to handle dragging of the window
 * X-Plane will automatically do the redraw. */
void ExampleGaugePanelWindowCallback(XPLMWindowID inWindowID, void* inRefcon)
{
}

/* Our key handling callback does nothing in this plugin.  This is ok;
 * we simply don't use keyboard input.*/
void ExampleGaugePanelKeyCallback(XPLMWindowID inWindowID,char inKey, XPLMKeyFlags inFlags,char inVirtualKey, void * inRefcon, int losingFocus)
{
}

/* Our mouse click callback updates the position that the windows is dragged to. */
int ExampleGaugePanelMouseClickCallback(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void * inRefcon)
{
	int	dX = 0, dY = 0;
	int	Weight = 0, Height = 0;
	int	Left, Top, Right, Bottom;

	int	gDragging = 0;

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

/// Draws the textures that make up the gauge
void DrawGLScene()
{
	float rgb[3] = { XPLMGetDataf(RED), XPLMGetDataf(GREEN), XPLMGetDataf(BLUE) };
	gauge_renderer->Render(rgb, NULL, NULL);
}

/* This callback does the work of drawing our window once per sim cycle each time
* it is needed.  It dynamically changes the text depending on the saved mouse
* status.  Note that we don't have to tell X-Plane to redraw us when our text
* changes; we are redrawn by the sim continuously. */
void MyDrawWindowCallback(XPLMWindowID inWindowID, void * inRefcon)
{
	int		left, top, right, bottom;
	float	color[] = { 1.0, 1.0, 1.0 }; 	/* RGB White */
	UpdateFromDataRefs();

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

	char lat[128]; snprintf(lat, 128, "lat: %f", latREF);
	char lon[128]; snprintf(lon, 128, "lon: %f", lonREF);
	

	/* Finally we draw the text into the window, also using XPLMGraphics routines.  The NULL indicates no word wrapping. */
	XPLMDrawString(color, left + 5, top - 40, (char*)(verticalSpeedDataChar), NULL, xplmFont_Basic);
	XPLMDrawString(color, left + 5, top - 60, (char*)(calcVSChar), NULL, xplmFont_Basic);

	XPLMDrawString(color, left + 5, top - 80, (char*)(lat), NULL, xplmFont_Basic);
	XPLMDrawString(color, left + 5, top - 100, (char*)(lon), NULL, xplmFont_Basic);
	
	concurrency::concurrent_unordered_map<std::string, Aircraft*>::const_iterator & iter = intruding_aircraft.cbegin();
	int offset = 120;
	char buff[128];

	for (; iter != intruding_aircraft.cend(); ++iter) {
		Aircraft* intruder = iter->second;
		intruder->lock_.lock();

		LLA const intruder_pos = intruder->position_current_;
		intruder->lock_.unlock();

		snprintf(buff, 128, "latitude: %f, longitude: %f", intruder_pos.latitude_.to_degrees(), intruder_pos.longitude_.to_degrees());
		XPLMDrawString(color, left + 5, top - offset, (char*)buff, NULL, xplmFont_Basic);
		offset += 20;
	}
}

/* Our key handling callback does nothing in this plugin.  This is ok; we simply don't use keyboard input. */
void MyHandleKeyCallback(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags, char inVirtualKey, void * inRefcon, int losingFocus)
{
}

/*Our mouse click callback toggles the status of our mouse variable
* as the mouse is clicked.  We then update our text on the next sim
* cycle. */
int MyHandleMouseClickCallback(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void * inRefcon)
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

void UpdateFromDataRefs()
{
	/*The ground speed of the aircraft: float, meters/sec*/
	groundSpeed = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/groundspeed"));

	/*Relative bearing of each other plane in degrees for TCAS: float[20], degrees*/
	tcasBearing = XPLMGetDataf(XPLMFindDataRef("sim/cockpit2/tcas/indicators/relative_bearing_degs"));

	/*Distance to each other plane in meters for TCAS: float[20], meters*/
	tcasDistance = XPLMGetDataf(XPLMFindDataRef("sim/cockpit2/tcas/indicators/relative_distance_mtrs"));

	/*Relative altitude (positive means above us) for TCAS: float[20], meters*/
	tcasAltitude = XPLMGetDataf(XPLMFindDataRef("sim/cockpit2/tcas/indicators/relative_altitude_mtrs"));

	//Air speed indicated - this takes into account air density and wind direction.
	indAirspeed = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed"));

	//Air speed indicated - this takes into account air density and wind direction.
	indAirspeed2 = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed2"));

	//Air speed true - this does not take into account air density at altitude!
	trueAirspeed = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/true_airspeed"));

	/*Indicated vertical speed in feet per minute, pilot system: float, feet/minute*/
	verticalSpeedData = XPLMGetDataf(XPLMFindDataRef("sim/cockpit2/gauges/indicators/vvi_fpm_pilot"));

	/*The latitude of this aircraft: double, degrees*/
	latREF = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/latitude"));

	/*The longitude of this aircraft: double, degrees*/
	lonREF = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/longitude"));

	/*The latitude of the intruder aircraft: double, degrees*/
	//I_latREF = (float)intruding_aircraft["intruder"]->position_.latitude_.to_degrees();

	/*The longitude of the intruder aircraft: double, degrees*/
	//I_lonREF = (float)intruding_aircraft["intruder"]-> position_.longitude_.to_degrees();
}