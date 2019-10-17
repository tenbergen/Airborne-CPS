#include "ArtHorizGaugeRenderer.h"


const double AHGaugeRenderer::millisecondsPerMinute_ = 60000.0;

float const AHGaugeRenderer::gaugePosLeft_ = 1024.0f;
float const AHGaugeRenderer::gaugePosRight_ = 1280.0f;
float const AHGaugeRenderer::gaugePosBot_ = 0.0f;
float const AHGaugeRenderer::gaugePosTop_ = 256.0f;

/*
* Note that bot the outer and inner gauge are intended to be squares, so the width is equal to the height.
* The gauge corner is a small square used to "round" the edges of the inner gauge somewhat.
* */
float const AHGaugeRenderer::paddingHeight = 35.0f;
float const AHGaugeRenderer::outerGaugeHeight = gaugePosTop_ - gaugePosBot_;
float const AHGaugeRenderer::innerGaugeHeight = (outerGaugeHeight)*5.0f / 9.0f;
float const AHGaugeRenderer::outerGaugeWidthRight = ((outerGaugeHeight / 2.0f) >= (innerGaugeHeight / 2.0f + 2.0f*paddingHeight)) ? (outerGaugeHeight / 2.0f) : (innerGaugeHeight / 2.0f + 2.0f*paddingHeight);
float const AHGaugeRenderer::outerGaugeWidthLeft = ((outerGaugeHeight / 2.0f) >= (innerGaugeHeight / 2.0f + paddingHeight)) ? (outerGaugeHeight / 2.0f) : (innerGaugeHeight / 2.0f + paddingHeight);
float const AHGaugeRenderer::artHorizHeight = 0.9f*innerGaugeHeight*sqrt(2.0f);
float const AHGaugeRenderer::gaugeCornerHeight = 0.1f*innerGaugeHeight*sqrt(2.0f);

float const AHGaugeRenderer::gaugeCenterX_ = (gaugePosRight_ + gaugePosLeft_) / 2.0f + paddingHeight;
float const AHGaugeRenderer::gaugeCenterY_ = (gaugePosTop_ + gaugePosBot_) / 2.0f;

/*
* Note that 180 degrees are viewable on the gauge at any given time for the elevation or pitch of the plane; 90 degrees
* above and 90 degrees below the current pitch.
* Angular divisions will be placed along the y axis relative to the plane at a spacing of dyHorizon in pixel values,
* and a spacing of separationAngleY degrees.
* Graduations will also be placed at both smallSeparationAngleTheta and largeSeparationAngleTheta degrees along the
* perimeter of the gauge to give an indication of the roll of the plane.
* */
float const AHGaugeRenderer::gaugeViewingAngle = 180.0f;
float const AHGaugeRenderer::separationAngleY = 10.0f;
float const AHGaugeRenderer::dyHorizon = (gaugePosTop_ - gaugePosBot_) / (gaugeViewingAngle / separationAngleY);
float const AHGaugeRenderer::PI = 3.1415927f;
float const AHGaugeRenderer::smallSeparationAngleTheta = 5.0f;
float const AHGaugeRenderer::largeSeparationAngleTheta = 15.0f;
float const AHGaugeRenderer::dTheta_small = smallSeparationAngleTheta * PI / 180.0f;
float const AHGaugeRenderer::dTheta_large = largeSeparationAngleTheta * PI / 180.0f;
float const AHGaugeRenderer::smallLineLength = 5.0f;
float const AHGaugeRenderer::largeLineLength = 10.0f;
float const AHGaugeRenderer::lineWidth = 2.0f;
float const AHGaugeRenderer::zeroDegreesIndicatorHeight = 10.0f;
float const AHGaugeRenderer::zeroDegreesIndicatorWidth = 5.0f;
float const AHGaugeRenderer::planeRollIndicatorHeight = 10.0f;
float const AHGaugeRenderer::planeRollIndicatorWidth = 5.0f;

/*
* Note that rvs stands for recommended vertical speed. These outline the perimeter of the RVS Gauge.
* */
float const AHGaugeRenderer::rvsLeft = (outerGaugeWidthRight - innerGaugeHeight / 2.0f) / 2.0f + innerGaugeHeight / 2.0f + 5.0f;
float const AHGaugeRenderer::rvsRight = outerGaugeWidthRight - 5.0f;
float const AHGaugeRenderer::rvsTopHigh = innerGaugeHeight / 2.0f;
float const AHGaugeRenderer::rvsTopLow = rvsTopHigh - (rvsRight - rvsLeft)*tan(PI / 6.0);
float const AHGaugeRenderer::rvsBottomLow = -innerGaugeHeight / 2.0f;
float const AHGaugeRenderer::rvsBottomHigh = rvsBottomLow + (rvsRight - rvsLeft)*tan(PI / 6.0);
float const AHGaugeRenderer::fpmToPixelsLeft = abs(rvsTopHigh - rvsBottomLow) / abs(maxVertSpeed - minVertSpeed);
float const AHGaugeRenderer::fpmToPixelsRight = abs(rvsTopLow - rvsBottomHigh) / abs(maxVertSpeed - minVertSpeed);
float const AHGaugeRenderer::rvsDivisions = 16;

/*
* Constant parameters for the TCAS's altimeter.
* */
float const AHGaugeRenderer::altimeterLeft = innerGaugeHeight / 2.0f + 5.0f;
float const AHGaugeRenderer::altimeterRight = (outerGaugeWidthRight - innerGaugeHeight / 2.0f) / 2.0f + innerGaugeHeight / 2.0f - 5.0f;
float const AHGaugeRenderer::altimeterTop = innerGaugeHeight / 2.0f;
float const AHGaugeRenderer::altimeterBottom = -innerGaugeHeight / 2.0f;
float const AHGaugeRenderer::altimeterDisplayLeft = altimeterLeft + 5.0f;
float const AHGaugeRenderer::altimeterDisplayMid = altimeterRight;
float const AHGaugeRenderer::altimeterDisplayRight = altimeterRight + 7.0f;
float const AHGaugeRenderer::altimeterDisplayTopLow = .1f*innerGaugeHeight / 2.0f;
float const AHGaugeRenderer::altimeterDisplayTopHigh = altimeterDisplayTopLow + 3.0f;
float const AHGaugeRenderer::altimeterDisplayBottomHigh = -altimeterDisplayTopLow;
float const AHGaugeRenderer::altimeterDisplayBottomLow = -altimeterDisplayTopHigh;
float const AHGaugeRenderer::altimeterDisplayTriangleLeft = altimeterLeft;
float const AHGaugeRenderer::altimeterDisplayTriangleRight = altimeterDisplayLeft;
float const AHGaugeRenderer::altimeterDisplayTriangleTop = .5f*altimeterDisplayTopLow;
float const AHGaugeRenderer::altimeterDisplayTriangleMid = 0.0f;
float const AHGaugeRenderer::altimeterDisplayTriangleBottom = -altimeterDisplayTriangleTop;
float const AHGaugeRenderer::altimeterRange = 1000.0f;
float const AHGaugeRenderer::altimeterDivisions = 10;
float const AHGaugeRenderer::da = altimeterRange / altimeterDivisions;
float const AHGaugeRenderer::altToPixels = (altimeterTop - altimeterBottom) / (altimeterRange);
float const AHGaugeRenderer::dyAltimeter = da*altToPixels;

