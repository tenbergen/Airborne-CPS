#include "GaugeRenderer.h"

const double GaugeRenderer::kGaugeInnerCircleRadiusPxls_ = 75.0;
Distance const GaugeRenderer::kGaugeInnerCircleRadius_ { 30.0 , Distance::NMI };
Distance const GaugeRenderer::kAircraftToGaugeCenterOffset_ { (28.0 / (2.0 * kGaugeInnerCircleRadiusPxls_)) * kGaugeInnerCircleRadius_.to_feet() * 2.0, Distance::FEET};

const float GaugeRenderer::kGaugePosLeft = 768.0f;
const float GaugeRenderer::kGaugePosRight = 1024.0f;
const float GaugeRenderer::kGaugePosBot = 0.0f;
const float GaugeRenderer::kGaugePosTop = 256.0f;

const float GaugeRenderer::kGaugeCenterX = (kGaugePosRight + kGaugePosLeft) / 2.0f;
const float GaugeRenderer::kGaugeCenterY = (kGaugePosTop + kGaugePosBot) / 2.0f;

const float GaugeRenderer::kNeedlePosLeft = kGaugePosLeft + 125.0f;
const float GaugeRenderer::kNeedlePosRight = kNeedlePosLeft + 8.0f;
const float GaugeRenderer::kNeedlePosBot = kGaugePosBot + 123.0f;
const float GaugeRenderer::kNeedlePosTop = kNeedlePosBot + 80.0f;

const double GaugeRenderer::kMinVertSpeed_ = -4000.0;
const double GaugeRenderer::kMaxVertSpeed_ = 4000.0;

const double GaugeRenderer::kMaxVSpeedDegrees = 150.0;
const float GaugeRenderer::kGlAngleOffset_ = 90.0f;

const float GaugeRenderer::kNeedleTranslationX = kNeedlePosLeft + ((kNeedlePosRight - kNeedlePosLeft) / 2.0f);
const float GaugeRenderer::kNeedleTranslationY = kNeedlePosBot + 5.0f;

const double GaugeRenderer::kDiskInnerRadius_ = 50.0;
const double GaugeRenderer::kDiskOuterRadius_ = 105.0;
const int GaugeRenderer::kDiskSlices_ = 32;
const int GaugeRenderer::kDiskLoops_ = 2;

GaugeRenderer::GaugeRenderer(char const * const app_path, Aircraft * const user_aircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> * const intruding_aircraft) : app_path_(app_path), user_aircraft_(user_aircraft), intruders_(intruding_aircraft) {
	quadric_ = gluNewQuadric();

	gluQuadricNormals(quadric_, GLU_SMOOTH);
	gluQuadricDrawStyle(quadric_, GLU_FILL);
	gluQuadricTexture(quadric_, GLU_FALSE);
	gluQuadricOrientation(quadric_, GLU_INSIDE);
}

GaugeRenderer::~GaugeRenderer() {
	gluDeleteQuadric(quadric_);
}

void GaugeRenderer::LoadTextures()
{
	char fname_buf[256];

	for (int tex_id = texture_constants::GAUGE_ID; tex_id < texture_constants::kNumTextures; tex_id++) {
		str_util::BuildFilePath(fname_buf, texture_constants::kGaugeFileNames[tex_id], app_path_);
		
		if (strlen(fname_buf) > 0 && !LoadTexture(fname_buf, tex_id)) {
			char debug_buf[256];
			snprintf(fname_buf, 256, "GaugeRenderer::LoadTextures - failed to load texture at: %s\n", fname_buf);
			XPLMDebugString(debug_buf);
		}
	}
}


