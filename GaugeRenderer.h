#pragma once

#include "XPLMUtilities.h"

#include "RecommendationRange.h"
#include "Aircraft.h"
#include "BMPLoader.h"
#include "Renderer.inc"
#include "Distance.h"
#include "MathUtil.h"

class GaugeRenderer
{
public:
	GaugeRenderer(char* appPath);
	~GaugeRenderer();

	void LoadTextures();
	int LoadTexture(char *pFileName, int TextureId);
	void Render(float* rgb, Aircraft * const user_aircraft, Aircraft * const intruder, RecommendationRange*  recommended, RecommendationRange* not_recommended);

	// The minimum and maximum vertical speed values in units of feet per minute
	static const float kMinVertSpeed_, kMaxVertSpeed_;

	// The clockwise degree rotation corresponding to the maximum vertical speed, with 180 degrees on a unit circle defined as 0 degrees
	static const float kMaxVSpeedDegrees;

private:
	static const unsigned char kNumTextures = 3;

	static constexpr char* kGaugeTexFname_ = "GaugeTex256.bmp";
	static constexpr char* kNeedleTexFname_ = "Needle.bmp";
	static constexpr char* kNeedleMaskFname_ = "NeedleMask.bmp";

	static const unsigned char kGaugeTexId = 0;
	static const unsigned char kNeedleTexId = 1;
	static const unsigned char kNeedleTexMaskId = 2;

	// The radius of the inner circle of the gauge that contains the airplane icons in pixels
	static const double kGaugeInnerCircleRadiusPxls_;
	static Distance const kGaugeInnerCircleRadius_;

	// The offset in NMI of the airplane relative to the exact center of the gauge
	static Distance const kAirplaneOffset_;

	static const float kGaugePosLeft, kGaugePosRight, kGaugePosBot, kGaugePosTop;
	static const float kGaugeCenterX, kGaugeCenterY;
	
	static const float kNeedlePosLeft, kNeedlePosRight, kNeedlePosBot, kNeedlePosTop;
	static const float kNeedleTranslationX, kNeedleTranslationY;

	// The offset that must be applied to account for GLUPartialDisk treating the +z axis (90 degrees on a unit circle) as 0 degrees 
	static const float kGlAngleOffset_;

	static const float kMinDegrees, kMaxDegrees;

	// The "application path", which for the plugin is the directory that the plugin is contained in
	const char* app_path_;

	GLUquadricObj* quadric_;

	BmpLoader::tagIMAGEDATA bmpTextures_[kNumTextures];
	XPLMTextureID glTextures_[kNumTextures];

	/* Draws the supplied recommendation range */
	void DrawRecommendationRange(RecommendationRange rec_range);
	/* Draws the supplied vertical speed range as either recommended (green) or not recommended (red) */
	void DrawRecommendedVerticalSpeedRange(float min_vert_speed, float max_vert_speed, bool recommended);
	/* Draws the supplied degree range as either recommended (green) or not recommended (red)*/
	void DrawRecommendationRangeStartStop(float start_angle, float stop_angle, bool recommended);
	/* Draws a recommendation range starting at the supplied start angle in a sweepAngle degrees arc*/
	void DrawRecommendationRangeStartSweep(float start_angle, float sweep_angle, bool recommended);

	void BuildTexPath(char* catBuf, char* tex_fname, const char* plugin_path);
};