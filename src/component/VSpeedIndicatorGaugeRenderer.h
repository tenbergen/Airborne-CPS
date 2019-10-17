#pragma once

#include <algorithm>

#include "component/Decider.h"
#include "rendering/Renderer.inc"
#include "rendering/VSpeedIndicatorTextureConstants.hpp"
#include "util/BMPLoader.h"
#include "util/MathUtil.h"
#include "util/StringUtil.h"

// @author nstemmle

class VSIGaugeRenderer
{
public:
	VSIGaugeRenderer(char const * const appPath, Decider * const decider, Aircraft * const userAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> * intrudingAircraft);
	~VSIGaugeRenderer();

	void loadTextures();
	void render(vsitextureconstants::GlRgb8Color cockpitLighting);
	void markHostile();
	bool returnHostileValue();

	bool hostile = false;

	// The minimum and maximum vertical speed values in units of feet per minute
	static double const kMinVertSpeed, kMaxVertSpeed;

	// The clockwise degree rotation corresponding to the maximum vertical speed, with 180 degrees on a unit circle defined as 0 degrees
	static double const kMaxVSpeedDegrees;

private:
	// The radius of the inner circle of the gauge that contains the airplane icons in pixels
	static double const kGaugeInnerCircleRadiusPxls_;
	static Distance const kGaugeInnerCircleRadius_;

	static double const kMillisecondsPerSecond_;

	/* Parameters for drawing the recommendation rings using GluPartialDisk */
	static double const kDiskInnerRadius_;
	static double const kDiskOuterRadius_;
	static int const kDiskSlices_;
	static int const kDiskLoops_;

	/* The offset of the aircraft symbol in the inner gauge face relative to the exact center of the gauge
	proportioned to a distance. All drawing of intruding aircraft is done relative to the center of the gauge
	and not the user's position (but the center of the gauge is relative to the user's position and heading).

	This is based on the proportion of pixels the aircraft symbol has to be shifted in the y-direction (28) to
	be in the center of the gauge as a percentage of the diameter of the inner gauge ring in pixels (150 px) related 
	to the distance spanned by the diameter of the gauge (radius = 30 NMI => diameter = 60 NMI) i.e. Offset = (28 / 150) * 60.
	*/
	static Distance const kAircraftToGaugeCenterOffset_;

	/* The pixel positions of the window the gauge is drawn in since our window is static (does not move)*/
	static float const kGaugePosLeft_, kGaugePosRight_, kGaugePosBot_, kGaugePosTop_;
	static float const kGaugeCenterX_, kGaugeCenterY_;
	
	/* The absolute pixel positions of the needle (determined relative to the gauge positions). */
	static float const kNeedlePosLeft_, kNeedlePosRight_, kNeedlePosBot_, kNeedlePosTop_;
	/* The translation that is applied to the needle in order to allow for rotation about the gauge center.*/
	static float const kNeedleTranslationX_, kNeedleTranslationY_;

	/* The offset that must be applied to account for OpenGL functions treating the +z axis 
	(90 degrees on a unit circle) as 0 degrees. */
	static float const kGlAngleOffset_;

	// The "application path", which for the plugin is the directory that the plugin is contained in
	char const * const appPath_;
	
	Decider * const decider_;
	Aircraft * const userAircraft_;
	// The map of intruding aircraft
	concurrency::concurrent_unordered_map<std::string, Aircraft*> * const intruders_;

	// An OpenGL quadric (quadratic) object required for use with the GLUT library's partial disk function.
	GLUquadricObj* quadric_;

	XPLMTextureID glTextures_[vsitextureconstants::K_NUM_TEXTURES];

	static vsitextureconstants::TexCoords const * aircraftSymbolFromThreatClassification(Aircraft::ThreatClassification threatClass);
	static vsitextureconstants::GlRgb8Color const * VSIGaugeRenderer::symbolColorFromThreatClassification(Aircraft::ThreatClassification threatClass);

	bool loadTexture(char * texPath, int texId) const;

	/* Draws the outer gauge ring.*/
	void drawOuterGauge() const;
	/* Draw the inner vertical speed gauge rings. */
	void drawInnerGaugeVelocityRing() const;
	/* Draws the vertical speed indicator needle on the gauge from the supplied vertical velocity.*/
	void drawVerticalVelocityNeedle(Velocity const userAircraftVertVel) const;
	/*  */
	void drawIntrudingAircraft(LLA const * const intruderPos, Velocity const * const intruderVvel, Angle const * const userHeading, LLA const * const gaugeCenterPos, Distance const * const range, Aircraft::ThreatClassification threatClass) const;

	/* Draws the supplied recommendation range  as either recommended (green) or not recommended (red). */
	void drawRecommendationRange(RecommendationRange* recRange, bool recommended) const;
	/* Draws the supplied vertical speed range as either recommended (green) or not recommended (red). */
	void drawRecommendedVerticalSpeedRange(Velocity minVertSpeed, Velocity maxVertSpeed, bool recommended) const;
	/* Draws the supplied degree range as either recommended (green) or not recommended (red).*/
	void drawRecommendationRangeStartStop(Angle start, Angle stop, bool recommended) const;
	/* Draws a recommendation range starting at the supplied start angle in a sweep_angle degrees arc.*/
	void drawRecommendationRangeStartSweep(Angle start, Angle sweep, bool recommended) const;

	void drawTextureRegion(vsitextureconstants::TexCoords const * texCoords, double vertLeft, double vertRight, double vertTop, double vertBot) const;


	vsitextureconstants::TexCoords const * VSIGaugeRenderer::gaugeTexCoordsFromDigitCharacter(char) const;

	// No copy constructor or copy-assignment 
	VSIGaugeRenderer (const VSIGaugeRenderer& that) = delete;
	VSIGaugeRenderer& operator=(const VSIGaugeRenderer& that) = delete;
};