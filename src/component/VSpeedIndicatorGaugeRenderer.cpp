#include "VSpeedIndicatorGaugeRenderer.h"

const double VSIGaugeRenderer::kMillisecondsPerSecond_ = 60000.0;

const double VSIGaugeRenderer::kGaugeInnerCircleRadiusPxls_ = 150.0;
Distance const VSIGaugeRenderer::kGaugeInnerCircleRadius_ { 30.0 , Distance::DistanceUnits::NMI };
Distance const VSIGaugeRenderer::kAircraftToGaugeCenterOffset_ { (28.0 / (2.0 * kGaugeInnerCircleRadiusPxls_)) * kGaugeInnerCircleRadius_.toFeet() * 2.0, Distance::DistanceUnits::FEET};

float const VSIGaugeRenderer::kGaugePosLeft_ = 1024.0f;
float const VSIGaugeRenderer::kGaugePosRight_ = 1280.0f;
float const VSIGaugeRenderer::kGaugePosBot_ = 0.0f;
float const VSIGaugeRenderer::kGaugePosTop_ = 256.0f;

float const VSIGaugeRenderer::kGaugeCenterX_ = (kGaugePosRight_ + kGaugePosLeft_) / 2.0f;
float const VSIGaugeRenderer::kGaugeCenterY_ = (kGaugePosTop_ + kGaugePosBot_) / 2.0f;

float const VSIGaugeRenderer::kNeedlePosLeft_ = kGaugePosLeft_ + 125.0f;
float const VSIGaugeRenderer::kNeedlePosRight_ = kNeedlePosLeft_ + 8.0f;
float const VSIGaugeRenderer::kNeedlePosBot_ = kGaugePosBot_ + 123.0f;
float const VSIGaugeRenderer::kNeedlePosTop_ = kNeedlePosBot_ + 80.0f;

double const VSIGaugeRenderer::kMinVertSpeed = -4000.0;
double const VSIGaugeRenderer::kMaxVertSpeed = 4000.0;

double const VSIGaugeRenderer::kMaxVSpeedDegrees = 150.0;
float const VSIGaugeRenderer::kGlAngleOffset_ = 90.0f;

float const VSIGaugeRenderer::kNeedleTranslationX_ = kNeedlePosLeft_ + ((kNeedlePosRight_ - kNeedlePosLeft_) / 2.0f);
float const VSIGaugeRenderer::kNeedleTranslationY_ = kNeedlePosBot_ + 5.0f;

double const VSIGaugeRenderer::kDiskInnerRadius_ = 75.0;
double const VSIGaugeRenderer::kDiskOuterRadius_ = 105.0;
int const VSIGaugeRenderer::kDiskSlices_ = 32;
int const VSIGaugeRenderer::kDiskLoops_ = 2;


VSIGaugeRenderer::VSIGaugeRenderer(char const * const appPath, Decider * const decider, Aircraft * const userAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> * const intrudingAircraft) :
	appPath_(appPath), decider_(decider), userAircraft_(userAircraft), intruders_(intrudingAircraft) {
	quadric_ = gluNewQuadric();

	gluQuadricNormals(quadric_, GLU_SMOOTH);
	gluQuadricDrawStyle(quadric_, GLU_FILL);
	gluQuadricTexture(quadric_, GLU_FALSE);
	gluQuadricOrientation(quadric_, GLU_INSIDE);
}

VSIGaugeRenderer::~VSIGaugeRenderer() {
	gluDeleteQuadric(quadric_);
}

void VSIGaugeRenderer::loadTextures()
{
	char fNameBuf[256];

	for (int texId = vsitextureconstants::GAUGE_ID; texId < vsitextureconstants::K_NUM_TEXTURES; texId++) {
		strutil::buildFilePath(fNameBuf, vsitextureconstants::K_GAUGE_FILENAMES[texId], appPath_);
		
		if (strlen(fNameBuf) > 0 && !loadTexture(fNameBuf, texId)) {
			char debugBuf[256];
			snprintf(fNameBuf, 256, "VSIGaugeRenderer::LoadTextures - failed to load texture at: %s\n", fNameBuf);
			XPLMDebugString(debugBuf);
		}
	}
}


