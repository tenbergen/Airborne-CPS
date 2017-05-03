#include "GaugeRenderer.h"

const double GaugeRenderer::kMillisecondsPerSecond_ = 60000.0;

const double GaugeRenderer::kGaugeInnerCircleRadiusPxls_ = 150.0; // was 75
Distance const GaugeRenderer::kGaugeInnerCircleRadius_ { 30.0 , Distance::DistanceUnits::NMI };
Distance const GaugeRenderer::kAircraftToGaugeCenterOffset_ { (28.0 / (2.0 * kGaugeInnerCircleRadiusPxls_)) * kGaugeInnerCircleRadius_.to_feet() * 2.0, Distance::DistanceUnits::FEET};

float const GaugeRenderer::kGaugePosLeft = 1024.0f;
float const GaugeRenderer::kGaugePosRight = 1280.0f;
float const GaugeRenderer::kGaugePosBot = 0.0f;
float const GaugeRenderer::kGaugePosTop = 256.0f;

float const GaugeRenderer::kGaugeCenterX = (kGaugePosRight + kGaugePosLeft) / 2.0f;
float const GaugeRenderer::kGaugeCenterY = (kGaugePosTop + kGaugePosBot) / 2.0f;

float const GaugeRenderer::kNeedlePosLeft = kGaugePosLeft + 125.0f;
float const GaugeRenderer::kNeedlePosRight = kNeedlePosLeft + 8.0f;
float const GaugeRenderer::kNeedlePosBot = kGaugePosBot + 123.0f;
float const GaugeRenderer::kNeedlePosTop = kNeedlePosBot + 80.0f;

double const GaugeRenderer::kMinVertSpeed_ = -4000.0;
double const GaugeRenderer::kMaxVertSpeed_ = 4000.0;

double const GaugeRenderer::kMaxVSpeedDegrees = 150.0;
float const GaugeRenderer::kGlAngleOffset_ = 90.0f;

float const GaugeRenderer::kNeedleTranslationX = kNeedlePosLeft + ((kNeedlePosRight - kNeedlePosLeft) / 2.0f);
float const GaugeRenderer::kNeedleTranslationY = kNeedlePosBot + 5.0f;

double const GaugeRenderer::kDiskInnerRadius_ = 75.0;
double const GaugeRenderer::kDiskOuterRadius_ = 105.0;
int const GaugeRenderer::kDiskSlices_ = 32;
int const GaugeRenderer::kDiskLoops_ = 2;

GaugeRenderer::GaugeRenderer(char const * const app_path, Decider * const decider, Aircraft * const user_aircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> * const intruding_aircraft) : 
	app_path_(app_path), decider_(decider), user_aircraft_(user_aircraft), intruders_(intruding_aircraft) {
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

void GaugeRenderer::Render(texture_constants::GlRgb8Color cockpit_lighting) {
	user_aircraft_->lock_.lock();

	LLA const user_pos = user_aircraft_->position_current_;
	Angle const user_heading = user_aircraft_->heading_;
	Velocity const user_aircraft_vert_vel = user_aircraft_->vertical_velocity_;

	user_aircraft_->lock_.unlock();

	/// Turn off alpha blending and depth testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 0/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);

	/// Color the gauge background according to the day/night coloring inside the cockpit
	glColor3f(cockpit_lighting.red, cockpit_lighting.green, cockpit_lighting.blue);

	// Push the MV matrix onto the stack
	glPushMatrix();

	// Draw the gauge
	XPLMBindTexture2d(glTextures_[texture_constants::GAUGE_ID], 0);
	DrawOuterGauge();

	decider_->recommendation_range_lock_.lock();
	RecommendationRange positive = decider_->positive_recommendation_range_;
	RecommendationRange neg = decider_->negative_recommendation_range_;
	decider_->recommendation_range_lock_.unlock();

	// Draw the recommendation ranges if they should be drawn
	if (positive.valid)
		DrawRecommendationRange(&positive, true);

	if (neg.valid)
		DrawRecommendationRange(&neg, false);

	// Enable alpha blending for drawing the rest of the textures
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 1/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);

	// Turn off the day/night tinting while drawing the inner gauge parts
	// Will have to examine if this should be turned off or not.
	glColor3f(1.0f, 1.0f, 1.0f);

	// Specify the blending mode so the inner gauge rings draw on top of the outer gauge with proper transparency.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	concurrency::concurrent_unordered_map<std::string, Aircraft*>::const_iterator & iter = intruders_->cbegin();

	if (iter != intruders_->cend()) {
		LLA gauge_center_pos = user_pos.Translate(&user_heading, &kAircraftToGaugeCenterOffset_);

		for (; iter != intruders_->cend(); ++iter) {
			Aircraft* intruder = iter->second;

			intruder->lock_.lock();
			LLA const intruder_pos = intruder->position_current_;
			Distance const alt_diff = intruder_pos.altitude_ - intruder->position_old_.altitude_;
			std::chrono::milliseconds const alt_time_diff = intruder->position_current_time_- intruder->position_old_time_;
			Aircraft::ThreatClassification threat_class = intruder->threat_classification_;
			intruder->lock_.unlock();

			Distance range = gauge_center_pos.Range(&intruder_pos);

			if (range.to_feet() < kGaugeInnerCircleRadius_.to_feet() * 2) {
				Velocity const intr_vvel = Velocity( (alt_diff.to_feet() / (double) alt_time_diff.count()) * kMillisecondsPerSecond_, Velocity::VelocityUnits::FEET_PER_MIN );
				DrawIntrudingAircraft(&intruder_pos, &intr_vvel, &user_heading, &gauge_center_pos, &range, threat_class);
			}
		}
	}

	DrawInnerGaugeVelocityRing();

	DrawVerticalVelocityNeedle(user_aircraft_vert_vel);

	// Turn off Alpha Blending and turn on Depth Testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 0/*AlphaBlending*/, 1/*DepthTesting*/, 0/*DepthWriting*/);

	glPopMatrix();
	glFlush();
}