/*
* Constant parameters for the TCAS's airspeed gauge.
* */
float const AHGaugeRenderer::airspeedGaugeLeft = -innerGaugeHeight / 2.0f - paddingHeight + 5.0f;
float const AHGaugeRenderer::airspeedGaugeRight = -innerGaugeHeight / 2.0f - 5.0f;
float const AHGaugeRenderer::airspeedGaugeTop = innerGaugeHeight / 2.0f;
float const AHGaugeRenderer::airspeedGaugeBottom = -innerGaugeHeight / 2.0f;
float const AHGaugeRenderer::airspeedGaugeDisplayLeft = airspeedGaugeLeft - 7.0f;
float const AHGaugeRenderer::airspeedGaugeDisplayMid = airspeedGaugeLeft;
float const AHGaugeRenderer::airspeedGaugeDisplayRight = airspeedGaugeRight - 5.0f;
float const AHGaugeRenderer::airspeedGaugeDisplayTopLow = .1f*innerGaugeHeight / 2.0f;
float const AHGaugeRenderer::airspeedGaugeDisplayTopHigh = airspeedGaugeDisplayTopLow + 3.0f;
float const AHGaugeRenderer::airspeedGaugeDisplayBottomHigh = -airspeedGaugeDisplayTopLow;
float const AHGaugeRenderer::airspeedGaugeDisplayBottomLow = -airspeedGaugeDisplayTopHigh;
float const AHGaugeRenderer::airspeedGaugeDisplayTriangleLeft = airspeedGaugeDisplayRight;
float const AHGaugeRenderer::airspeedGaugeDisplayTriangleRight = airspeedGaugeRight;
float const AHGaugeRenderer::airspeedGaugeDisplayTriangleTop = .5f*airspeedGaugeDisplayTopLow;
float const AHGaugeRenderer::airspeedGaugeDisplayTriangleMid = 0.0f;
float const AHGaugeRenderer::airspeedGaugeDisplayTriangleBottom = -airspeedGaugeDisplayTriangleTop;
float const AHGaugeRenderer::airspeedGaugeRange = 100.0f;
float const AHGaugeRenderer::airspeedGaugeDivisions = 10;
float const AHGaugeRenderer::ds = airspeedGaugeRange / airspeedGaugeDivisions;
float const AHGaugeRenderer::airspeedToPixels = (airspeedGaugeTop - airspeedGaugeBottom) / (airspeedGaugeRange);
float const AHGaugeRenderer::dyAirspeedGauge = ds*airspeedToPixels;

/*
* FIXME -- Change radius to innerGaugeHeight?
* */
const double AHGaugeRenderer::gaugeInnerCircleRadiusPxls_ = 150.0;
Distance const AHGaugeRenderer::gaugeInnerCircleRadius_{ 30.0 , Distance::DistanceUnits::NMI };
Distance const AHGaugeRenderer::aircraftToGaugeCenterOffset_{ (28.0 / (2.0 * gaugeInnerCircleRadiusPxls_)) * gaugeInnerCircleRadius_.toFeet() * 2.0, Distance::DistanceUnits::FEET };


Distance const AHGaugeRenderer::advisoryRadiusNMi{ 30.0 , Distance::DistanceUnits::NMI };
float const AHGaugeRenderer::advisoryRadiusPixels = innerGaugeHeight;
float const AHGaugeRenderer::advisoryViewAngleX = 150.0f;
float const AHGaugeRenderer::advisoryViewAngleY = 135.0f;
float const AHGaugeRenderer::advisoryDomainX = 8000.0f;
float const AHGaugeRenderer::advisoryDomainY = 4000.0f;
float const AHGaugeRenderer::advisoryPixelsX = innerGaugeHeight / 2.0f;
float const AHGaugeRenderer::advisoryPixelsY = innerGaugeHeight / 2.0f;
float const AHGaugeRenderer::advisoryFeetToPixelsX = advisoryDomainX / advisoryRadiusPixels;
float const AHGaugeRenderer::advisoryFeetToPixelsY = advisoryDomainY / advisoryRadiusPixels;



/*
float const AHGaugeRenderer::artHorizHeight = (9.0f * outerGaugeHeight * sqrt(2.0f))/(9.0f*sqrt(2.0f) + 8.0f);
float const AHGaugeRenderer::paddingHeight = (4.0f*artHorizHeight)/(9.0f*sqrt(2.0f));
float const AHGaugeRenderer::innerGaugeHeight = artHorizHeight*sqrt(2.0f) - 2.0f*paddingHeight;
float const AHGaugeRenderer::gaugeCornerHeight = 0.1f*innerGaugeHeight*sqrt(2.0f);
*/


float const AHGaugeRenderer::needlePosLeft_ = gaugePosLeft_ + 125.0f;
float const AHGaugeRenderer::needlePosRight_ = needlePosLeft_ + 8.0f;
float const AHGaugeRenderer::needlePosBot_ = gaugePosBot_ + 123.0f;
float const AHGaugeRenderer::needlePosTop_ = needlePosBot_ + 80.0f;

double const AHGaugeRenderer::minVertSpeed = -4000.0;
double const AHGaugeRenderer::maxVertSpeed = 4000.0;

double const AHGaugeRenderer::maxVSpeedDegrees = 150.0;
float const AHGaugeRenderer::glAngleOffset_ = 90.0f;

float const AHGaugeRenderer::needleTranslationX_ = needlePosLeft_ + ((needlePosRight_ - needlePosLeft_) / 2.0f);
float const AHGaugeRenderer::needleTranslationY_ = needlePosBot_ + 5.0f;

double const AHGaugeRenderer::diskInnerRadius_ = 75.0;
double const AHGaugeRenderer::diskOuterRadius_ = 105.0;
int const AHGaugeRenderer::diskSlices_ = 32;
int const AHGaugeRenderer::diskLoops_ = 2;


AHGaugeRenderer::AHGaugeRenderer(char const * const appPath, Decider * const decider, Aircraft * const userAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> * const intrudingAircraft) :
	appPath_(appPath), decider_(decider), userAircraft_(userAircraft), intruders_(intrudingAircraft) {
	quadric_ = gluNewQuadric();

	gluQuadricNormals(quadric_, GLU_SMOOTH);
	gluQuadricDrawStyle(quadric_, GLU_FILL);
	gluQuadricTexture(quadric_, GLU_FALSE);
	gluQuadricOrientation(quadric_, GLU_INSIDE);
}

AHGaugeRenderer::~AHGaugeRenderer() {
	gluDeleteQuadric(quadric_);
}

void AHGaugeRenderer::loadTextures()
{
	char fNameBuf[256];

	for (int texId = ahtextureconstants::GAUGE_ID; texId < ahtextureconstants::K_NUM_TEXTURES; texId++) {
		strutil::buildFilePath(fNameBuf, ahtextureconstants::K_GAUGE_FILENAMES[texId], appPath_);

		if (strlen(fNameBuf) > 0 && !loadTexture(fNameBuf, texId)) {
			char debugBuf[256];
			snprintf(fNameBuf, 256, "AHGaugeRenderer::LoadTextures - failed to load texture at: %s\n", fNameBuf);
			XPLMDebugString(debugBuf);
		}
	}
}


