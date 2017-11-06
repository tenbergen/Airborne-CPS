#include "GaugeRenderer.h"

const double GaugeRenderer::kMillisecondsPerSecond_ = 60000.0;

const double GaugeRenderer::kGaugeInnerCircleRadiusPxls_ = 150.0;
Distance const GaugeRenderer::kGaugeInnerCircleRadius_ { 30.0 , Distance::DistanceUnits::NMI };
Distance const GaugeRenderer::kAircraftToGaugeCenterOffset_ { (28.0 / (2.0 * kGaugeInnerCircleRadiusPxls_)) * kGaugeInnerCircleRadius_.toFeet() * 2.0, Distance::DistanceUnits::FEET};

float const GaugeRenderer::kGaugePosLeft_ = 1024.0f;
float const GaugeRenderer::kGaugePosRight_ = 1280.0f;
float const GaugeRenderer::kGaugePosBot_ = 0.0f;
float const GaugeRenderer::kGaugePosTop_ = 256.0f;

float const GaugeRenderer::kGaugeCenterX_ = (kGaugePosRight_ + kGaugePosLeft_) / 2.0f;
float const GaugeRenderer::kGaugeCenterY_ = (kGaugePosTop_ + kGaugePosBot_) / 2.0f;

float const GaugeRenderer::kNeedlePosLeft_ = kGaugePosLeft_ + 125.0f;
float const GaugeRenderer::kNeedlePosRight_ = kNeedlePosLeft_ + 8.0f;
float const GaugeRenderer::kNeedlePosBot_ = kGaugePosBot_ + 123.0f;
float const GaugeRenderer::kNeedlePosTop_ = kNeedlePosBot_ + 80.0f;

double const GaugeRenderer::kMinVertSpeed = -4000.0;
double const GaugeRenderer::kMaxVertSpeed = 4000.0;

double const GaugeRenderer::kMaxVSpeedDegrees = 150.0;
float const GaugeRenderer::kGlAngleOffset_ = 90.0f;

float const GaugeRenderer::kNeedleTranslationX_ = kNeedlePosLeft_ + ((kNeedlePosRight_ - kNeedlePosLeft_) / 2.0f);
float const GaugeRenderer::kNeedleTranslationY_ = kNeedlePosBot_ + 5.0f;

double const GaugeRenderer::kDiskInnerRadius_ = 75.0;
double const GaugeRenderer::kDiskOuterRadius_ = 105.0;
int const GaugeRenderer::kDiskSlices_ = 32;
int const GaugeRenderer::kDiskLoops_ = 2;

GaugeRenderer::GaugeRenderer(char const * const appPath, Decider * const decider, Aircraft * const userAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> * const intrudingAircraft) : 
	appPath_(appPath), decider_(decider), userAircraft_(userAircraft), intruders_(intrudingAircraft) {
	quadric_ = gluNewQuadric();

	gluQuadricNormals(quadric_, GLU_SMOOTH);
	gluQuadricDrawStyle(quadric_, GLU_FILL);
	gluQuadricTexture(quadric_, GLU_FALSE);
	gluQuadricOrientation(quadric_, GLU_INSIDE);
}

GaugeRenderer::~GaugeRenderer() {
	gluDeleteQuadric(quadric_);
}

void GaugeRenderer::loadTextures()
{
	char fNameBuf[256];

	for (int texId = textureconstants::GAUGE_ID; texId < textureconstants::kNumTextures; texId++) {
		strutil::buildFilePath(fNameBuf, textureconstants::kGaugeFileNames[texId], appPath_);
		
		if (strlen(fNameBuf) > 0 && !loadTexture(fNameBuf, texId)) {
			char debugBuf[256];
			snprintf(fNameBuf, 256, "GaugeRenderer::LoadTextures - failed to load texture at: %s\n", fNameBuf);
			XPLMDebugString(debugBuf);
		}
	}
}