bool VSIGaugeRenderer::loadTexture(char* texPath, int texId) const {
	bool loadedSuccessfully = false;

	BmpLoader::ImageData sImageData;
	/// Get the bitmap from the file
	loadedSuccessfully = BmpLoader::loadBmp(texPath, &sImageData) != 0;

	if (loadedSuccessfully)
	{
		BmpLoader::swapRedBlue(&sImageData);

		/// Do the opengl stuff using XPLM functions for a friendly Xplane existence.
		XPLMGenerateTextureNumbers((int *) &glTextures_[texId], 1);
		XPLMBindTexture2d(glTextures_[texId], 0);

		// This (safely?) assumes that the bitmap will be either 4 channels (RGBA) or 3 channels (RGB)
		GLenum type = sImageData.channels == 4 ? GL_RGBA : GL_RGB;
		gluBuild2DMipmaps(GL_TEXTURE_2D, sImageData.channels, sImageData.width, sImageData.height, type, GL_UNSIGNED_BYTE, sImageData.pData);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	if (sImageData.pData)
		free(sImageData.pData);

	return loadedSuccessfully;
}

void VSIGaugeRenderer::render(vsitextureconstants::GlRgb8Color cockpitLighting) {
	userAircraft_->lock.lock();

	LLA const userPos = userAircraft_->positionCurrent;
	Angle const userHeading = userAircraft_->heading;
	Velocity const userAircraftVertVel = userAircraft_->verticalVelocity;

	userAircraft_->lock.unlock();

	/// Turn off alpha blending and depth testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 0/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);

	/// Color the gauge background according to the day/night coloring inside the cockpit
	glColor3f(cockpitLighting.red, cockpitLighting.green, cockpitLighting.blue);

	// Push the MV matrix onto the stack
	glPushMatrix();

	// Draw the gauge
	XPLMBindTexture2d(glTextures_[vsitextureconstants::GAUGE_ID], 0);
	drawOuterGauge();

	decider_->recommendationRangeLock.lock();
	RecommendationRange positive = decider_->positiveRecommendationRange;
	RecommendationRange neg = decider_->negativeRecommendationRange;
	decider_->recommendationRangeLock.unlock();

	// Draw the recommendation ranges if they should be drawn
	if (positive.valid)
		drawRecommendationRange(&positive, true);

	if (neg.valid)
		drawRecommendationRange(&neg, false);

	// Enable alpha blending for drawing the rest of the textures
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 1/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);

	// Turn off the day/night tinting while drawing the inner gauge parts
	// Will have to examine if this should be turned off or not.
	glColor3f(1.0f, 1.0f, 1.0f);

	// Specify the blending mode so the inner gauge rings draw on top of the outer gauge with proper transparency.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	concurrency::concurrent_unordered_map<std::string, Aircraft*>::const_iterator & iter = intruders_->cbegin();

	if (iter != intruders_->cend()) {
		LLA gaugeCenterPos = userPos.translate(&userHeading, &kAircraftToGaugeCenterOffset_);

		for (; iter != intruders_->cend(); ++iter) {
			Aircraft* intruder = iter->second;

			intruder->lock.lock();
			LLA const intruderPos = intruder->positionCurrent;
			Distance const altDiff = intruderPos.altitude - intruder->positionOld.altitude;
			std::chrono::milliseconds const altTimeDiff = intruder->positionCurrentTime- intruder->positionOldTime;
			Aircraft::ThreatClassification threatClass = intruder->threatClassification;
			intruder->lock.unlock();

			Distance range = gaugeCenterPos.range(&intruderPos);

			if (range.toFeet() < kGaugeInnerCircleRadius_.toFeet() * 2) {
				Velocity const intrVvel = Velocity( (altDiff.toFeet() / (double) altTimeDiff.count()) * kMillisecondsPerSecond_, Velocity::VelocityUnits::FEET_PER_MIN );
				drawIntrudingAircraft(&intruderPos, &intrVvel, &userHeading, &gaugeCenterPos, &range, threatClass);
			}
		}
	}

	drawInnerGaugeVelocityRing();

	drawVerticalVelocityNeedle(userAircraftVertVel);

	// Turn off Alpha Blending and turn on Depth Testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 0/*AlphaBlending*/, 1/*DepthTesting*/, 0/*DepthWriting*/);

	glPopMatrix();
	glFlush();
}