bool GaugeRenderer::LoadTexture(char* tex_path, int tex_id) const {
	bool loaded_successfully = false;

	BmpLoader::IMAGEDATA sImageData;
	/// Get the bitmap from the file
	loaded_successfully = BmpLoader::LoadBmp(tex_path, &sImageData) != 0;

	if (loaded_successfully)
	{
		BmpLoader::SwapRedBlue(&sImageData);

		/// Do the opengl stuff using XPLM functions for a friendly Xplane existence.
		XPLMGenerateTextureNumbers((int *) &glTextures_[tex_id], 1);
		XPLMBindTexture2d(glTextures_[tex_id], 0);

		// This (safely?) assumes that the bitmap will be either 4 channels (RGBA) or 3 channels (RGB)
		GLenum type = sImageData.Channels == 4 ? GL_RGBA : GL_RGB;
		gluBuild2DMipmaps(GL_TEXTURE_2D, sImageData.Channels, sImageData.Width, sImageData.Height, type, GL_UNSIGNED_BYTE, sImageData.pData);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	if (sImageData.pData)
		free(sImageData.pData);

	return loaded_successfully;
}

void GaugeRenderer::Render(float* rgb, RecommendationRange*  recommended, RecommendationRange* not_recommended) {
	user_aircraft_->lock_.lock();

	LLA const user_pos = user_aircraft_->position_current_;
	Vec2 const user_vel = user_aircraft_->horizontal_velocity_;
	double const user_aircraft_vert_vel = user_aircraft_->vertical_velocity_;

	user_aircraft_->lock_.unlock();

	/// Turn off alpha blending and depth testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 0/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);

	/// Color the gauge background according to the day/night coloring inside the cockpit
	glColor3f(rgb[0], rgb[1], rgb[2]);

	// Push the MV matrix onto the stack
	glPushMatrix();

	// Draw the gauge
	XPLMBindTexture2d(glTextures_[texture_constants::GAUGE_ID], 0);

	DrawOuterGauge();

	// Draw the recommendation ranges if they were provided
	if (recommended)
		DrawRecommendationRange(*recommended);

	if (not_recommended)
		DrawRecommendationRange(*not_recommended);

	// Enable alpha blending for drawing the rest of the textures
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 1/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);

	// Turn off the day/night tinting while drawing the inner gauge parts; will have to examine if this should be turned off or not.
	glColor3f(1.0f, 1.0f, 1.0f);

	// Specify the blending mode so the inner gauge rings draw on top of the outer gauge with proper transparency.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DrawInnerGauge();

	concurrency::concurrent_unordered_map<std::string, Aircraft*>::const_iterator & iter = intruders_->cbegin();

	if (iter != intruders_->cend()) {
		LLA gauge_center_pos = CalculateGaugeCenterPosition(&user_pos, &user_vel);

		for (; iter != intruders_->cend(); ++iter) {
			Aircraft* intruder = iter->second;
			intruder->lock_.lock();

			LLA const intruder_pos = intruder->position_current_;
			intruder->lock_.unlock();

			// Here is where to insert for each aircraft
			Distance range = gauge_center_pos.Range(&intruder_pos);

			if (range.to_feet() < kGaugeInnerCircleRadius_.to_feet())
				DrawIntrudingAircraft(&intruder_pos, &gauge_center_pos, &range);
		}
	}

	DrawGaugeNeedle(user_aircraft_vert_vel);

	// Turn off Alpha Blending and turn on Depth Testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 0/*AlphaBlending*/, 1/*DepthTesting*/, 0/*DepthWriting*/);

	glPopMatrix();
	glFlush();
}

void GaugeRenderer::DrawIntrudingAircraft(LLA const * const intruder_pos, LLA const * const gauge_center_pos, Distance const * const range) const {
	Angle bearing = gauge_center_pos->Bearing(intruder_pos);
	double bearing_rads = bearing.to_radians();

	double range_over_max_range_ratio = range->to_feet() / kGaugeInnerCircleRadius_.to_feet();
	double pixel_offset = range_over_max_range_ratio * kGaugeInnerCircleRadiusPxls_;

	double pixel_offset_x = cos(bearing_rads) * pixel_offset;
	double pixel_offset_y = sin(bearing_rads) * pixel_offset;

	double symbol_center_x = kGaugeCenterX + pixel_offset_x;
	double symbol_center_y = kGaugeCenterY + pixel_offset_y;

	// The symbols are contained in 16x16 pixel squares in the texture so to find the vertices, 
	//calculate the 16 pixel square centered around the symbol center
	double symbol_left = symbol_center_x - 8;
	double symbol_right = symbol_center_x + 8;
	double symbol_bot = symbol_center_y - 8;
	double symbol_top = symbol_center_y + 8;

	texture_constants::TexCoords symbol_coords = texture_constants::kSymbolRedSquare;

	glBegin(GL_QUADS);
	glTexCoord2d(symbol_coords.right, symbol_coords.bottom); glVertex2d(symbol_right, symbol_bot);
	glTexCoord2d(symbol_coords.left, symbol_coords.bottom); glVertex2d(symbol_left, symbol_bot);
	glTexCoord2d(symbol_coords.left, symbol_coords.top); glVertex2d(symbol_left, symbol_top);
	glTexCoord2d(symbol_coords.right, symbol_coords.top); glVertex2d(symbol_right, symbol_top);
	glEnd();
}


