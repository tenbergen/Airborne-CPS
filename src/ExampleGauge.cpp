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
#include "XPLMDataAccess.h"
#include "XPLMNavigation.h"

#include "component/Transponder.h"

XPLMDataRef latitude_ref, longitude_ref, altitude_ref;
XPLMDataRef heading_true_north_deg_ref, heading_true_mag_deg_ref;
XPLMDataRef vert_speed_ref, true_airspeed_ref, ind_airspeed_ref;

// These datarefs represent the RGB color of the lighting inside the cockpit
XPLMDataRef	cockpit_lighting_red, cockpit_lighting_green, cockpit_lighting_blue;

static XPLMWindowID	gExampleGaugePanelDisplayWindow = NULL;
static int ExampleGaugeDisplayPanelWindow = 1;
static XPLMHotKeyID gExampleGaugeHotKey = NULL;

// The plugin application path
static char gPluginDataFile[255];

Vec2 user_ac_vel = { 0.0, 1.0 };
Vec2 intr_ac_vel = { -30.0, 30.0 };

LLA user_ac_pos = { 43.0, -76.0, 10000.0, Angle::AngleUnits::DEGREES, Distance::DistanceUnits::FEET };

/// Relative to test gauge center position
//LLA intr_ac_pos_ne = { 43.362, -75.757, 10000.0, Angle::AngleUnits::DEGREES, Distance::DistanceUnits::FEET }; // Bearing = 44.072
//LLA intr_ac_pos_se = { 43.009, -75.758, 10000.0, Angle::AngleUnits::DEGREES, Distance::DistanceUnits::FEET }; // Bearing = 135.515
//LLA intr_ac_pos_sw = { 43.009, -76.242, 10000.0, Angle::AngleUnits::DEGREES, Distance::DistanceUnits::FEET }; // Bearing = 224.485
//LLA intr_ac_pos_nw = { 43.362, -76.243, 10000.0, Angle::AngleUnits::DEGREES, Distance::DistanceUnits::FEET }; // Bearing = 315.928

/// Relative to test position
LLA intr_ac_pos_ne = { 43.153, -75.790, 11200.0, Angle::AngleUnits::DEGREES, Distance::DistanceUnits::FEET }; // Bearing = 44.072
LLA intr_ac_pos_nw = { 43.153, -76.210, 12100.0, Angle::AngleUnits::DEGREES, Distance::DistanceUnits::FEET }; // Bearing = 315.928
LLA intr_ac_pos_se = { 42.846, -75.791, 8900.0, Angle::AngleUnits::DEGREES, Distance::DistanceUnits::FEET }; // Bearing = 135.515
LLA intr_ac_pos_sw = { 42.846, -76.209, 7800.0, Angle::AngleUnits::DEGREES, Distance::DistanceUnits::FEET }; // Bearing = 224.485

LLA intr_ac_pos_n = {43.2, -76.0, 10000.0, Angle::AngleUnits::DEGREES, Distance::DistanceUnits::FEET};

Velocity test_vvel = {1000.0, Velocity::VelocityUnits::FEET_PER_MIN};

Aircraft* user_aircraft;

Aircraft test_intr_ne = { "intruder_ne", "192.168.1.1", intr_ac_pos_ne, Angle::ZERO, test_vvel };
Aircraft test_intr_nw = { "intruder_nw", "192.168.1.1", intr_ac_pos_nw, Angle::ZERO, test_vvel };
Aircraft test_intr_se = { "intruder_se", "192.168.1.1", intr_ac_pos_se, Angle::ZERO, test_vvel };
Aircraft test_intr_sw = { "intruder_sw", "192.168.1.1", intr_ac_pos_sw, Angle::ZERO, test_vvel };
Aircraft test_intr_n = {"intruder_n", "192.168.1.1", intr_ac_pos_n, Angle::ZERO, test_vvel};

GaugeRenderer* gauge_renderer;

concurrency::concurrent_unordered_map<std::string, Aircraft*> intruding_aircraft;
concurrency::concurrent_unordered_map<std::string, ResolutionConnection*> open_connections;
Transponder* transponder;

Decider* decider;

/// Used for dragging plugin panel window.
static	int	CoordInRect(int x, int y, int l, int t, int r, int b);
static int	CoordInRect(int x, int y, int l, int t, int r, int b) {	
	return ((x >= l) && (x < r) && (y < t) && (y >= b)); 
}

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