void GaugeRenderer::DrawIntrudingAircraft(LLA const * const intruder_pos, Velocity const * const intruder_vvel, Angle const * const user_heading, LLA const * const gauge_center_pos, Distance const * const range, Aircraft::ThreatClassification threat_class) const {
	Angle bearing = gauge_center_pos->Bearing(intruder_pos);
	// These two calls line makes the gauge display intruders relative to the user's heading
	bearing = bearing - *user_heading;
	bearing.normalize();

	double range_over_max_range_ratio = range->to_feet() / kGaugeInnerCircleRadius_.to_feet();
	double pixel_offset = range_over_max_range_ratio * kGaugeInnerCircleRadiusPxls_;

	Angle cartesian_angle = Angle::BearingToCartesianAngle(&bearing);
	double pixel_offset_x = cos(cartesian_angle.to_radians()) * pixel_offset;
	double pixel_offset_y = sin(cartesian_angle.to_radians()) * pixel_offset;

	double symbol_center_x = kGaugeCenterX + pixel_offset_x;
	double symbol_center_y = kGaugeCenterY + pixel_offset_y;

	// The symbols are contained in 16x16 pixel squares in the texture so to find the vertices, 
	// calculate the sides of the 16 px square centered around the symbol center
	double symbol_left = symbol_center_x - 8.0;
	double symbol_right = symbol_center_x + 8.0;
	double symbol_bot = symbol_center_y - 8.0;
	double symbol_top = symbol_center_y + 8.0;

	texture_constants::TexCoords const * symbol_coords = AircraftSymbolFromThreatClassification(threat_class);
	DrawTextureRegion(symbol_coords, symbol_left, symbol_right, symbol_top, symbol_bot);

	Distance altitude_difference = intruder_pos->altitude_ - gauge_center_pos->altitude_;
	int alt_diff_hundreds_ft_per_min = (int) std::round(altitude_difference.to_feet() / 100.0);

	texture_constants::TexCoords const * sign_char = alt_diff_hundreds_ft_per_min < 0.0 ? &texture_constants::kCharMinusSign : &texture_constants::kCharPlusSign;
	alt_diff_hundreds_ft_per_min = abs(alt_diff_hundreds_ft_per_min);
	std::string alt_string = std::to_string(alt_diff_hundreds_ft_per_min);

	// Characters in the GaugeTex256 texture are 6x10 (6 px wide, 10 px tall)
	double alt_string_top = symbol_bot;
	double alt_string_bot = alt_string_top - 10.0;
	double alt_string_left = symbol_left;
	double alt_string_right = alt_string_left + 6.0;

	texture_constants::GlRgb8Color const * symbol_color = SymbolColorFromThreatClassification(threat_class);
	glColor3f(symbol_color->red, symbol_color->green, symbol_color->blue);

	DrawTextureRegion(sign_char, alt_string_left, alt_string_right, alt_string_top, alt_string_bot);
	alt_string_left = alt_string_right;
	alt_string_right += 6.0;

	for (char c : alt_string) {
		texture_constants::TexCoords const * char_coords = GaugeTexCoordsFromDigitCharacter(c);
		DrawTextureRegion(char_coords, alt_string_left, alt_string_right, alt_string_top, alt_string_bot);
		alt_string_left = alt_string_right;
		alt_string_right += 6.0;
	}

	texture_constants::TexCoords const * vvel_arrow_coords = intruder_vvel->to_feet_per_min() < 0.0 ? &texture_constants::kVertArrowDown : &texture_constants::kVertArrowUp;
	// The vertical arrow is 5 px wide by 13 px tall; the arrow is usually? centered vertically wrt to the symbol so the arrow should be drawn some amount lower than the top of the symbol
	DrawTextureRegion(vvel_arrow_coords, symbol_right, symbol_right + 5.0, symbol_top - 4.0, symbol_top - 17.0);

	glColor3f(1.0f, 1.0f, 1.0f);
}