void GaugeRenderer::DrawOuterGauge() const {
	texture_constants::TexCoords outer_gauge = texture_constants::kOuterGauge;

	glBegin(GL_QUADS);
	glTexCoord2d(outer_gauge.right, outer_gauge.bottom); glVertex2f(kGaugePosRight, kGaugePosBot);
	glTexCoord2d(outer_gauge.left, outer_gauge.bottom); glVertex2f(kGaugePosLeft, kGaugePosBot);
	glTexCoord2d(outer_gauge.left, outer_gauge.top); glVertex2f(kGaugePosLeft, kGaugePosTop);
	glTexCoord2d(outer_gauge.right, outer_gauge.top); glVertex2f(kGaugePosRight, kGaugePosTop);
	glEnd();
}

void GaugeRenderer::DrawInnerGauge() const {
	texture_constants::TexCoords inner_gauge = texture_constants::kInnerGauge;

	glBegin(GL_QUADS);
	glTexCoord2d(inner_gauge.right, inner_gauge.bottom); glVertex2f(kGaugePosRight, kGaugePosBot);
	glTexCoord2d(inner_gauge.left, inner_gauge.bottom); glVertex2f(kGaugePosLeft, kGaugePosBot);
	glTexCoord2d(inner_gauge.left, inner_gauge.top); glVertex2f(kGaugePosLeft, kGaugePosTop);
	glTexCoord2d(inner_gauge.right, inner_gauge.top); glVertex2f(kGaugePosRight, kGaugePosTop);
	glEnd();
}

void GaugeRenderer::DrawGaugeNeedle(double const user_aircraft_vert_vel) const {
	// Translate the needle so it's properly rotated in place about the gauge center
	glTranslatef(kNeedleTranslationX, kNeedleTranslationY, 0.0f);

	// Rotate the needle according to the current vertical velocity - 4,000 ft/min is 150 degree rotation relative to 0 ft/min
	double vert_speed_deg = (user_aircraft_vert_vel / kMaxVertSpeed_) * kMaxVSpeedDegrees - kGlAngleOffset_;
	vert_speed_deg = math_util::clampd(vert_speed_deg, -240.0, 60.0);
	glRotated(vert_speed_deg, 0.0, 0.0, -1.0);

	// Translate the needle back so it's in the gauge center
	glTranslatef(-kNeedleTranslationX, -kNeedleTranslationY, 0.0f);

	glBlendFunc(GL_DST_COLOR, GL_ZERO);

	// Draw Needle Mask
	XPLMBindTexture2d(glTextures_[texture_constants::NEEDLE_MASK_ID], 0);

	texture_constants::TexCoords needle_mask = texture_constants::kNeedleMask;

	glBegin(GL_QUADS);
	glTexCoord2d(needle_mask.right, needle_mask.bottom); glVertex2f(kNeedlePosRight, kNeedlePosBot);
	glTexCoord2d(needle_mask.left, needle_mask.bottom); glVertex2f(kNeedlePosLeft, kNeedlePosBot);
	glTexCoord2d(needle_mask.left, needle_mask.top); glVertex2f(kNeedlePosLeft, kNeedlePosTop);
	glTexCoord2d(needle_mask.right, needle_mask.top); glVertex2f(kNeedlePosRight, kNeedlePosTop);
	glEnd();

	glBlendFunc(GL_ONE, GL_ONE);

	texture_constants::TexCoords needle = texture_constants::kNeedle;

	// Draw Needle
	XPLMBindTexture2d(glTextures_[texture_constants::NEEDLE_ID], 0);
	glBegin(GL_QUADS);
	glTexCoord2d(needle.right, needle.bottom); glVertex2f(kNeedlePosRight, kNeedlePosBot);
	glTexCoord2d(needle.left, needle.bottom); glVertex2f(kNeedlePosLeft, kNeedlePosBot);
	glTexCoord2d(needle.left, needle.top); glVertex2f(kNeedlePosLeft, kNeedlePosTop);
	glTexCoord2d(needle.right, needle.top); glVertex2f(kNeedlePosRight, kNeedlePosTop);
	glEnd();
}

