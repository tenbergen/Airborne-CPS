#pragma once

#include <algorithm>

#include "component/Decider.h"
#include "rendering/Renderer.inc"
#include "rendering/ArtHorizTextureConstants.hpp"
#include "units/Velocity.h"
#include "util/BMPLoader.h"
#include "util/MathUtil.h"
#include "util/StringUtil.h"

class AHGaugeRenderer
{
public:
	AHGaugeRenderer(char const * const appPath, Decider * const decider, Aircraft * const userAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> * intrudingAircraft);
	~AHGaugeRenderer();

	void loadTextures();
	void render(ahtextureconstants::GlRgb8Color cockpitLighting);
	void markHostile();
	bool returnHostileValue();

	bool hostile = false;

	// The minimum and maximum vertical speed values in units of feet per minute
	static double const minVertSpeed, maxVertSpeed;

	// The clockwise degree rotation corresponding to the maximum vertical speed, with 180 degrees on a unit circle defined as 0 degrees
	static double const maxVSpeedDegrees;

private:
	// The radius of the inner circle of the gauge that contains the airplane icons in pixels
	static double const gaugeInnerCircleRadiusPxls_;
	static Distance const gaugeInnerCircleRadius_;

	static double const millisecondsPerMinute_;

	/* Parameters for drawing the recommendation rings using GluPartialDisk */
	static double const diskInnerRadius_;
	static double const diskOuterRadius_;
	static int const diskSlices_;
	static int const diskLoops_;

	/* The offset of the aircraft symbol in the inner gauge face relative to the exact center of the gauge
	proportioned to a distance. All drawing of intruding aircraft is done relative to the center of the gauge
	and not the user's position (but the center of the gauge is relative to the user's position and heading).

	This is based on the proportion of pixels the aircraft symbol has to be shifted in the y-direction (28) to
	be in the center of the gauge as a percentage of the diameter of the inner gauge ring in pixels (150 px) related
	to the distance spanned by the diameter of the gauge (radius = 30 NMI => diameter = 60 NMI) i.e. Offset = (28 / 150) * 60.
	*/
	static Distance const aircraftToGaugeCenterOffset_;

	/* The pixel positions of the window the gauge is drawn in since our window is static (does not move)*/
	static float const gaugePosLeft_, gaugePosRight_, gaugePosBot_, gaugePosTop_;
	static float const gaugeCenterX_, gaugeCenterY_;


	/*
	* outerGaugeHeight: The height of the entire gauge
	* artHorizHeight: The height of both components of the artificial horizon of the gauge
	* paddingHeight: The amount of padding between the inner and outer gauge
	* innerGaugeHeight: The height of the display window of the artificial horizon
	* gaugeCornerHeight: The height of a square used to "round" the corners of the gauge
	* */
	static float const paddingHeight;
	static float const outerGaugeHeight;
	static float const outerGaugeWidthRight;
	static float const outerGaugeWidthLeft;
	static float const artHorizHeight;
	static float const innerGaugeHeight;
	static float const gaugeCornerHeight;

	static float const gaugeViewingAngle;
	static float const separationAngleY, dyHorizon;
	static float const PI;
	static float const smallSeparationAngleTheta, largeSeparationAngleTheta;
	static float const dTheta_small, dTheta_large;
	static float const smallLineLength, largeLineLength;
	static float const lineWidth;
	static float const zeroDegreesIndicatorHeight, zeroDegreesIndicatorWidth;
	static float const planeRollIndicatorHeight, planeRollIndicatorWidth;
	static float const rvsLeft, rvsRight;
	static float const rvsTopHigh, rvsTopLow;
	static float const rvsBottomLow, rvsBottomHigh;
	static float const fpmToPixelsLeft, fpmToPixelsRight;
	static float const rvsDivisions;

	static float const altimeterLeft, altimeterRight;
	static float const altimeterTop, altimeterBottom;
	static float const altimeterDisplayLeft, altimeterDisplayMid, altimeterDisplayRight;
	static float const altimeterDisplayTopLow, altimeterDisplayTopHigh;
	static float const altimeterDisplayBottomHigh, altimeterDisplayBottomLow;
	static float const altimeterDisplayTriangleLeft, altimeterDisplayTriangleRight;
	static float const altimeterDisplayTriangleTop, altimeterDisplayTriangleMid, altimeterDisplayTriangleBottom;
	static float const altimeterRange;
	static float const altimeterDivisions;
	static float const da;
	static float const altToPixels;
	static float const dyAltimeter;

	static float const airspeedGaugeLeft, airspeedGaugeRight;
	static float const airspeedGaugeTop, airspeedGaugeBottom;
	static float const airspeedGaugeDisplayLeft, airspeedGaugeDisplayMid, airspeedGaugeDisplayRight;
	static float const airspeedGaugeDisplayTopLow, airspeedGaugeDisplayTopHigh;
	static float const airspeedGaugeDisplayBottomHigh, airspeedGaugeDisplayBottomLow;
	static float const airspeedGaugeDisplayTriangleLeft, airspeedGaugeDisplayTriangleRight;
	static float const airspeedGaugeDisplayTriangleTop, airspeedGaugeDisplayTriangleMid, airspeedGaugeDisplayTriangleBottom;
	static float const airspeedGaugeRange;
	static float const airspeedGaugeDivisions;
	static float const ds;
	static float const airspeedToPixels;
	static float const dyAirspeedGauge;