bool AHGaugeRenderer::loadTexture(char* texPath, int texId) const {
	bool loadedSuccessfully = false;

	BmpLoader::ImageData sImageData;
	/// Get the bitmap from the file
	loadedSuccessfully = BmpLoader::loadBmp(texPath, &sImageData) != 0;

	if (loadedSuccessfully)
	{
		BmpLoader::swapRedBlue(&sImageData);

		/// Do the opengl stuff using XPLM functions for a friendly Xplane existence.

		//FIXME: May need to change texture ID numbers in ArtHorizTextureConstants.hpp
		XPLMGenerateTextureNumbers((int *)&glTextures_[texId], 1);
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

void AHGaugeRenderer::render(ahtextureconstants::GlRgb8Color cockpitLighting) {
	userAircraft_->lock.lock();

	LLA const userPos = userAircraft_->positionCurrent;
	Angle const userHeading = userAircraft_->heading;
	Velocity const userAircraftVerticalVelocity = userAircraft_->verticalVelocity;
	Velocity const trueAirspeed = userAircraft_->trueAirspeed;
	Angle const userTheta = userAircraft_->theta;
	Angle const userPhi = userAircraft_->phi;

	userAircraft_->lock.unlock();

	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 1/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);
	//Enable colors for shading the gauge
	glEnable(GL_COLOR_MATERIAL);

	// Push the MV matrix onto the stack
	glPushMatrix();

	// Turn off the day/night tinting while drawing the inner gauge parts
	// Will have to examine if this should be turned off or not.
	glColor3f(1.0f, 1.0f, 1.0f);
	drawHorizon(userTheta, userPhi);
	drawRecommendedVerticalSpeedGauge(userAircraftVerticalVelocity);
	drawAltimeter(1.0f*userPos.altitude.toFeet());
	drawAirspeedGauge(trueAirspeed.toMph());

	concurrency::concurrent_unordered_map<std::string, Aircraft*>::const_iterator & iter = intruders_->cbegin();

	if (iter != intruders_->cend()) {
		LLA gaugeCenterPos = userPos.translate(&userHeading, &aircraftToGaugeCenterOffset_);

		for (; iter != intruders_->cend(); ++iter) {
			Aircraft* intruder = iter->second;

			intruder->lock.lock();
			LLA const intruderPos = intruder->positionCurrent;
			Distance const altDiff = intruderPos.altitude - intruder->positionOld.altitude;
			std::chrono::milliseconds const altTimeDiff = intruder->positionCurrentTime - intruder->positionOldTime;
			Aircraft::ThreatClassification threatClass = intruder->threatClassification;
			intruder->lock.unlock();

			Distance range = gaugeCenterPos.range(&intruderPos);

			if (range.toFeet() < gaugeInnerCircleRadius_.toFeet() * 2) {
				Velocity const intrVvel = Velocity((altDiff.toFeet() / (double)altTimeDiff.count()) * millisecondsPerMinute_, Velocity::VelocityUnits::FEET_PER_MIN);
				drawIntrudingAircraft(&intruderPos, &intrVvel, &userHeading, &gaugeCenterPos, &range, threatClass);
			}
		}
	}

	/// Color the gauge background according to the day/night coloring inside the cockpit
	glColor3f(cockpitLighting.red, cockpitLighting.green, cockpitLighting.blue);

	// Draw the outer gauge
	//drawOuterGauge();

	// Turn off Alpha Blending and turn on Depth Testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 0/*AlphaBlending*/, 1/*DepthTesting*/, 0/*DepthWriting*/);

	glPopMatrix();
	glFlush();

}

ahtextureconstants::TexCoords const * AHGaugeRenderer::gaugeTexCoordsFromDigitCharacter(char c) const {
	switch (c) {
	case '0':
		return &ahtextureconstants::K_CHAR_ZERO;
	case '1':
		return &ahtextureconstants::K_CHAR_ONE;
	case '2':
		return &ahtextureconstants::K_CHAR_TWO;
	case '3':
		return &ahtextureconstants::K_CHAR_THREE;
	case '4':
		return &ahtextureconstants::K_CHAR_FOUR;
	case '5':
		return &ahtextureconstants::K_CHAR_FIVE;
	case '6':
		return &ahtextureconstants::K_CHAR_SIX;
	case '7':
		return &ahtextureconstants::K_CHAR_SEVEN;
	case '8':
		return &ahtextureconstants::K_CHAR_EIGHT;
	case '9':
		return &ahtextureconstants::K_CHAR_NINE;
	default:
		char debugBuf[128];
		snprintf(debugBuf, 128, "AHGaugeRenderer::GaugeTexCoordsFromDigitCharacter - unhandled character %c supplied\n", c);

		return &ahtextureconstants::K_DEBUG_SYMBOL;
	}
}

void AHGaugeRenderer::drawTextureRegion(ahtextureconstants::TexCoords const * texCoords,
	double vertLeft, double vertRight, double vertTop, double vertBot) const {
	//glPushMatrix();
	//glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_QUADS);
	glTexCoord2d(texCoords->left, texCoords->top); glVertex2d(vertLeft, vertTop);
	glTexCoord2d(texCoords->right, texCoords->top); glVertex2d(vertRight, vertTop);
	glTexCoord2d(texCoords->right, texCoords->bottom); glVertex2d(vertRight, vertBot);
	glTexCoord2d(texCoords->left, texCoords->bottom); glVertex2d(vertLeft, vertBot);
	glEnd();
	//glPopMatrix();
}

ahtextureconstants::TexCoords const * AHGaugeRenderer::aircraftSymbolFromThreatClassification(Aircraft::ThreatClassification threatClass) {
	switch (threatClass) {
	case Aircraft::ThreatClassification::NON_THREAT_TRAFFIC:
		return &ahtextureconstants::K_SYMBOL_BLUE_DIAMOND_CUTOUT;
	case Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC:
		return &ahtextureconstants::K_SYMBOL_BLUE_DIAMOND_WHOLE;
	case Aircraft::ThreatClassification::TRAFFIC_ADVISORY:
		return &ahtextureconstants::K_SYMBOL_YELLOW_CIRCLE;
	case Aircraft::ThreatClassification::RESOLUTION_ADVISORY:
		return &ahtextureconstants::K_SYMBOL_RED_SQUARE;
	default:
		XPLMDebugString("AHGaugeRenderer::AircraftSymbolFromThreatClassification - Unknown threat classification.\n");
		return &ahtextureconstants::K_DEBUG_SYMBOL;
	}
}

ahtextureconstants::GlRgb8Color const * AHGaugeRenderer::symbolColorFromThreatClassification(Aircraft::ThreatClassification threatClass) {
	switch (threatClass) {
	case Aircraft::ThreatClassification::NON_THREAT_TRAFFIC:
	case Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC:
		return &ahtextureconstants::K_SYMBOL_BLUE_DIAMOND_COLOR;
	case Aircraft::ThreatClassification::TRAFFIC_ADVISORY:
		return &ahtextureconstants::K_SYMBOL_YELLOW_CIRCLE_COLOR;
	case Aircraft::ThreatClassification::RESOLUTION_ADVISORY:
		return &ahtextureconstants::K_SYMBOL_RED_SQUARE_COLOR;
	default:
		XPLMDebugString("AHGaugeRenderer::SymbolColorFromThreatClassification - Unknown threat classification.\n");
		return &ahtextureconstants::K_RECOMMENDATION_RANGE_POSITIVE;
	}
}

void AHGaugeRenderer::drawHorizon(Angle theta, Angle phi) const {
	double phiDegrees = phi.toDegrees();
	float gaugeRadius;
	float horizPosMid;
	float y;
	float halfSmallLineLength = smallLineLength / 2.0f;
	float halfLargeLineLength = largeLineLength / 2.0f;
	float startingAngle = PI / 2.0f;
	float cornerHalfDiagonal = 0.5f*gaugeCornerHeight*sqrt(2.0f);
	int count;

	if (((-45.0 <= phiDegrees) && (phiDegrees <= 45.0))
		|| ((-180.0 <= phiDegrees) && (phiDegrees <= -135.0))
		|| ((135.0 <= phiDegrees) && (phiDegrees <= 180.0))) {
		gaugeRadius = abs((innerGaugeHeight / 2.0f) / cos(phi.toRadians()));
	}
	else {
		gaugeRadius = abs((innerGaugeHeight / 2.0f) / sin(phi.toRadians()));
	}

	horizPosMid = gaugeRadius - ((theta.toDegrees() + 90.0f) / 180.0f) * 2.0f*gaugeRadius;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);
	glRotatef(phi.toDegrees(), 0.0f, 0.0f, 1.0f);

	glBegin(GL_QUADS);
	/* Light Blue -- Sky */
	glColor4f(77.0f / 255.0f, 121.0f / 255.0f, 255.0f / 255.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(-artHorizHeight / 2.0f, artHorizHeight / 2.0f);
	glVertex2d(artHorizHeight / 2.0f, artHorizHeight / 2.0f);
	glVertex2d(artHorizHeight / 2.0f, horizPosMid);
	glVertex2d(-artHorizHeight / 2.0f, horizPosMid);

	/* Light Brown -- Ground */
	glColor4f(172.0f / 255.0f, 115.0f / 255.0f, 57.0f / 255.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(-artHorizHeight / 2.0f, horizPosMid);
	glVertex2d(artHorizHeight / 2.0f, horizPosMid);
	glVertex2d(artHorizHeight / 2.0f, -artHorizHeight / 2.0f);
	glVertex2d(-artHorizHeight / 2.0f, -artHorizHeight / 2.0f);

	glEnd();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);
	glRotatef(phi.toDegrees(), 0.0f, 0.0f, 1.0f);

	glBegin(GL_LINES);
	glLineWidth(lineWidth);

	/* White -- Pitch and Roll Angular Indicators */
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(0.0f, artHorizHeight / 2.0f);
	glVertex2d(0.0f, -artHorizHeight / 2.0f);

	y = horizPosMid;
	count = 0;

	while (y <= gaugeRadius) {
		if ((count % 2) == 0) {
			glVertex2d(-halfLargeLineLength, y);
			glVertex2d(halfLargeLineLength, y);
		}
		else {
			glVertex2d(-halfSmallLineLength, y);
			glVertex2d(halfSmallLineLength, y);
		}
		y += dyHorizon;
		count++;
	}

	y = horizPosMid - dyHorizon;
	count = 1;

	while (y >= -gaugeRadius) {
		if ((count % 2) == 0) {
			glVertex2d(-halfLargeLineLength, y);
			glVertex2d(halfLargeLineLength, y);
		}
		else {
			glVertex2d(-halfSmallLineLength, y);
			glVertex2d(halfSmallLineLength, y);
		}
		y -= dyHorizon;
		count++;
	}

	gaugeRadius = innerGaugeHeight / 2.0f;
	count = 1;

	while (count < 7) {
		if ((count % 2) == 0) {
			float angle = startingAngle + dTheta_small*count;
			float x1 = gaugeRadius*cos(angle);
			float y1 = gaugeRadius*sin(angle);
			float x2 = (gaugeRadius - largeLineLength)*cos(angle);
			float y2 = (gaugeRadius - largeLineLength)*sin(angle);

			glVertex2d(x1, y1);
			glVertex2d(x2, y2);

			angle = startingAngle - dTheta_small*count;
			x1 = gaugeRadius*cos(angle);
			y1 = gaugeRadius*sin(angle);
			x2 = (gaugeRadius - largeLineLength)*cos(angle);
			y2 = (gaugeRadius - largeLineLength)*sin(angle);

			glVertex2d(x1, y1);
			glVertex2d(x2, y2);
		}
		else {
			float angle = startingAngle + dTheta_small*count;
			float x1 = gaugeRadius*cos(angle);
			float y1 = gaugeRadius*sin(angle);
			float x2 = (gaugeRadius - smallLineLength)*cos(angle);
			float y2 = (gaugeRadius - smallLineLength)*sin(angle);

			glVertex2d(x1, y1);
			glVertex2d(x2, y2);

			angle = startingAngle - dTheta_small*count;
			x1 = gaugeRadius*cos(angle);
			y1 = gaugeRadius*sin(angle);
			x2 = (gaugeRadius - smallLineLength)*cos(angle);
			y2 = (gaugeRadius - smallLineLength)*sin(angle);

			glVertex2d(x1, y1);
			glVertex2d(x2, y2);
		}
		count++;
	}

	count = 3;

	while (count < 12) {
		float angle = startingAngle + dTheta_large*count;
		float x1 = gaugeRadius*cos(angle);
		float y1 = gaugeRadius*sin(angle);
		float x2 = (gaugeRadius - largeLineLength)*cos(angle);
		float y2 = (gaugeRadius - largeLineLength)*sin(angle);

		glVertex2d(x1, y1);
		glVertex2d(x2, y2);

		angle = startingAngle - dTheta_large*count;
		x1 = gaugeRadius*cos(angle);
		y1 = gaugeRadius*sin(angle);
		x2 = (gaugeRadius - largeLineLength)*cos(angle);
		y2 = (gaugeRadius - largeLineLength)*sin(angle);

		glVertex2d(x1, y1);
		glVertex2d(x2, y2);

		count++;
	}

	glEnd();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_TRIANGLES);

	/* Red -- Plane Roll Angular Indicator */
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	glVertex2d(0.0f, innerGaugeHeight / 2.0f - largeLineLength);
	glVertex2d(planeRollIndicatorWidth / 2.0f, innerGaugeHeight / 2.0f - largeLineLength - planeRollIndicatorHeight);
	glVertex2d(-planeRollIndicatorWidth / 2.0f, innerGaugeHeight / 2.0f - largeLineLength - planeRollIndicatorHeight);

	glEnd();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_QUADS);
	/* Black -- Outer Gauge */
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	/* Left Background */
	glVertex2d(-outerGaugeWidthLeft, outerGaugeHeight / 2.0f);
	glVertex2d(-innerGaugeHeight / 2.0f, outerGaugeHeight / 2.0f);
	glVertex2d(-innerGaugeHeight / 2.0f, -outerGaugeHeight / 2.0f);
	glVertex2d(-outerGaugeWidthLeft, -outerGaugeHeight / 2.0f);

	/* Right Background */
	glVertex2d(innerGaugeHeight / 2.0f, outerGaugeHeight / 2.0f);
	glVertex2d(outerGaugeWidthRight, outerGaugeHeight / 2.0f);
	glVertex2d(outerGaugeWidthRight, -outerGaugeHeight / 2.0f);
	glVertex2d(innerGaugeHeight / 2.0f, -outerGaugeHeight / 2.0f);

	/* Top Background */
	glVertex2d(-innerGaugeHeight / 2.0f, outerGaugeHeight / 2.0f);
	glVertex2d(innerGaugeHeight / 2.0f, outerGaugeHeight / 2.0f);
	glVertex2d(innerGaugeHeight / 2.0f, innerGaugeHeight / 2.0f);
	glVertex2d(-innerGaugeHeight / 2.0f, innerGaugeHeight / 2.0f);

	/* Bottom Background */
	glVertex2d(-innerGaugeHeight / 2.0f, -innerGaugeHeight / 2.0f);
	glVertex2d(innerGaugeHeight / 2.0f, -innerGaugeHeight / 2.0f);
	glVertex2d(innerGaugeHeight / 2.0f, -outerGaugeHeight / 2.0f);
	glVertex2d(-innerGaugeHeight / 2.0f, -outerGaugeHeight / 2.0f);

	/* Top Left Corner Background */
	glVertex2d(-innerGaugeHeight / 2.0f, innerGaugeHeight / 2.0f + cornerHalfDiagonal);
	glVertex2d(-innerGaugeHeight / 2.0f + cornerHalfDiagonal, innerGaugeHeight / 2.0f);
	glVertex2d(-innerGaugeHeight / 2.0f, innerGaugeHeight / 2.0f - cornerHalfDiagonal);
	glVertex2d(-innerGaugeHeight / 2.0f - cornerHalfDiagonal, innerGaugeHeight / 2.0f);

	/* Top Right Corner Background */
	glVertex2d(innerGaugeHeight / 2.0f, innerGaugeHeight / 2.0f + cornerHalfDiagonal);
	glVertex2d(innerGaugeHeight / 2.0f + cornerHalfDiagonal, innerGaugeHeight / 2.0f);
	glVertex2d(innerGaugeHeight / 2.0f, innerGaugeHeight / 2.0f - cornerHalfDiagonal);
	glVertex2d(innerGaugeHeight / 2.0f - cornerHalfDiagonal, innerGaugeHeight / 2.0f);

	/* Bottom Right Corner Background */
	glVertex2d(innerGaugeHeight / 2.0f, -innerGaugeHeight / 2.0f + cornerHalfDiagonal);
	glVertex2d(innerGaugeHeight / 2.0f + cornerHalfDiagonal, -innerGaugeHeight / 2.0f);
	glVertex2d(innerGaugeHeight / 2.0f, -innerGaugeHeight / 2.0f - cornerHalfDiagonal);
	glVertex2d(innerGaugeHeight / 2.0f - cornerHalfDiagonal, -innerGaugeHeight / 2.0f);

	/* Bottom Left Corner Background */
	glVertex2d(-innerGaugeHeight / 2.0f, -innerGaugeHeight / 2.0f + cornerHalfDiagonal);
	glVertex2d(-innerGaugeHeight / 2.0f + cornerHalfDiagonal, -innerGaugeHeight / 2.0f);
	glVertex2d(-innerGaugeHeight / 2.0f, -innerGaugeHeight / 2.0f - cornerHalfDiagonal);
	glVertex2d(-innerGaugeHeight / 2.0f - cornerHalfDiagonal, -innerGaugeHeight / 2.0f);

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::drawOuterGauge() const {
	XPLMBindTexture2d(glTextures_[ahtextureconstants::GAUGE_ID], 0);
	drawTextureRegion(&ahtextureconstants::K_OUTER_GAUGE, gaugePosLeft_, gaugePosRight_, gaugePosTop_, gaugePosBot_);
}