void VSIGaugeRenderer::drawIntrudingAircraft(LLA const * const intruderPos, Velocity const * const intruderVvel, Angle const * const userHeading, LLA const * const gaugeCenterPos, Distance const * const range, Aircraft::ThreatClassification threatClass) const {
	Angle bearing = gaugeCenterPos->bearing(intruderPos);
	// These two calls line makes the gauge display intruders relative to the user's heading
	bearing = bearing - *userHeading;
	bearing.normalize();

	double rangeOverMaxRangeRatio = range->toFeet() / kGaugeInnerCircleRadius_.toFeet();
	double pixelOffset = rangeOverMaxRangeRatio * kGaugeInnerCircleRadiusPxls_;

	Angle cartesianAngle = Angle::bearingToCartesianAngle(&bearing);
	double pixelOffsetX = cos(cartesianAngle.toRadians()) * pixelOffset;
	double pixelOffsetY = sin(cartesianAngle.toRadians()) * pixelOffset;

	double symbolCenterX = kGaugeCenterX_ + pixelOffsetX;
	double symbolCenterY = kGaugeCenterY_ + pixelOffsetY;

	// The symbols are contained in 16x16 pixel squares in the texture so to find the vertices, 
	// calculate the sides of the 16 px square centered around the symbol center
	double symbolLeft = symbolCenterX - 8.0;
	double symbolRight = symbolCenterX + 8.0;
	double symbolBot = symbolCenterY - 8.0;
	double symbolTop = symbolCenterY + 8.0;

	vsitextureconstants::TexCoords const * symbolCoords = aircraftSymbolFromThreatClassification(threatClass);
	drawTextureRegion(symbolCoords, symbolLeft, symbolRight, symbolTop, symbolBot);

	Distance altitudeDifference = intruderPos->altitude - gaugeCenterPos->altitude;
	int altDiffHundredsFtPerMin = (int) std::round(altitudeDifference.toFeet() / 100.0);

	vsitextureconstants::TexCoords const * signChar = altDiffHundredsFtPerMin < 0.0 ? &vsitextureconstants::K_CHAR_MINUS_SIGN : &vsitextureconstants::K_CHAR_PLUS_SIGN;
	altDiffHundredsFtPerMin = abs(altDiffHundredsFtPerMin);
	std::string altString = std::to_string(altDiffHundredsFtPerMin);

	// Characters in the GaugeTex256 texture are 6x10 (6 px wide, 10 px tall)
	double altStringTop = symbolBot;
	double altStringBot = altStringTop - 10.0;
	double altStringLeft = symbolLeft;
	double altStringRight = altStringLeft + 6.0;

	vsitextureconstants::GlRgb8Color const * symbolColor = symbolColorFromThreatClassification(threatClass);
	glColor3f(symbolColor->red, symbolColor->green, symbolColor->blue);

	drawTextureRegion(signChar, altStringLeft, altStringRight, altStringTop, altStringBot);
	altStringLeft = altStringRight;
	altStringRight += 6.0;

	for (char c : altString) {
		vsitextureconstants::TexCoords const * charCoords = gaugeTexCoordsFromDigitCharacter(c);
		drawTextureRegion(charCoords, altStringLeft, altStringRight, altStringTop, altStringBot);
		altStringLeft = altStringRight;
		altStringRight += 6.0;
	}

	vsitextureconstants::TexCoords const * vvelArrowCoords = intruderVvel->toFeetPerMin() < 0.0 ? &vsitextureconstants::K_VERT_ARROW_DOWN : &vsitextureconstants::K_VERT_ARROW_UP;
	// The vertical arrow is 5 px wide by 13 px tall; the arrow is usually? centered vertically wrt to the symbol so the arrow should be drawn some amount lower than the top of the symbol
	drawTextureRegion(vvelArrowCoords, symbolRight, symbolRight + 5.0, symbolTop - 4.0, symbolTop - 17.0);

	glColor3f(1.0f, 1.0f, 1.0f);
}

vsitextureconstants::TexCoords const * VSIGaugeRenderer::gaugeTexCoordsFromDigitCharacter(char c) const {
	switch (c) {
	case '0':
		return &vsitextureconstants::K_CHAR_ZERO;
	case '1':
		return &vsitextureconstants::K_CHAR_ONE;
	case '2':
		return &vsitextureconstants::K_CHAR_TWO;
	case '3':
		return &vsitextureconstants::K_CHAR_THREE;
	case '4':
		return &vsitextureconstants::K_CHAR_FOUR;
	case '5':
		return &vsitextureconstants::K_CHAR_FIVE;
	case '6':
		return &vsitextureconstants::K_CHAR_SIX;
	case '7':
		return &vsitextureconstants::K_CHAR_SEVEN;
	case '8':
		return &vsitextureconstants::K_CHAR_EIGHT;
	case '9':
		return &vsitextureconstants::K_CHAR_NINE;
	default:
		char debugBuf[128];
		snprintf(debugBuf, 128, "VSIGaugeRenderer::GaugeTexCoordsFromDigitCharacter - unhandled character %c supplied\n", c);

		return &vsitextureconstants::K_DEBUG_SYMBOL;
	}
}

