#pragma once

#include <algorithm>

#include "Decider.h"
#include "BMPLoader.h"
#include "Renderer.inc"

#include "MathUtil.h"
#include "TextureConstants.hpp"
#include "StringUtil.h"

// @author nstemmle
class GaugeRenderer
{
public:
	GaugeRenderer(char const * const app_path, Decider * const decider, Aircraft * const user_aircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> * intruding_aircraft);
	~GaugeRenderer();

	void LoadTextures();
	void Render(texture_constants::GlRgb8Color cockpit_lighting);

	// The minimum and maximum vertical speed values in units of feet per minute
	static double const kMinVertSpeed_, kMaxVertSpeed_;

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

	/* The offset of the aircraft symbol in the inner gauge face relative to the exact center of the gauge.
	This is based on the proportion of pixels the need aircraft symbol has to be shifted in the y-direction (28)
	as a percentage of the diameter of the inner gauge ring in pixels (150 px) related to the distance spanned 
	by the diameter (radius = 30 NMI => diameter = 60 NMI) i.e. Offset = (28 / 150) * 60.
	*/
	static Distance const kAircraftToGaugeCenterOffset_;

	/* The pixel positions of the window the gauge is drawn in since our window is not moved.*/
	static float const kGaugePosLeft, kGaugePosRight, kGaugePosBot, kGaugePosTop;
	static float const kGaugeCenterX, kGaugeCenterY;
	
	/* The absolute pixel positions of the needle (determined relative to the gauge positions). */
	static float const kNeedlePosLeft, kNeedlePosRight, kNeedlePosBot, kNeedlePosTop;
	/* The translation that is applied to the needle in order to allow for rotation about the gauge center.*/
	static float const kNeedleTranslationX, kNeedleTranslationY;

	/* The offset that must be applied to account for OpenGL functions treating the +z axis 
	(90 degrees on a unit circle) as 0 degrees. */
	static float const kGlAngleOffset_;

	// The "application path", which for the plugin is the directory that the plugin is contained in
	char const * const app_path_;
	
	Decider * const decider_;
	Aircraft * const user_aircraft_;
	concurrency::concurrent_unordered_map<std::string, Aircraft*> * const intruders_;

	// An OpenGL quadric (quadratic) object required for use with the GLUT library's partial disk function.
	GLUquadricObj* quadric_;

	XPLMTextureID glTextures_[texture_constants::kNumTextures];

	static texture_constants::TexCoords const * AircraftSymbolFromThreatClassification(Aircraft::ThreatClassification threat_class);
	static texture_constants::GlRgb8Color const * GaugeRenderer::SymbolColorFromThreatClassification(Aircraft::ThreatClassification threat_class);

	bool LoadTexture(char * tex_path, int tex_id) const;

	/* Draws the outer gauge ring.*/
	void DrawOuterGauge() const;
	/* Draw the inner vertical speed gauge rings. */
	void DrawInnerGauge() const;
	/* Draws the vertical speed indicator needle on the gauge from the supplied vertical velocity.*/
	void DrawGaugeNeedle(Velocity const user_aircraft_vert_vel) const;
	/*  */
	void DrawIntrudingAircraft(LLA const * const intruder_pos, Velocity const * const intruder_vvel, Angle const * const user_heading, LLA const * const gauge_center_pos, Distance const * const range, Aircraft::ThreatClassification threat_class) const;

	/* Draws the supplied recommendation range. */
	void DrawRecommendationRange(RecommendationRange* rec_range, bool recommended) const;
	/* Draws the supplied vertical speed range as either recommended (green) or not recommended (red). */
	void DrawRecommendedVerticalSpeedRange(Velocity min_vert_speed, Velocity max_vert_speed, bool recommended) const;
	/* Draws the supplied degree range as either recommended (green) or not recommended (red).*/
	void DrawRecommendationRangeStartStop(Angle start, Angle stop, bool recommended) const;
	/* Draws a recommendation range starting at the supplied start angle in a sweep_angle degrees arc.*/
	void DrawRecommendationRangeStartSweep(Angle start, Angle sweep, bool recommended) const;

	void DrawTextureRegion(texture_constants::TexCoords const * tex_coords, double vert_left, double vert_right, double vert_top, double vert_bot) const;

	texture_constants::TexCoords const * GaugeRenderer::GaugeTexCoordsFromDigitCharacter(char) const;

	GaugeRenderer (const GaugeRenderer& that) = delete;
	GaugeRenderer& operator=(const GaugeRenderer& that) = delete;
};