void AHGaugeRenderer::drawInnerGaugeVelocityRing() const {
	drawTextureRegion(&ahtextureconstants::K_INNER_GAUGE, gaugePosLeft_, gaugePosRight_, gaugePosTop_, gaugePosBot_);
}

void AHGaugeRenderer::drawVerticalVelocityNeedle(Velocity const userAircraftVertVel) const {
	// Translate the needle so it's properly rotated in place about the gauge center
	glTranslatef(needleTranslationX_, needleTranslationY_, 0.0f);

	// Rotate the needle according to the current vertical velocity - 4,000 ft/min is 150 degree rotation relative to 0 ft/min
	double vertSpeedDeg = (userAircraftVertVel.toFeetPerMin() / maxVertSpeed) * maxVSpeedDegrees - glAngleOffset_;
	vertSpeedDeg = mathutil::clampd(vertSpeedDeg, -240.0, 60.0);
	glRotated(vertSpeedDeg, 0.0, 0.0, -1.0);

	// Translate the needle back so it's in the gauge center
	glTranslatef(-needleTranslationX_, -needleTranslationY_, 0.0f);

	glBlendFunc(GL_DST_COLOR, GL_ZERO);

	// Draw Needle Mask
	XPLMBindTexture2d(glTextures_[ahtextureconstants::GAUGE_ID], 0);
	drawTextureRegion(&ahtextureconstants::K_OUTER_GAUGE, needlePosLeft_, needlePosRight_, needlePosTop_, needlePosBot_);

	glBlendFunc(GL_ONE, GL_ONE);

	// Draw Needle
	XPLMBindTexture2d(glTextures_[ahtextureconstants::GAUGE_ID], 0);
	drawTextureRegion(&ahtextureconstants::K_NEEDLE, needlePosLeft_, needlePosRight_, needlePosTop_, needlePosBot_);
}