void VSIGaugeRenderer::drawTextureRegion(vsitextureconstants::TexCoords const * texCoords,
	double vertLeft, double vertRight, double vertTop, double vertBot) const {
	glBegin(GL_QUADS);
	glTexCoord2d(texCoords->right, texCoords->bottom); glVertex2d(vertRight, vertBot);
	glTexCoord2d(texCoords->left, texCoords->bottom); glVertex2d(vertLeft, vertBot);
	glTexCoord2d(texCoords->left, texCoords->top); glVertex2d(vertLeft, vertTop);
	glTexCoord2d(texCoords->right, texCoords->top); glVertex2d(vertRight, vertTop);
	glEnd();
}

vsitextureconstants::TexCoords const * VSIGaugeRenderer::aircraftSymbolFromThreatClassification(Aircraft::ThreatClassification threatClass) {
	switch (threatClass) {
	case Aircraft::ThreatClassification::NON_THREAT_TRAFFIC:
		return &vsitextureconstants::K_SYMBOL_BLUE_DIAMOND_CUTOUT;
	case Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC:
		return &vsitextureconstants::K_SYMBOL_BLUE_DIAMOND_WHOLE;
	case Aircraft::ThreatClassification::TRAFFIC_ADVISORY:
		return &vsitextureconstants::K_SYMBOL_YELLOW_CIRCLE;
	case Aircraft::ThreatClassification::RESOLUTION_ADVISORY:
		return &vsitextureconstants::K_SYMBOL_RED_SQUARE;
	default:
		XPLMDebugString("VSIGaugeRenderer::AircraftSymbolFromThreatClassification - Unknown threat classification.\n");
		return &vsitextureconstants::K_DEBUG_SYMBOL;
	}
}

vsitextureconstants::GlRgb8Color const * VSIGaugeRenderer::symbolColorFromThreatClassification(Aircraft::ThreatClassification threatClass) {
	switch (threatClass) {
	case Aircraft::ThreatClassification::NON_THREAT_TRAFFIC:
	case Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC:
		return &vsitextureconstants::K_SYMBOL_BLUE_DIAMOND_COLOR;
	case Aircraft::ThreatClassification::TRAFFIC_ADVISORY:
		return &vsitextureconstants::K_SYMBOL_YELLOW_CIRCLE_COLOR;
	case Aircraft::ThreatClassification::RESOLUTION_ADVISORY:
		return &vsitextureconstants::K_SYMBOL_RED_SQUARE_COLOR;
	default:
		XPLMDebugString("VSIGaugeRenderer::SymbolColorFromThreatClassification - Unknown threat classification.\n");
		return &vsitextureconstants::K_RECOMMENDATION_RANGE_POSITIVE;
	}
}

void VSIGaugeRenderer::drawOuterGauge() const {
	drawTextureRegion(&vsitextureconstants::K_OUTER_GAUGE, kGaugePosLeft_, kGaugePosRight_, kGaugePosTop_, kGaugePosBot_);
}

void VSIGaugeRenderer::drawInnerGaugeVelocityRing() const {
	drawTextureRegion(&vsitextureconstants::K_INNER_GAUGE, kGaugePosLeft_, kGaugePosRight_, kGaugePosTop_, kGaugePosBot_);
}