LLA GaugeRenderer::CalculateGaugeCenterPosition(LLA const * const position, Vec2 const * const velocity) const {
	Angle bearing = Aircraft::VelocityToBearing(velocity);
	return position->Translate(&bearing, &kAircraftToGaugeCenterOffset_);
}

void GaugeRenderer::DrawRecommendationRange(RecommendationRange& rec_range) const {
	DrawRecommendedVerticalSpeedRange(rec_range.min_vertical_speed, rec_range.max_vertical_speed, rec_range.recommended);
}

void GaugeRenderer::DrawRecommendedVerticalSpeedRange(double min_vert_speed, double max_vert_speed, bool recommended) const {
	if (min_vert_speed > max_vert_speed) {
		double min = min_vert_speed;
		min_vert_speed = max_vert_speed;
		max_vert_speed = min;
	}

	min_vert_speed = math_util::clampd(min_vert_speed, kMinVertSpeed_, kMaxVertSpeed_);
	max_vert_speed = math_util::clampd(max_vert_speed, kMinVertSpeed_, kMaxVertSpeed_);

	double start_angle = (min_vert_speed / kMaxVertSpeed_) * kMaxVSpeedDegrees - kGlAngleOffset_;
	double stop_angle = (max_vert_speed / kMaxVertSpeed_) * kMaxVSpeedDegrees - kGlAngleOffset_;

	DrawRecommendationRangeStartStop(start_angle, stop_angle, recommended);
}

void GaugeRenderer::DrawRecommendationRangeStartStop(double start_angle, double stop_angle, bool recommended) const {
	start_angle = math_util::clampd(start_angle, Angle::kMinDegrees_, Angle::kMaxDegrees_);
	stop_angle = math_util::clampd(stop_angle, Angle::kMinDegrees_, Angle::kMaxDegrees_);

	if (start_angle < 0.0f)
		start_angle += Angle::kMaxDegrees_;

	if (stop_angle < 0.0f)
		stop_angle += Angle::kMaxDegrees_;

	DrawRecommendationRangeStartSweep(start_angle, stop_angle - start_angle, recommended);
}

void GaugeRenderer::DrawRecommendationRangeStartSweep(double start_angle, double sweep_angle, bool recommended) const {
	start_angle = math_util::clampd(start_angle, Angle::kMinDegrees_, Angle::kMaxDegrees_);
	sweep_angle = math_util::clampd(sweep_angle, Angle::kMinDegrees_, Angle::kMaxDegrees_);

	if (start_angle < 0.0f)
		start_angle += Angle::kMaxDegrees_;

	if (sweep_angle < 0.0f)
		sweep_angle += Angle::kMaxDegrees_;
	
	glPushMatrix();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	if (recommended) {
		glColor4f(0.141f, 0.647f, 0.059f, 1.0f);
	}
	else {
		glColor4f(0.602f, 0.102f, 0.09f, 1.0f);
	}

	glLoadIdentity();
	glTranslatef(kGaugeCenterX, kGaugeCenterY, 0.0f);
	gluPartialDisk(quadric_, kDiskInnerRadius_, kDiskOuterRadius_, kDiskSlices_, kDiskLoops_, start_angle, sweep_angle);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);

	glPopMatrix();
}