void AHGaugeRenderer::drawRecommendedVerticalSpeedGauge(Velocity verticalVelocity) const {
	decider_->recommendationRangeLock.lock();
	RecommendationRange positive = decider_->positiveRecommendationRange;
	RecommendationRange neg = decider_->negativeRecommendationRange;
	decider_->recommendationRangeLock.unlock();

	drawVerticalSpeedGaugeBackground();

	if (positive.valid)
		drawRecommendationRange(&positive, true);

	if (neg.valid)
		drawRecommendationRange(&neg, false);

	drawVerticalSpeedGaugeGraduations();
	drawVerticalSpeedNeedle(verticalVelocity);
}

void AHGaugeRenderer::drawVerticalSpeedGaugeBackground() const {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_QUADS);
	/* Background of VSI is light grey */
	glColor4f(166.0f / 255.0f, 166.0f / 255.0f, 166.0f / 255.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(rvsLeft, rvsTopHigh);
	glVertex2d(rvsRight, rvsTopLow);
	glVertex2d(rvsRight, rvsBottomHigh);
	glVertex2d(rvsLeft, rvsBottomLow);

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::drawRecommendationRange(RecommendationRange* recommendationRange, bool recommended) const {
	float vMin = 1.0f*mathutil::clampd(recommendationRange->minVerticalSpeed.toFeetPerMin(), minVertSpeed, maxVertSpeed);
	float vMax = 1.0f*mathutil::clampd(recommendationRange->maxVerticalSpeed.toFeetPerMin(), minVertSpeed, maxVertSpeed);
	float p1;
	float p2;
	float p3;
	float p4;

	ahtextureconstants::GlRgb8Color const * recRangeColor;
	if (!hostile) {
		recRangeColor = recommended ?
			&ahtextureconstants::K_RECOMMENDATION_RANGE_POSITIVE : &ahtextureconstants::K_RECOMMENDATION_RANGE_NEGATIVE;
	} if (hostile) {
		recRangeColor = recommended ?
			&ahtextureconstants::K_RECOMMENDATION_RANGE_POSITIVE_INVERTED : &ahtextureconstants::K_RECOMMENDATION_RANGE_NEGATIVE_INVERTED;
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_QUADS);
	glColor4f(recRangeColor->red, recRangeColor->green, recRangeColor->blue, 1.0f);
	p1 = vMax*fpmToPixelsLeft;
	p2 = vMax*fpmToPixelsRight;
	p3 = vMin*fpmToPixelsRight;
	p4 = vMin*fpmToPixelsLeft;

	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(rvsLeft, p1);
	glVertex2d(rvsRight, p2);
	glVertex2d(rvsRight, p3);
	glVertex2d(rvsLeft, p4);

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::drawVerticalSpeedGaugeGraduations() const {
	float h = (innerGaugeHeight / 2.0) / tan(PI / 6.0);
	float dv = (maxVertSpeed - minVertSpeed) / rvsDivisions;
	float v;
	int count;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_LINES);
	glLineWidth(lineWidth);

	/* White -- Vertical Speed Gauge Graduations Indicating Vertical Speed */
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);

	v = maxVertSpeed - dv;
	count = 1;

	while (v > minVertSpeed) {
		float x1 = rvsLeft, x2, y1 = v*fpmToPixelsLeft, y2;
		float theta = atan(abs(y1) / h);
		float a, b;

		if ((count % 2) == 0) {
			/* Large line */
			a = largeLineLength*sin(theta);
			b = largeLineLength*cos(theta);
		}
		else {
			/* Small line */
			a = smallLineLength*sin(theta);
			b = smallLineLength*cos(theta);
		}

		x2 = x1 + b;
		y2 = (v < 0) ? (y1 + a) : (y1 - a);

		glVertex2d(x1, y1);
		glVertex2d(x2, y2);

		v -= dv;
		count++;
	}

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::drawVerticalSpeedNeedle(Velocity verticalVelocity) const {
	float velocity = 1.0f*mathutil::clampd(verticalVelocity.toFeetPerMin(), minVertSpeed, maxVertSpeed);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_LINES);
	glLineWidth(lineWidth);

	/* Light Blue -- Needle indicating the vertical speed of the aircraft */
	glColor4f(0.0f, 102.0f / 255.0f, 1.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(rvsLeft, velocity*fpmToPixelsLeft);
	glVertex2d(rvsRight, velocity*fpmToPixelsRight);

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::drawAltimeter(float currentAltitude) const {
	drawAltimeterBackground();
	drawAltimeterGraduations(currentAltitude);
	drawAltitudeIndicator(currentAltitude);
	writeAltitude(currentAltitude);
}

void AHGaugeRenderer::drawAltimeterBackground() const {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_QUADS);
	/* Background of Altimeter is light grey */
	glColor4f(166.0f / 255.0f, 166.0f / 255.0f, 166.0f / 255.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(altimeterLeft, altimeterTop);
	glVertex2d(altimeterRight, altimeterTop);
	glVertex2d(altimeterRight, altimeterBottom);
	glVertex2d(altimeterLeft, altimeterBottom);

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::drawAltimeterGraduations(float currentAltitude) const {
	float r = remainderf(currentAltitude, da);
	float y;
	int count;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_LINES);
	glLineWidth(lineWidth);

	/* White -- Altimeter graduations indicating altitudes above sea level */
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);

	y = -r*altToPixels;

	while (y >= altimeterBottom) {
		glVertex2d(altimeterLeft, y);
		glVertex2d(altimeterLeft + smallLineLength, y);

		y -= dyAltimeter;
	}

	r = da - r;
	y = r*altToPixels;

	while (y <= altimeterTop) {
		glVertex2d(altimeterLeft, y);
		glVertex2d(altimeterLeft + smallLineLength, y);

		y += dyAltimeter;
	}

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::drawAltitudeIndicator(float currentAltitude) const {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_TRIANGLES);
	/* Black -- Background of Altitude Indicator is black */
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(altimeterDisplayTriangleLeft, altimeterDisplayTriangleMid);
	glVertex2d(altimeterDisplayTriangleRight, altimeterDisplayTriangleTop);
	glVertex2d(altimeterDisplayTriangleRight, altimeterDisplayTriangleBottom);

	glEnd();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_QUADS);
	/* Black -- Background of Altitude Indicator is black */
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(altimeterDisplayLeft, altimeterDisplayTopLow);
	glVertex2d(altimeterDisplayMid, altimeterDisplayTopLow);
	glVertex2d(altimeterDisplayMid, altimeterDisplayBottomHigh);
	glVertex2d(altimeterDisplayLeft, altimeterDisplayBottomHigh);

	glVertex2d(altimeterDisplayMid, altimeterDisplayTopHigh);
	glVertex2d(altimeterDisplayRight, altimeterDisplayTopHigh);
	glVertex2d(altimeterDisplayRight, altimeterDisplayBottomLow);
	glVertex2d(altimeterDisplayMid, altimeterDisplayBottomLow);

	glEnd();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_LINES);
	glLineWidth(lineWidth);

	/* Yellow -- Border of the Altitude Indicator*/
	glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(altimeterDisplayTriangleLeft, altimeterDisplayTriangleMid);
	glVertex2d(altimeterDisplayTriangleRight, altimeterDisplayTriangleTop);
	glVertex2d(altimeterDisplayLeft, altimeterDisplayTopLow);
	glVertex2d(altimeterDisplayMid, altimeterDisplayTopLow);
	glVertex2d(altimeterDisplayMid, altimeterDisplayTopHigh);
	glVertex2d(altimeterDisplayRight, altimeterDisplayTopHigh);
	glVertex2d(altimeterDisplayRight, altimeterDisplayBottomLow);
	glVertex2d(altimeterDisplayMid, altimeterDisplayBottomLow);
	glVertex2d(altimeterDisplayMid, altimeterDisplayBottomHigh);
	glVertex2d(altimeterDisplayLeft, altimeterDisplayBottomHigh);
	glVertex2d(altimeterDisplayTriangleRight, altimeterDisplayTriangleBottom);
	glVertex2d(altimeterDisplayTriangleLeft, altimeterDisplayTriangleMid);

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::writeAltitude(float currentAltitude) const {
	writeNumber((int)std::round(currentAltitude), altimeterDisplayLeft,
		((altimeterDisplayTopLow - altimeterDisplayBottomHigh) / 2.0 + altimeterDisplayBottomHigh), true);
}