	static Distance const advisoryRadiusNMi;
	static float const advisoryRadiusPixels;
	static float const advisoryViewAngleX;
	static float const advisoryViewAngleY;
	static float const advisoryDomainX;
	static float const advisoryDomainY;
	static float const advisoryPixelsX;
	static float const advisoryPixelsY;
	static float const advisoryFeetToPixelsX;
	static float const advisoryFeetToPixelsY;




	/* The absolute pixel positions of the needle (determined relative to the gauge positions). */
	static float const needlePosLeft_, needlePosRight_, needlePosBot_, needlePosTop_;
	/* The translation that is applied to the needle in order to allow for rotation about the gauge center.*/
	static float const needleTranslationX_, needleTranslationY_;

	/* The offset that must be applied to account for OpenGL functions treating the +z axis
	(90 degrees on a unit circle) as 0 degrees. */
	static float const glAngleOffset_;

	// The "application path", which for the plugin is the directory that the plugin is contained in
	char const * const appPath_;

	Decider * const decider_;
	Aircraft * const userAircraft_;
	// The map of intruding aircraft
	concurrency::concurrent_unordered_map<std::string, Aircraft*> * const intruders_;

	// An OpenGL quadric (quadratic) object required for use with the GLUT library's partial disk function.
	GLUquadricObj* quadric_;

	XPLMTextureID glTextures_[ahtextureconstants::K_NUM_TEXTURES];

	static ahtextureconstants::TexCoords const * aircraftSymbolFromThreatClassification(Aircraft::ThreatClassification threatClass);
	static ahtextureconstants::GlRgb8Color const * AHGaugeRenderer::symbolColorFromThreatClassification(Aircraft::ThreatClassification threatClass);

	bool loadTexture(char * texPath, int texId) const;

	/* Draws the Artificial Horizon*/
	void drawHorizon(Angle theta, Angle phi) const;
	/* Draws the outer gauge ring.*/
	void drawOuterGauge() const;
	/* Draw the inner vertical speed gauge rings. */
	void drawInnerGaugeVelocityRing() const;
	/* Draws the vertical speed indicator needle on the gauge from the supplied vertical velocity.*/
	void drawVerticalVelocityNeedle(Velocity const userAircraftVertVel) const;
	/*  */
	void drawIntrudingAircraft(LLA const * const intruderPos, Velocity const * const intruderVvel, Angle const * const userHeading, LLA const * const gaugeCenterPos, Distance const * const range, Aircraft::ThreatClassification threatClass) const;


	/* Draws the recommended vertical speed gauge indicating a recommended speed based on the range provided. */
	void drawRecommendedVerticalSpeedGauge(Velocity verticalVelocity) const;
	/* Draws the background of the vertical speed gauge */
	void drawVerticalSpeedGaugeBackground() const;
	/* Draws the supplied recommendation range  as either recommended (green) or not recommended (red). */
	void drawRecommendationRange(RecommendationRange* recommendationRange, bool recommended) const;
	/* Draws the lines indicating the divisions of the vertical velocity gauge, separated by dv. */
	void drawVerticalSpeedGaugeGraduations() const;
	/* Draws the needle indicating the aircraft's vertical velocity. */
	void drawVerticalSpeedNeedle(Velocity verticalVelocity) const;

	/*  */
	void drawAltimeter(float currentAltitude) const;
	/*  */
	void drawAltimeterBackground() const;
	/*  */
	void drawAltimeterGraduations(float currentAltitude) const;
	/*  */
	void drawAltitudeIndicator(float currentAltitude) const;
	/*  */
	void writeAltitude(float currentAltitude) const;

	/*  */
	void drawAirspeedGauge(float currentAirspeed) const;
	/*  */
	void drawAirspeedGaugeBackground() const;
	/*  */
	void drawAirspeedGaugeGraduations(float currentAirspeed) const;
	/*  */
	void drawAirspeedGaugeIndicator(float currentAirspeed) const;
	/*  */
	void writeAirspeed(float currentAirspeed) const;

	/*  */
	void writeNumber(int number, double borderPositionX, double centerPositionY, bool leftBorder) const;


	void drawTextureRegion(ahtextureconstants::TexCoords const * texCoords, double vertLeft, double vertRight, double vertTop, double vertBot) const;


	ahtextureconstants::TexCoords const * AHGaugeRenderer::gaugeTexCoordsFromDigitCharacter(char) const;

	// No copy constructor or copy-assignment
	AHGaugeRenderer(const AHGaugeRenderer& that) = delete;
	AHGaugeRenderer& operator=(const AHGaugeRenderer& that) = delete;
};