texture_constants::TexCoords const * GaugeRenderer::GaugeTexCoordsFromDigitCharacter(char c) const {
	switch (c) {
	case '0':
		return &texture_constants::kCharZero;
	case '1':
		return &texture_constants::kCharOne;
	case '2':
		return &texture_constants::kCharTwo;
	case '3':
		return &texture_constants::kCharThree;
	case '4':
		return &texture_constants::kCharFour;
	case '5':
		return &texture_constants::kCharFive;
	case '6':
		return &texture_constants::kCharSix;
	case '7':
		return &texture_constants::kCharSeven;
	case '8':
		return &texture_constants::kCharEight;
	case '9':
		return &texture_constants::kCharNine;
	default:
		char debug_buf[128];
		snprintf(debug_buf, 128, "GaugeRenderer::GaugeTexCoordsFromDigitCharacter - unhandled character %c supplied\n", c);

		return &texture_constants::kDebugSymbol;
	}
}

void GaugeRenderer::DrawTextureRegion(texture_constants::TexCoords const * tex_coords, 
	double vert_left, double vert_right, double vert_top, double vert_bot) const {
	glBegin(GL_QUADS);
	glTexCoord2d(tex_coords->right, tex_coords->bottom); glVertex2d(vert_right, vert_bot);
	glTexCoord2d(tex_coords->left, tex_coords->bottom); glVertex2d(vert_left, vert_bot);
	glTexCoord2d(tex_coords->left, tex_coords->top); glVertex2d(vert_left, vert_top);
	glTexCoord2d(tex_coords->right, tex_coords->top); glVertex2d(vert_right, vert_top);
	glEnd();
}

texture_constants::TexCoords const * GaugeRenderer::AircraftSymbolFromThreatClassification(Aircraft::ThreatClassification threat_class) {
	switch (threat_class) {
	case Aircraft::ThreatClassification::NON_THREAT_TRAFFIC:
		return &texture_constants::kSymbolBlueDiamondCutout;
	case Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC:
		return &texture_constants::kSymbolBlueDiamondWhole;
	case Aircraft::ThreatClassification::TRAFFIC_ADVISORY:
		return &texture_constants::kSymbolYellowCircle;
	case Aircraft::ThreatClassification::RESOLUTION_ADVISORY:
		return &texture_constants::kSymbolRedSquare;
	default:
		XPLMDebugString("GaugeRenderer::AircraftSymbolFromThreatClassification - Unknown threat classification.\n");
		return &texture_constants::kDebugSymbol;
	}
}

texture_constants::GlRgb8Color const * GaugeRenderer::SymbolColorFromThreatClassification(Aircraft::ThreatClassification threat_class) {
	switch (threat_class) {
	case Aircraft::ThreatClassification::NON_THREAT_TRAFFIC:
	case Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC:
		return &texture_constants::kSymbolBlueDiamondColor;
	case Aircraft::ThreatClassification::TRAFFIC_ADVISORY:
		return &texture_constants::kSymbolYellowCircleColor;
	case Aircraft::ThreatClassification::RESOLUTION_ADVISORY:
		return &texture_constants::kSymbolRedSquareColor;
	default:
		XPLMDebugString("GaugeRenderer::SymbolColorFromThreatClassification - Unknown threat classification.\n");
		return &texture_constants::kRecommendationRangePositive;
	}
}

void GaugeRenderer::DrawOuterGauge() const {
	DrawTextureRegion(&texture_constants::kOuterGauge, kGaugePosLeft, kGaugePosRight, kGaugePosTop, kGaugePosBot);
}