void AHGaugeRenderer::drawAirspeedGauge(float currentAirspeed) const {
	drawAirspeedGaugeBackground();
	drawAirspeedGaugeGraduations(currentAirspeed);
	drawAirspeedGaugeIndicator(currentAirspeed);
	writeAirspeed(currentAirspeed);
}

void AHGaugeRenderer::drawAirspeedGaugeBackground() const {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_QUADS);
	/* Background of Airspeed Gauge is light grey */
	glColor4f(166.0f / 255.0f, 166.0f / 255.0f, 166.0f / 255.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(airspeedGaugeLeft, airspeedGaugeTop);
	glVertex2d(airspeedGaugeRight, airspeedGaugeTop);
	glVertex2d(airspeedGaugeRight, airspeedGaugeBottom);
	glVertex2d(airspeedGaugeLeft, airspeedGaugeBottom);

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::drawAirspeedGaugeGraduations(float currentAirspeed) const {
	float r = remainderf(currentAirspeed, ds);
	float y;
	int count;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_LINES);
	glLineWidth(lineWidth);

	/* White -- Altimeter graduations indicating altitudes above sea level */
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);

	y = -r*airspeedToPixels;

	while (y >= airspeedGaugeBottom) {
		glVertex2d(airspeedGaugeRight - smallLineLength, y);
		glVertex2d(airspeedGaugeRight, y);

		y -= dyAirspeedGauge;
	}

	r = ds - r;
	y = r*airspeedToPixels;

	while (y <= airspeedGaugeTop) {
		glVertex2d(airspeedGaugeRight - smallLineLength, y);
		glVertex2d(airspeedGaugeRight, y);

		y += dyAirspeedGauge;
	}

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::drawAirspeedGaugeIndicator(float currentAirspeed) const {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_TRIANGLES);
	/* Black -- Background of Altitude Indicator is black */
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(airspeedGaugeDisplayTriangleRight, airspeedGaugeDisplayTriangleMid);
	glVertex2d(airspeedGaugeDisplayTriangleLeft, airspeedGaugeDisplayTriangleBottom);
	glVertex2d(airspeedGaugeDisplayTriangleLeft, airspeedGaugeDisplayTriangleTop);

	glEnd();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_QUADS);
	/* Black -- Background of Altitude Indicator is black */
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex2d(airspeedGaugeDisplayMid, airspeedGaugeDisplayTopLow);
	glVertex2d(airspeedGaugeDisplayRight, airspeedGaugeDisplayTopLow);
	glVertex2d(airspeedGaugeDisplayRight, airspeedGaugeDisplayBottomHigh);
	glVertex2d(airspeedGaugeDisplayMid, airspeedGaugeDisplayBottomHigh);

	glVertex2d(airspeedGaugeDisplayLeft, airspeedGaugeDisplayTopHigh);
	glVertex2d(airspeedGaugeDisplayMid, airspeedGaugeDisplayTopHigh);
	glVertex2d(airspeedGaugeDisplayMid, airspeedGaugeDisplayBottomLow);
	glVertex2d(airspeedGaugeDisplayLeft, airspeedGaugeDisplayBottomLow);

	glEnd();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	glBegin(GL_LINES);
	glLineWidth(lineWidth);

	/* Yellow -- Border of the Altitude Indicator*/
	glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
	glNormal3f(0.0f, 0.0f, 1.0f);

	glVertex2d(airspeedGaugeDisplayTriangleRight, airspeedGaugeDisplayTriangleMid);
	glVertex2d(airspeedGaugeDisplayTriangleLeft, airspeedGaugeDisplayTriangleBottom);
	glVertex2d(airspeedGaugeDisplayRight, airspeedGaugeDisplayBottomHigh);
	glVertex2d(airspeedGaugeDisplayMid, airspeedGaugeDisplayBottomHigh);
	glVertex2d(airspeedGaugeDisplayMid, airspeedGaugeDisplayBottomLow);
	glVertex2d(airspeedGaugeDisplayLeft, airspeedGaugeDisplayBottomLow);
	glVertex2d(airspeedGaugeDisplayLeft, airspeedGaugeDisplayTopHigh);
	glVertex2d(airspeedGaugeDisplayMid, airspeedGaugeDisplayTopHigh);
	glVertex2d(airspeedGaugeDisplayMid, airspeedGaugeDisplayTopLow);
	glVertex2d(airspeedGaugeDisplayRight, airspeedGaugeDisplayTopLow);
	glVertex2d(airspeedGaugeDisplayTriangleLeft, airspeedGaugeDisplayTriangleTop);
	glVertex2d(airspeedGaugeDisplayTriangleRight, airspeedGaugeDisplayTriangleMid);

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::writeAirspeed(float currentAirspeed) const {
	writeNumber((int)std::roundf(currentAirspeed), airspeedGaugeDisplayRight,
		((airspeedGaugeDisplayTopLow - airspeedGaugeDisplayBottomHigh) / 2.0 + airspeedGaugeDisplayBottomHigh), false);
}