bool GaugeRenderer::loadTexture(char* texPath, int texId) const {
	bool loadedSuccessfully = false;

	BmpLoader::IMAGEDATA sImageData;
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

void GaugeRenderer::render(textureconstants::GlRgb8Color cockpitLighting) {
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
	XPLMBindTexture2d(glTextures_[textureconstants::GAUGE_ID], 0);
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

void GaugeRenderer::drawIntrudingAircraft(LLA const * const intruderPos, Velocity const * const intruderVvel, Angle const * const userHeading, LLA const * const gaugeCenterPos, Distance const * const range, Aircraft::ThreatClassification threatClass) const {
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

	textureconstants::TexCoords const * symbolCoords = aircraftSymbolFromThreatClassification(threatClass);
	drawTextureRegion(symbolCoords, symbolLeft, symbolRight, symbolTop, symbolBot);

	Distance altitudeDifference = intruderPos->altitude - gaugeCenterPos->altitude;
	int altDiffHundredsFtPerMin = (int) std::round(altitudeDifference.toFeet() / 100.0);

	textureconstants::TexCoords const * signChar = altDiffHundredsFtPerMin < 0.0 ? &textureconstants::kCharMinusSign : &textureconstants::kCharPlusSign;
	altDiffHundredsFtPerMin = abs(altDiffHundredsFtPerMin);
	std::string altString = std::to_string(altDiffHundredsFtPerMin);

	// Characters in the GaugeTex256 texture are 6x10 (6 px wide, 10 px tall)
	double altStringTop = symbolBot;
	double altStringBot = altStringTop - 10.0;
	double altStringLeft = symbolLeft;
	double altStringRight = altStringLeft + 6.0;

	textureconstants::GlRgb8Color const * symbolColor = symbolColorFromThreatClassification(threatClass);
	glColor3f(symbolColor->red, symbolColor->green, symbolColor->blue);

	drawTextureRegion(signChar, altStringLeft, altStringRight, altStringTop, altStringBot);
	altStringLeft = altStringRight;
	altStringRight += 6.0;

	for (char c : altString) {
		textureconstants::TexCoords const * charCoords = gaugeTexCoordsFromDigitCharacter(c);
		drawTextureRegion(charCoords, altStringLeft, altStringRight, altStringTop, altStringBot);
		altStringLeft = altStringRight;
		altStringRight += 6.0;
	}

	textureconstants::TexCoords const * vvelArrowCoords = intruderVvel->toFeetPerMin() < 0.0 ? &textureconstants::kVertArrowDown : &textureconstants::kVertArrowUp;
	// The vertical arrow is 5 px wide by 13 px tall; the arrow is usually? centered vertically wrt to the symbol so the arrow should be drawn some amount lower than the top of the symbol
	drawTextureRegion(vvelArrowCoords, symbolRight, symbolRight + 5.0, symbolTop - 4.0, symbolTop - 17.0);

	glColor3f(1.0f, 1.0f, 1.0f);
}

textureconstants::TexCoords const * GaugeRenderer::gaugeTexCoordsFromDigitCharacter(char c) const {
	switch (c) {
	case '0':
		return &textureconstants::kCharZero;
	case '1':
		return &textureconstants::kCharOne;
	case '2':
		return &textureconstants::kCharTwo;
	case '3':
		return &textureconstants::kCharThree;
	case '4':
		return &textureconstants::kCharFour;
	case '5':
		return &textureconstants::kCharFive;
	case '6':
		return &textureconstants::kCharSix;
	case '7':
		return &textureconstants::kCharSeven;
	case '8':
		return &textureconstants::kCharEight;
	case '9':
		return &textureconstants::kCharNine;
	default:
		char debugBuf[128];
		snprintf(debugBuf, 128, "GaugeRenderer::GaugeTexCoordsFromDigitCharacter - unhandled character %c supplied\n", c);

		return &textureconstants::kDebugSymbol;
	}
}

void GaugeRenderer::drawTextureRegion(textureconstants::TexCoords const * texCoords, 
	double vertLeft, double vertRight, double vertTop, double vertBot) const {
	glBegin(GL_QUADS);
	glTexCoord2d(texCoords->right, texCoords->bottom); glVertex2d(vertRight, vertBot);
	glTexCoord2d(texCoords->left, texCoords->bottom); glVertex2d(vertLeft, vertBot);
	glTexCoord2d(texCoords->left, texCoords->top); glVertex2d(vertLeft, vertTop);
	glTexCoord2d(texCoords->right, texCoords->top); glVertex2d(vertRight, vertTop);
	glEnd();
}

textureconstants::TexCoords const * GaugeRenderer::aircraftSymbolFromThreatClassification(Aircraft::ThreatClassification threatClass) {
	switch (threatClass) {
	case Aircraft::ThreatClassification::NON_THREAT_TRAFFIC:
		return &textureconstants::kSymbolBlueDiamondCutout;
	case Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC:
		return &textureconstants::kSymbolBlueDiamondWhole;
	case Aircraft::ThreatClassification::TRAFFIC_ADVISORY:
		return &textureconstants::kSymbolYellowCircle;
	case Aircraft::ThreatClassification::RESOLUTION_ADVISORY:
		return &textureconstants::kSymbolRedSquare;
	default:
		XPLMDebugString("GaugeRenderer::AircraftSymbolFromThreatClassification - Unknown threat classification.\n");
		return &textureconstants::kDebugSymbol;
	}
}

textureconstants::GlRgb8Color const * GaugeRenderer::symbolColorFromThreatClassification(Aircraft::ThreatClassification threatClass) {
	switch (threatClass) {
	case Aircraft::ThreatClassification::NON_THREAT_TRAFFIC:
	case Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC:
		return &textureconstants::kSymbolBlueDiamondColor;
	case Aircraft::ThreatClassification::TRAFFIC_ADVISORY:
		return &textureconstants::kSymbolYellowCircleColor;
	case Aircraft::ThreatClassification::RESOLUTION_ADVISORY:
		return &textureconstants::kSymbolRedSquareColor;
	default:
		XPLMDebugString("GaugeRenderer::SymbolColorFromThreatClassification - Unknown threat classification.\n");
		return &textureconstants::kRecommendationRangePositive;
	}
}

void GaugeRenderer::drawOuterGauge() const {
	drawTextureRegion(&textureconstants::kOuterGauge, kGaugePosLeft_, kGaugePosRight_, kGaugePosTop_, kGaugePosBot_);
}

void GaugeRenderer::drawInnerGaugeVelocityRing() const {
	drawTextureRegion(&textureconstants::kInnerGauge, kGaugePosLeft_, kGaugePosRight_, kGaugePosTop_, kGaugePosBot_);
}

void GaugeRenderer::drawVerticalVelocityNeedle(Velocity const userAircraftVertVel) const {
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
	XPLMBindTexture2d(glTextures_[textureconstants::NEEDLE_MASK_ID], 0);
	drawTextureRegion(&textureconstants::kNeedleMask, kNeedlePosLeft_, kNeedlePosRight_, kNeedlePosTop_, kNeedlePosBot_);

	glBlendFunc(GL_ONE, GL_ONE);

	// Draw Needle
	XPLMBindTexture2d(glTextures_[textureconstants::NEEDLE_ID], 0);
	drawTextureRegion(&textureconstants::kNeedle, kNeedlePosLeft_, kNeedlePosRight_, kNeedlePosTop_, kNeedlePosBot_);
}

void GaugeRenderer::drawRecommendationRange(RecommendationRange* recRange, bool recommended) const {
	drawRecommendedVerticalSpeedRange(recRange->minVerticalSpeed, recRange->maxVerticalSpeed, recommended);
}

void GaugeRenderer::drawRecommendedVerticalSpeedRange(Velocity minVertical, Velocity maxVertical, bool recommended) const {
	double minVert = mathutil::clampd(minVertical.toFeetPerMin(), kMinVertSpeed, kMaxVertSpeed);
	double maxVert = mathutil::clampd(maxVertical.toFeetPerMin(), kMinVertSpeed, kMaxVertSpeed);

	double startAngle = (minVert / kMaxVertSpeed) * kMaxVSpeedDegrees - kGlAngleOffset_;
	double stopAngle = (maxVert / kMaxVertSpeed) * kMaxVSpeedDegrees - kGlAngleOffset_;

	drawRecommendationRangeStartStop(Angle(startAngle, Angle::AngleUnits::DEGREES), Angle(stopAngle, Angle::AngleUnits::DEGREES), recommended);
}

void GaugeRenderer::drawRecommendationRangeStartStop(Angle start, Angle stop, bool recommended) const {
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

void GaugeRenderer::drawRecommendationRangeStartSweep(Angle start, Angle sweep, bool recommended) const {
	start.normalize();
	sweep.normalize();
	
	glPushMatrix();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	textureconstants::GlRgb8Color const * recRangeColor = recommended ? 
		&textureconstants::kRecommendationRangePositive : &textureconstants::kRecommendationRangeNegative;

	glColor4f(recRangeColor->red, recRangeColor->green, recRangeColor->blue, 1.0f);

	glLoadIdentity();
	glTranslatef(kGaugeCenterX_, kGaugeCenterY_, 0.0f);
	gluPartialDisk(quadric_, kDiskInnerRadius_, kDiskOuterRadius_, kDiskSlices_, kDiskLoops_, start.toDegrees(), sweep.toDegrees());

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);

	glPopMatrix();
}