void test() {
	intruding_aircraft[test_intr_ne.id_] = &test_intr_ne;
	intruding_aircraft[test_intr_nw.id_] = &test_intr_nw;
	intruding_aircraft[test_intr_sw.id_] = &test_intr_sw;
	intruding_aircraft[test_intr_se.id_] = &test_intr_se;
	//intruding_aircraft[test_intr_n.id_] = &test_intr_n;

	user_aircraft->lock_.lock();
				 
	user_aircraft->heading_ = Angle::k0Degrees_;
	user_aircraft->position_current_ = user_ac_pos;
	user_aircraft->position_old_ = user_ac_pos;
	user_aircraft->position_old_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	user_aircraft->position_current_time_ = user_aircraft->position_old_time_;
				 
	user_aircraft->lock_.unlock();

	test_intr_ne.threat_classification_ = Aircraft::ThreatClassification::NON_THREAT_TRAFFIC;
	test_intr_nw.threat_classification_ = Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC;
	test_intr_se.threat_classification_ = Aircraft::ThreatClassification::TRAFFIC_ADVISORY;
	test_intr_sw.threat_classification_ = Aircraft::ThreatClassification::RESOLUTION_ADVISORY;
}

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
	gWindow = XPLMCreateWindow(50, 600, 300, 200, 1, MyDrawWindowCallback, MyHandleKeyCallback, MyHandleMouseClickCallback, NULL);

	/// Register so that our gauge is drawing during the Xplane gauge phase
	XPLMRegisterDrawCallback(ExampleGaugeDrawCallback, xplm_Phase_Gauges, 0, NULL);

	/// Create our window, setup datarefs and register our hotkey.
	gExampleGaugePanelDisplayWindow = XPLMCreateWindow(1024, 256, 1280, 0, 1, ExampleGaugePanelWindowCallback, ExampleGaugePanelKeyCallback, ExampleGaugePanelMouseClickCallback, NULL);

	vert_speed_ref = XPLMFindDataRef("sim/cockpit2/gauges/indicators/vvi_fpm_pilot");

	latitude_ref = XPLMFindDataRef("sim/flightmodel/position/latitude");
	longitude_ref = XPLMFindDataRef("sim/flightmodel/position/longitude");
	altitude_ref = XPLMFindDataRef("sim/flightmodel/position/elevation");

	heading_true_mag_deg_ref = XPLMFindDataRef("sim/flightmodel/position/mag_psi");
	heading_true_north_deg_ref = XPLMFindDataRef("sim/flightmodel/position/true_psi");

	true_airspeed_ref = XPLMFindDataRef("sim/flightmodel/position/airspeed_true");
	ind_airspeed_ref = XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed");

	cockpit_lighting_red = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_r");
	cockpit_lighting_green = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_g");
	cockpit_lighting_blue = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_b");

	gExampleGaugeHotKey = XPLMRegisterHotKey(XPLM_VK_F8, xplm_DownFlag,   "F8",   ExampleGaugeHotKey, NULL);

	Transponder::initNetworking();
	std::string my_mac = Transponder::getHardwareAddress();

	LLA current_pos = { XPLMGetDatad(latitude_ref), XPLMGetDatad(longitude_ref), 
		XPLMGetDatad(altitude_ref), Angle::AngleUnits::DEGREES, Distance::DistanceUnits::METERS };
	user_aircraft = new Aircraft(my_mac, "127.0.0.1", current_pos, Angle::ZERO, Velocity::ZERO);

	decider = new Decider(user_aircraft, &open_connections);

	gauge_renderer = new GaugeRenderer(gPluginDataFile, decider, user_aircraft, &intruding_aircraft);
	gauge_renderer->LoadTextures();

	// start broadcasting location, and listening for aircraft
	transponder = new Transponder(user_aircraft, &intruding_aircraft, &open_connections, decider);
	transponder->start();

	return 1;
}

PLUGIN_API void	XPluginStop(void) {
	/// Clean up
	XPLMUnregisterDrawCallback(ExampleGaugeDrawCallback, xplm_Phase_Gauges, 0, NULL);
	XPLMDestroyWindow(gWindow);
	XPLMUnregisterHotKey(gExampleGaugeHotKey);
	XPLMDestroyWindow(gExampleGaugePanelDisplayWindow);
	XPLMDebugString("ExampleGauge::XPluginStop - destroyed gExampleGaugePanelDisplayWindow\n");

	delete gauge_renderer;
	XPLMDebugString("ExampleGauge::XPluginStop - destroyed gauge_renderer\n");
	delete transponder;
	XPLMDebugString("ExampleGauge::XPluginStop - destroyed transponder\n");
}

PLUGIN_API void XPluginDisable(void) {}

PLUGIN_API int XPluginEnable(void) { return 1; }

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID	inFromWho, int	inMessage, void * inParam){}