void AHGaugeRenderer::writeNumber(int number, double borderPositionX, double centerPositionY, bool leftBorder) const {
	double charLeft, charRight, charBottom, charTop;
	ahtextureconstants::TexCoords const * signChar = number < 0.0 ? &ahtextureconstants::K_CHAR_MINUS_SIGN : &ahtextureconstants::K_CHAR_PLUS_SIGN;
	number = abs(number);
	std::string numberString = std::to_string(number);

	// Characters in the GaugeTex256 texture are 6x10 (6 px wide, 10 px tall)
	if (leftBorder) {
		charLeft = borderPositionX + 2.0;
		charRight = charLeft + 6.0;
		charBottom = centerPositionY - 5.0;
		charTop = centerPositionY + 5.0;
	}
	else {
		charLeft = borderPositionX - (numberString.length() + 1)*6.0 - 2.0;
		charRight = charLeft + 6.0;
		charBottom = centerPositionY - 5.0;
		charTop = centerPositionY + 5.0;
	}

	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 1/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);
	glBindTexture(GL_TEXTURE_2D, glTextures_[ahtextureconstants::GAUGE_ID]);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

	/* White -- Characters drawn in display boxes */
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBegin(GL_QUADS);

	glNormal3f(0.0, 0.0f, 1.0f);
	glTexCoord2d(signChar->left, signChar->top);
	glVertex2d(charLeft, charTop);
	glTexCoord2d(signChar->right, signChar->top);
	glVertex2d(charRight, charTop);
	glTexCoord2d(signChar->right, signChar->bottom);
	glVertex2d(charRight, charBottom);
	glTexCoord2d(signChar->left, signChar->bottom);
	glVertex2d(charLeft, charBottom);

	charLeft = charRight;
	charRight += 6.0;

	for (char c : numberString) {
		ahtextureconstants::TexCoords const * charCoordinates = gaugeTexCoordsFromDigitCharacter(c);
		glNormal3f(0.0, 0.0f, 1.0f);
		glTexCoord2d(charCoordinates->left, charCoordinates->top);
		glVertex2d(charLeft, charTop);
		glTexCoord2d(charCoordinates->right, charCoordinates->top);
		glVertex2d(charRight, charTop);
		glTexCoord2d(charCoordinates->right, charCoordinates->bottom);
		glVertex2d(charRight, charBottom);
		glTexCoord2d(charCoordinates->left, charCoordinates->bottom);
		glVertex2d(charLeft, charBottom);

		charLeft = charRight;
		charRight += 6.0;
	}

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void AHGaugeRenderer::drawIntrudingAircraft(LLA const * const intruderPos, Velocity const * const intruderVvel, Angle const * const userHeading, LLA const * const gaugeCenterPos, Distance const * const range, Aircraft::ThreatClassification threatClass) const {
	// These lines makes the gauge display intruders relative to the user's heading
	Angle bearing = gaugeCenterPos->bearing(intruderPos) - *userHeading;
	bearing.normalize();
	Angle cartesianAngle = Angle::bearingToCartesianAngle(&bearing);
	float separationDistance = abs(1.0f*intruderPos->range(gaugeCenterPos).toFeet());
	float intruderAltitude = 1.0f*intruderPos->altitude.toFeet();
	float userAltitude = 1.0f*gaugeCenterPos->altitude.toFeet();
	float altitudeDifference = abs(intruderAltitude - userAltitude);
	float intruderAngleY = 1.0f*asin(altitudeDifference / separationDistance);

	float maxViewAngleX = advisoryViewAngleX / 2.0f + 90.0f;
	float minViewAngleX = -advisoryViewAngleX / 2.0f + 90.0f;
	float maxViewAnlgeY = advisoryViewAngleY / 2.0f;
	float minViewAngleY = -maxViewAnlgeY;

	if ((separationDistance <= advisoryRadiusNMi.toFeet()) &&
		(minViewAngleX <= cartesianAngle.toDegrees() && cartesianAngle.toDegrees() <= maxViewAngleX) &&
		(minViewAngleY <= -intruderAngleY && intruderAngleY <= maxViewAnlgeY)) {
		double symbolCenterX = gaugeCenterX_ + cos(cartesianAngle.toRadians())*advisoryPixelsX;
		double symbolCenterY = gaugeCenterY_ + sin(cartesianAngle.toRadians())*advisoryPixelsY;

		// The symbols are contained in 16x16 pixel squares in the texture so to find the vertices,
		// calculate the sides of the 16 px square centered around the symbol center
		double symbolLeft = symbolCenterX - 8.0;
		double symbolRight = symbolCenterX + 8.0;
		double symbolBottom = symbolCenterY - 8.0;
		double symbolTop = symbolCenterY + 8.0;

		ahtextureconstants::TexCoords const * symbolCoords = aircraftSymbolFromThreatClassification(threatClass);

		XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 1/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);
		glBindTexture(GL_TEXTURE_2D, glTextures_[ahtextureconstants::GAUGE_ID]);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glPushMatrix();
		glTranslatef(gaugeCenterX_, gaugeCenterY_, 0.0f);

		/* White -- Characters drawn in display boxes */
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		glBegin(GL_QUADS);

		glNormal3f(0.0, 0.0f, 1.0f);
		glTexCoord2d(symbolCoords->left, symbolCoords->top);
		glVertex2d(symbolLeft, symbolTop);
		glTexCoord2d(symbolCoords->right, symbolCoords->top);
		glVertex2d(symbolRight, symbolTop);
		glTexCoord2d(symbolCoords->right, symbolCoords->bottom);
		glVertex2d(symbolRight, symbolBottom);
		glTexCoord2d(symbolCoords->left, symbolCoords->bottom);
		glVertex2d(symbolLeft, symbolBottom);

		Distance altitudeDifference = intruderPos->altitude - gaugeCenterPos->altitude;
		int altDiffHundredsFtPerMin = (int)std::round(altitudeDifference.toFeet() / 100.0);

		ahtextureconstants::TexCoords const * signChar = altDiffHundredsFtPerMin < 0.0 ? &ahtextureconstants::K_CHAR_MINUS_SIGN : &ahtextureconstants::K_CHAR_PLUS_SIGN;
		altDiffHundredsFtPerMin = abs(altDiffHundredsFtPerMin);
		std::string altString = std::to_string(altDiffHundredsFtPerMin);

		// Characters in the GaugeTex256 texture are 6x10 (6 px wide, 10 px tall)
		double charTop = symbolBottom;
		double charBottom = charTop - 10.0;
		double charLeft = symbolLeft;
		double charRight = charLeft + 6.0;

		ahtextureconstants::GlRgb8Color const * symbolColor = symbolColorFromThreatClassification(threatClass);
		glColor3f(symbolColor->red, symbolColor->green, symbolColor->blue);

		glBegin(GL_QUADS);

		glNormal3f(0.0, 0.0f, 1.0f);
		glTexCoord2d(signChar->left, signChar->top);
		glVertex2d(charLeft, charTop);
		glTexCoord2d(signChar->right, signChar->top);
		glVertex2d(charRight, charTop);
		glTexCoord2d(signChar->right, signChar->bottom);
		glVertex2d(charRight, charBottom);
		glTexCoord2d(signChar->left, signChar->bottom);
		glVertex2d(charLeft, charBottom);

		charLeft = charRight;
		charRight += 6.0;

		for (char c : altString) {
			ahtextureconstants::TexCoords const * charCoordinates = gaugeTexCoordsFromDigitCharacter(c);
			glNormal3f(0.0, 0.0f, 1.0f);
			glTexCoord2d(charCoordinates->left, charCoordinates->top);
			glVertex2d(charLeft, charTop);
			glTexCoord2d(charCoordinates->right, charCoordinates->top);
			glVertex2d(charRight, charTop);
			glTexCoord2d(charCoordinates->right, charCoordinates->bottom);
			glVertex2d(charRight, charBottom);
			glTexCoord2d(charCoordinates->left, charCoordinates->bottom);
			glVertex2d(charLeft, charBottom);

			charLeft = charRight;
			charRight += 6.0;
		}

		ahtextureconstants::TexCoords const * vertVelArrowCoords = intruderVvel->toFeetPerMin() < 0.0 ? &ahtextureconstants::K_VERT_ARROW_DOWN : &ahtextureconstants::K_VERT_ARROW_UP;
		// The vertical arrow is 5 px wide b|y 13 px tall; the arrow is usually? centered vertically wrt to the symbol so the arrow should be drawn some amount lower than the top of the symbol
		symbolLeft = symbolRight;
		symbolRight = symbolRight + 5.0f;
		symbolTop = symbolTop - 4.0f;
		symbolBottom = symbolTop - 13.0f;

		glNormal3f(0.0, 0.0f, 1.0f);
		glTexCoord2d(vertVelArrowCoords->left, vertVelArrowCoords->top);
		glVertex2d(symbolLeft, symbolTop);
		glTexCoord2d(vertVelArrowCoords->right, vertVelArrowCoords->top);
		glVertex2d(symbolRight, symbolTop);
		glTexCoord2d(vertVelArrowCoords->right, vertVelArrowCoords->bottom);
		glVertex2d(symbolRight, symbolBottom);
		glTexCoord2d(vertVelArrowCoords->left, vertVelArrowCoords->bottom);
		glVertex2d(symbolLeft, symbolBottom);

		glColor3f(1.0f, 1.0f, 1.0f);
		glEnd();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
		glPopMatrix();

	}

}

void AHGaugeRenderer::markHostile() {
	hostile = !hostile;
}

bool AHGaugeRenderer::returnHostileValue() {
	return hostile;
}