void GaugeRenderer::DrawInnerGaugeVelocityRing() const {
	DrawTextureRegion(&texture_constants::kInnerGauge, kGaugePosLeft, kGaugePosRight, kGaugePosTop, kGaugePosBot);
}

void GaugeRenderer::DrawVerticalVelocityNeedle(Velocity const user_aircraft_vert_vel) const {
	// Translate the needle so it's properly rotated in place about the gauge center
	glTranslatef(kNeedleTranslationX, kNeedleTranslationY, 0.0f);

	// Rotate the needle according to the current vertical velocity - 4,000 ft/min is 150 degree rotation relative to 0 ft/min
	double vert_speed_deg = (user_aircraft_vert_vel.to_feet_per_min() / kMaxVertSpeed_) * kMaxVSpeedDegrees - kGlAngleOffset_;
	vert_speed_deg = math_util::clampd(vert_speed_deg, -240.0, 60.0);
	glRotated(vert_speed_deg, 0.0, 0.0, -1.0);

	// Translate the needle back so it's in the gauge center
	glTranslatef(-kNeedleTranslationX, -kNeedleTranslationY, 0.0f);

	glBlendFunc(GL_DST_COLOR, GL_ZERO);

	// Draw Needle Mask
	XPLMBindTexture2d(glTextures_[texture_constants::NEEDLE_MASK_ID], 0);
	DrawTextureRegion(&texture_constants::kNeedleMask, kNeedlePosLeft, kNeedlePosRight, kNeedlePosTop, kNeedlePosBot);

	glBlendFunc(GL_ONE, GL_ONE);

	// Draw Needle
	XPLMBindTexture2d(glTextures_[texture_constants::NEEDLE_ID], 0);
	DrawTextureRegion(&texture_constants::kNeedle, kNeedlePosLeft, kNeedlePosRight, kNeedlePosTop, kNeedlePosBot);
}

void GaugeRenderer::DrawRecommendationRange(RecommendationRange* rec_range, bool recommended) const {
	DrawRecommendedVerticalSpeedRange(rec_range->min_vertical_speed, rec_range->max_vertical_speed, recommended);
}

void GaugeRenderer::DrawRecommendedVerticalSpeedRange(Velocity min_vertical, Velocity max_vertical, bool recommended) const {
	double min_vert = math_util::clampd(min_vertical.to_feet_per_min(), kMinVertSpeed_, kMaxVertSpeed_);
	double max_vert = math_util::clampd(max_vertical.to_feet_per_min(), kMinVertSpeed_, kMaxVertSpeed_);

	double start_angle = (min_vert / kMaxVertSpeed_) * kMaxVSpeedDegrees - kGlAngleOffset_;
	double stop_angle = (max_vert / kMaxVertSpeed_) * kMaxVSpeedDegrees - kGlAngleOffset_;

	DrawRecommendationRangeStartStop(Angle(start_angle, Angle::AngleUnits::DEGREES), Angle(stop_angle, Angle::AngleUnits::DEGREES), recommended);
}

void GaugeRenderer::DrawRecommendationRangeStartStop(Angle start, Angle stop, bool recommended) const {
	start.normalize();
	stop.normalize();

	/* The start and stop angles are specifying a clockwise arc that winds around the circle centered on the gauge
	so if the start angle is greater than the stop angle, we will perform the clockwise rotation that would result by swapping 
	the arguments instead of allowing a counter-clockwise rotation to happen, which is guaranteed to wrap around the entire gauge.*/
	if (start > stop) {
		Angle min = stop;
		stop = start;
		start = min;
	}

	DrawRecommendationRangeStartSweep(start, stop - start, recommended);
}

void GaugeRenderer::DrawRecommendationRangeStartSweep(Angle start, Angle sweep, bool recommended) const {
	start.normalize();
	sweep.normalize();
	
	glPushMatrix();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	texture_constants::GlRgb8Color const * rec_range_color = recommended ? 
		&texture_constants::kRecommendationRangePositive : &texture_constants::kRecommendationRangeNegative;

	glColor4f(rec_range_color->red, rec_range_color->green, rec_range_color->blue, 1.0f);

	glLoadIdentity();
	glTranslatef(kGaugeCenterX, kGaugeCenterY, 0.0f);
	gluPartialDisk(quadric_, kDiskInnerRadius_, kDiskOuterRadius_, kDiskSlices_, kDiskLoops_, start.to_degrees(), sweep.to_degrees());

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);

	glPopMatrix();
}