/* This will draw our gauge during the Xplane gauge drawing phase. */
int	ExampleGaugeDrawCallback(XPLMDrawingPhase inPhase,int inIsBefore,void * inRefcon) {
	// Do the actual drawing, but only if the window is active
	if (ExampleGaugeDisplayPanelWindow) {
		LLA updated ={ Angle {XPLMGetDatad(latitude_ref), Angle::AngleUnits::DEGREES}, 
			Angle{XPLMGetDatad(longitude_ref), Angle::AngleUnits::DEGREES}, 
			Distance {XPLMGetDatad(altitude_ref), Distance::DistanceUnits::METERS} };
		Velocity updated_vvel = Velocity(XPLMGetDataf(vert_speed_ref), Velocity::VelocityUnits::FEET_PER_MIN);
		std::chrono::milliseconds ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

		user_aircraft->lock_.lock();
		user_aircraft->position_old_ = user_aircraft->position_current_;
		user_aircraft->position_old_time_ = user_aircraft->position_current_time_;

		user_aircraft->position_current_ = updated;
		user_aircraft->position_current_time_ = ms_since_epoch;

		user_aircraft->vertical_velocity_ = updated_vvel;
		user_aircraft->heading_ = Angle(XPLMGetDataf(heading_true_mag_deg_ref), Angle::AngleUnits::DEGREES);
		user_aircraft->true_airspeed_ = Velocity(XPLMGetDataf(true_airspeed_ref), Velocity::VelocityUnits::METERS_PER_S);
		
		user_aircraft->lock_.unlock();

		DrawGLScene();
	}
	return 1;
}


/* This callback does not do any drawing as such.
 * We use the mouse callback below to handle dragging of the window
 * X-Plane will automatically do the redraw. */
void ExampleGaugePanelWindowCallback(XPLMWindowID inWindowID, void* inRefcon){}

/* Our key handling callback does nothing in this plugin.  This is ok;
 * we simply don't use keyboard input.*/
void ExampleGaugePanelKeyCallback(XPLMWindowID inWindowID,char inKey, 
	XPLMKeyFlags inFlags,char inVirtualKey, void * inRefcon, int losingFocus){}

/* Our mouse click callback updates the position that the windows is dragged to. */
int ExampleGaugePanelMouseClickCallback(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void * inRefcon){
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
		if (CoordInRect(x, y, Left, Top, Right, Top-15)){
			dX = x - Left;
			dY = y - Top;
			Weight = Right - Left;
			Height = Bottom - Top;
			gDragging = 1;
		}
		break;
	case xplm_MouseDrag:
		/// We are dragging so update the window position
		if (gDragging) {
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
void ExampleGaugeHotKey(void * refCon) {
	ExampleGaugeDisplayPanelWindow = !ExampleGaugeDisplayPanelWindow;
}

/// Draws the textures that make up the gauge
void DrawGLScene() {
	texture_constants::GlRgb8Color cockpit_lighting = { XPLMGetDataf(cockpit_lighting_red), XPLMGetDataf(cockpit_lighting_green), XPLMGetDataf(cockpit_lighting_blue) };
	gauge_renderer->Render(cockpit_lighting);
}

/* This callback does the work of drawing our window once per sim cycle each time
* it is needed.  It dynamically changes the text depending on the saved mouse
* status.  Note that we don't have to tell X-Plane to redraw us when our text
* changes; we are redrawn by the sim continuously. */
// This function draws the window that is currently being used for debug text rendering
void MyDrawWindowCallback(XPLMWindowID inWindowID, void * inRefcon) {
	int		left, top, right, bottom;
	static float color[] = { 1.0, 1.0, 1.0 }; 	/* RGB White */

	/* First we get the location of the window passed in to us. */
	XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);

	/* We now use an XPLMGraphics routine to draw a translucent dark rectangle that is our window's shape. */
	XPLMDrawTranslucentDarkBox(left, top, right, bottom);
	
	/* Finally we draw the text into the window, also using XPLMGraphics routines.  The NULL indicates no word wrapping. */
	char position_buf[128];
	snprintf(position_buf, 128, "Position: (%.3f, %.3f, %.3f)", XPLMGetDataf(latitude_ref), XPLMGetDataf(longitude_ref), XPLMGetDataf(altitude_ref));
	XPLMDrawString(color, left + 5, top - 20, position_buf, NULL, xplmFont_Basic);

	/* Drawing the LLA for each intruder aircraft in the intruding_aircraft set */
	int offset_y_pxls = 40;

	for (auto & iter = intruding_aircraft.cbegin(); iter != intruding_aircraft.cend(); ++iter) {
		Aircraft* intruder = iter -> second;

		intruder->lock_.lock();
		LLA const intruder_pos = intruder -> position_current_;
		intruder->lock_.unlock();

		position_buf[0] = '\0';
		snprintf(position_buf, 128, "intr_pos: (%.3f, %.3f)", intruder_pos.latitude_.to_degrees(), intruder_pos.longitude_.to_degrees());
		XPLMDrawString(color, left + 5, top - offset_y_pxls, (char*)position_buf, NULL, xplmFont_Basic);
		offset_y_pxls += 20;
	}
}

/* Our key handling callback does nothing in this plugin.  This is ok; we simply don't use keyboard input. */
void MyHandleKeyCallback(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags, 
	char inVirtualKey, void * inRefcon, int losingFocus){}

/*Our mouse click callback toggles the status of our mouse variable
* as the mouse is clicked.  We then update our text on the next sim
* cycle. */
int MyHandleMouseClickCallback(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void * inRefcon){
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