void VSIGaugeRenderer::drawVerticalVelocityNeedle(Velocity const userAircraftVertVel) const {
	// Translate the needle so it's properly rotated in place about the gauge center
	glTranslatef(kNeedleTranslationX_, kNeedleTranslationY_, 0.0f);

	// Rotate the needle according to the current vertical velocity - 4,000 ft/min is 150 degree rotation relative to 0 ft/min
	double vertSpeedDeg = (userAircraftVertVel.toFeetPerMin() / kMaxVertSpeed) * kMaxVSpeedDegrees - kGlAngleOffset_;
	vertSpeedDeg = mathutil::clampd(vertSpeedDeg, -240.0, 60.0);
	glRotated(vertSpeedDeg, 0.0, 0.0, -1.0);

	// Translate the needle back so it's in the gauge center
	glTranslatef(-kNeedleTranslationX_, -kNeedleTranslationY_, 0.0f);

	glBlendFunc(GL_DST_COLOR, GL_ZERO);

	// Draw Needle Mask
	XPLMBindTexture2d(glTextures_[vsitextureconstants::NEEDLE_MASK_ID], 0);
	drawTextureRegion(&vsitextureconstants::K_NEEDLE_MASK, kNeedlePosLeft_, kNeedlePosRight_, kNeedlePosTop_, kNeedlePosBot_);

	glBlendFunc(GL_ONE, GL_ONE);

	// Draw Needle
	XPLMBindTexture2d(glTextures_[vsitextureconstants::NEEDLE_ID], 0);
	drawTextureRegion(&vsitextureconstants::K_NEEDLE, kNeedlePosLeft_, kNeedlePosRight_, kNeedlePosTop_, kNeedlePosBot_);
}

void VSIGaugeRenderer::drawRecommendationRange(RecommendationRange* recRange, bool recommended) const {
	drawRecommendedVerticalSpeedRange(recRange->minVerticalSpeed, recRange->maxVerticalSpeed, recommended);
}

void VSIGaugeRenderer::drawRecommendedVerticalSpeedRange(Velocity minVertical, Velocity maxVertical, bool recommended) const {
	double minVert = mathutil::clampd(minVertical.toFeetPerMin(), kMinVertSpeed, kMaxVertSpeed);
	double maxVert = mathutil::clampd(maxVertical.toFeetPerMin(), kMinVertSpeed, kMaxVertSpeed);

	double startAngle = (minVert / kMaxVertSpeed) * kMaxVSpeedDegrees - kGlAngleOffset_;
	double stopAngle = (maxVert / kMaxVertSpeed) * kMaxVSpeedDegrees - kGlAngleOffset_;

	drawRecommendationRangeStartStop(Angle(startAngle, Angle::AngleUnits::DEGREES), Angle(stopAngle, Angle::AngleUnits::DEGREES), recommended);
}

void VSIGaugeRenderer::drawRecommendationRangeStartStop(Angle start, Angle stop, bool recommended) const {
	start.normalize();
	stop.normalize();

	/* The start and stop angles are specifying a clockwise arc that winds around the circle centered on the gauge
	so if the start angle is greater than the stop angle, we will perform the clockwise rotation that would result by swapping 
	the arguments instead of allowing a counter-clockwise rotation to happen, which is guaranteed to wrap around the entire gauge.*/
	Angle sweepSize = stop - start;
	if (start > stop) {
		/*Angle min = stop;
		stop = start;
		start = min;*/
		sweepSize = Angle(stop.toDegrees() + (360 - start.toDegrees()), Angle::AngleUnits::DEGREES);
	}

	drawRecommendationRangeStartSweep(start, sweepSize, recommended);
}

void VSIGaugeRenderer::drawRecommendationRangeStartSweep(Angle start, Angle sweep, bool recommended) const { //Recommended appears to be true for pull up, false for go down
	start.normalize();
	sweep.normalize();
	
	glPushMatrix();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	vsitextureconstants::GlRgb8Color const * recRangeColor;
	if (!hostile) {
		 recRangeColor= recommended ?
			&vsitextureconstants::K_RECOMMENDATION_RANGE_POSITIVE : &vsitextureconstants::K_RECOMMENDATION_RANGE_NEGATIVE;
	} if (hostile) {
		recRangeColor = recommended ?
			&vsitextureconstants::K_RECOMMENDATION_RANGE_POSITIVE_INVERTED : &vsitextureconstants::K_RECOMMENDATION_RANGE_NEGATIVE_INVERTED;
	}

	glColor4f(recRangeColor->red, recRangeColor->green, recRangeColor->blue, 1.0f);

	glLoadIdentity();
	glTranslatef(kGaugeCenterX_, kGaugeCenterY_, 0.0f);
	gluPartialDisk(quadric_, kDiskInnerRadius_, kDiskOuterRadius_, kDiskSlices_, kDiskLoops_, start.toDegrees(), sweep.toDegrees());

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);

	glPopMatrix();
}

void VSIGaugeRenderer::markHostile() {
	hostile = !hostile;
}

bool VSIGaugeRenderer::returnHostileValue() {
	return hostile;
}