#include "GaugeRenderer.h"

const float GaugeRenderer::kGaugeInnerCircleRadiusPxls = 75.0f;

const float GaugeRenderer::kGaugePosLeft = 768.0f;
const float GaugeRenderer::kGaugePosRight = 1024.0f;
const float GaugeRenderer::kGaugePosBot = 0.0f;
const float GaugeRenderer::kGaugePosTop = 256.0f;

const float GaugeRenderer::kGaugeCenterX = (kGaugePosRight + kGaugePosLeft) / 2.0f;
const float GaugeRenderer::kGaugeCenterY = (kGaugePosTop + kGaugePosBot) / 2.0f;

const float GaugeRenderer::kNeedlePosLeft = kGaugePosLeft + 125.0f;
const float GaugeRenderer::kNeedlePosRight = kNeedlePosLeft + 8.0f;
const float GaugeRenderer::kNeedlePosBot = kGaugePosBot + 120.0f;
const float GaugeRenderer::kNeedlePosTop = kNeedlePosBot + 80.0f;

const float GaugeRenderer::kMinVertSpeed_ = -4000.0f;
const float GaugeRenderer::kMaxVertSpeed_ = 4000.0f;

const float GaugeRenderer::kMinDegrees = -360.0f;
const float GaugeRenderer::kMaxDegrees = 360.0f;

const float GaugeRenderer::kMaxVSpeedDegrees = 150.0f;
const float GaugeRenderer::kGlDiskAngleOffset = 90.0f;

const float GaugeRenderer::kNeedleTranslationX = kNeedlePosLeft + ((kNeedlePosRight - kNeedlePosLeft) / 2.0f);
const float GaugeRenderer::kNeedleTranslationY = kNeedlePosBot + 5.0f;

GaugeRenderer::GaugeRenderer(char* appPath) {
	GaugeRenderer::app_path_ = appPath;
	quadric_ = gluNewQuadric();

	gluQuadricNormals(quadric_, GLU_SMOOTH);
	gluQuadricDrawStyle(quadric_, GLU_FILL);
	gluQuadricTexture(quadric_, GLU_FALSE);
	gluQuadricOrientation(quadric_, GLU_INSIDE);
}

GaugeRenderer::~GaugeRenderer() {

}

void GaugeRenderer::LoadTextures()
{
	char catBuf[255];

	BuildTexPath(catBuf, kGaugeTexFname_, app_path_);
	if (strlen(catBuf) > 0 && !LoadTexture(catBuf, kGaugeTexId)) {
		XPLMDebugString("Gauge texture failed to load\n");
	}

	BuildTexPath(catBuf, kNeedleTexFname_, app_path_);
	if (strlen(catBuf) > 0 && !LoadTexture(catBuf, kNeedleTexId)) {
		XPLMDebugString("Needle texture failed to load\n");
	}

	BuildTexPath(catBuf, kNeedleMaskFname_, app_path_);
	if (strlen(catBuf) && !LoadTexture(catBuf, kNeedleTexMaskId)) {
		XPLMDebugString("Needle texture mask failed to load\n");
	}
}


#if APL && __MACH__
#include <Carbon/Carbon.h>
int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen)
{
	CFStringRef inStr = CFStringCreateWithCString(kCFAllocatorDefault, inPath, kCFStringEncodingMacRoman);
	if (inStr == NULL)
		return 0;
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLHFSPathStyle, 0);
	CFStringRef outStr = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	if (!CFStringGetCString(outStr, outPath, outPathMaxLen, kCFURLPOSIXPathStyle))
		return 0;
	CFRelease(outStr);
	CFRelease(url);
	CFRelease(inStr);
	return 1;
}
#endif

void GaugeRenderer::BuildTexPath(char* catBuf, char* tex_fname, const char* plugin_path) {
	catBuf[0] = '\0';
	strcat(catBuf, plugin_path);
	strcat(catBuf, tex_fname);

#if APL && __MACH__
	char catBuf2[255];
	if (ConvertPath(catBuf, catBuf2, sizeof(catBuf))) 
		strcpy(catBuf, catBuf2);
	else {
		XPLMDebugString("AirborneCPS - Unable to convert path\n");
		catBuf[0] = '\0';
	}
#endif
}

int GaugeRenderer::LoadTexture(char* tex_path, int tex_id) {
	int Status = FALSE;

	void *pImageData = 0;
	BmpLoader::IMAGEDATA sImageData;
	/// Get the bitmap from the file
	if (BmpLoader::LoadBmp(tex_path, &sImageData))
	{
		Status = TRUE;

		BmpLoader::SwapRedBlue(&sImageData);
		pImageData = sImageData.pData;

		/// Do the opengl stuff using XPLM functions for a friendly Xplane existence.
		XPLMGenerateTextureNumbers(&glTextures_[tex_id], 1);
		XPLMBindTexture2d(glTextures_[tex_id], 0);

		// This assumes that the bitmap will be either 4 channels (RGBA) or 3 channels (RGB)
		GLenum type = sImageData.Channels == 4 ? GL_RGBA : GL_RGB;
		gluBuild2DMipmaps(GL_TEXTURE_2D, sImageData.Channels, sImageData.Width, sImageData.Height, type, GL_UNSIGNED_BYTE, pImageData);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	if (pImageData)
		free(pImageData);

	return Status;
}

void GaugeRenderer::Render(float* rgb, float vert_speed_deg, RecommendationRange* recommended, RecommendationRange* not_recommended) {
	/// Turn on Alpha Blending and turn off Depth Testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 0/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);

	/// Color the gauge background according to the day/night coloring inside the cockpit
	glColor3f(rgb[0], rgb[1], rgb[2]);

	// Push the MV matrix onto the stack
	glPushMatrix();

	// Draw the gauge backface
	XPLMBindTexture2d(glTextures_[kGaugeTexId], 0);

	glBegin(GL_QUADS);
	glTexCoord2f(0.5f, 0.0f); glVertex2f(kGaugePosRight, kGaugePosBot);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(kGaugePosLeft, kGaugePosBot);
	glTexCoord2f(0.0f, 0.5f); glVertex2f(kGaugePosLeft, kGaugePosTop);
	glTexCoord2f(0.5f, 0.5f); glVertex2f(kGaugePosRight, kGaugePosTop);
	glEnd();

	drawRecommendedVerticalSpeedRange(0.0f, kMaxVertSpeed_, true);
	drawRecommendedVerticalSpeedRange(kMinVertSpeed_, 0.0f, false);
	/*drawRecommendationRangeStartSweep(0.0f, 30.0f, true);
	drawRecommendationRangeStartSweep(120.0f, 270.0f, false);
	drawRecommendationRangeStartSweep(270.0f, 360.0f, true);*/
	//drawRecommendationRange(gaugeCenterX, gaugeCenterY, 0.0f, 60.0f, true);

	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 1/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);

	glColor3f(1.0f, 1.0f, 1.0f);

	glPopMatrix();

	// Turn on alpha blending so drawing the inner gauge rings doesn't hide the outer gauge face
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Draw the inner vertical speed gauge rings
	glBegin(GL_QUADS);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(kGaugePosRight, kGaugePosBot);
	glTexCoord2f(0.5f, 0.0f); glVertex2f(kGaugePosLeft, kGaugePosBot);
	glTexCoord2f(0.5f, 0.5f); glVertex2f(kGaugePosLeft, kGaugePosTop);
	glTexCoord2f(1.0f, 0.5f); glVertex2f(kGaugePosRight, kGaugePosTop);
	glEnd();

	// Rotate the needle according to the current vertical velocity
	glTranslatef(kNeedleTranslationX, kNeedleTranslationY, 0.0f);

	if (vert_speed_deg > 60.0f) glRotatef(60, 0.0f, 0.0f, -1.0f);
	else if (vert_speed_deg < -240) glRotatef(-240, 0.0f, 0.0f, -1.0f);
	else glRotatef(vert_speed_deg, 0.0f, 0.0f, -1.0f);

	glTranslatef(-kNeedleTranslationX, -kNeedleTranslationY, 0.0f);

	glBlendFunc(GL_DST_COLOR, GL_ZERO);

	// Draw Needle Mask
	XPLMBindTexture2d(glTextures_[kNeedleTexMaskId], 0);
	glBegin(GL_QUADS);
	glTexCoord2f(1, 0.0f); glVertex2f(kNeedlePosRight, kNeedlePosBot);
	glTexCoord2f(0, 0.0f); glVertex2f(kNeedlePosLeft, kNeedlePosBot);
	glTexCoord2f(0, 1.0f); glVertex2f(kNeedlePosLeft, kNeedlePosTop);
	glTexCoord2f(1, 1.0f); glVertex2f(kNeedlePosRight, kNeedlePosTop);
	glEnd();

	glBlendFunc(GL_ONE, GL_ONE);

	// Draw Needle
	XPLMBindTexture2d(glTextures_[kNeedleTexId], 0);
	glBegin(GL_QUADS);
	glTexCoord2f(1, 0.0f); glVertex2f(kNeedlePosRight, kNeedlePosBot);
	glTexCoord2f(0, 0.0f); glVertex2f(kNeedlePosLeft, kNeedlePosBot);
	glTexCoord2f(0, 1.0f); glVertex2f(kNeedlePosLeft, kNeedlePosTop);
	glTexCoord2f(1, 1.0f); glVertex2f(kNeedlePosRight, kNeedlePosTop);
	glEnd();

	// Turn off Alpha Blending and turn on Depth Testing
	XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 0/*AlphaBlending*/, 1/*DepthTesting*/, 0/*DepthWriting*/);
	glPopMatrix();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPopMatrix();
	glFlush();
}

void GaugeRenderer::drawRecommendationRange(RecommendationRange rec_range) {
	drawRecommendedVerticalSpeedRange(rec_range.min_vertical_speed, rec_range.max_vertical_speed, rec_range.recommended);
}

void GaugeRenderer::drawRecommendedVerticalSpeedRange(float min_vert_speed, float max_vert_speed, bool recommended) {
	if (min_vert_speed > max_vert_speed) {
		float min = min_vert_speed;
		min_vert_speed = max_vert_speed;
		max_vert_speed = min;
	}

	min_vert_speed = clamp(min_vert_speed, kMinVertSpeed_, kMaxVertSpeed_);
	max_vert_speed = clamp(max_vert_speed, kMinVertSpeed_, kMaxVertSpeed_);

	float start_angle = (min_vert_speed / kMaxVertSpeed_) * kMaxVSpeedDegrees - kGlDiskAngleOffset;
	float stop_angle = (max_vert_speed / kMaxVertSpeed_) * kMaxVSpeedDegrees - kGlDiskAngleOffset;

	drawRecommendationRange(start_angle, stop_angle, recommended);
}

void GaugeRenderer::drawRecommendationRange(float start_angle, float stop_angle, bool recommended) {
	start_angle = clamp(start_angle, kMinDegrees, kMaxDegrees);
	stop_angle = clamp(stop_angle, kMinDegrees, kMaxDegrees);

	if (start_angle < 0.0f)
		start_angle += kMaxDegrees;

	if (stop_angle < 0.0f)
		stop_angle += kMaxDegrees;

	drawRecommendationRangeStartSweep(start_angle, stop_angle - start_angle, recommended);
}

void GaugeRenderer::drawRecommendationRangeStartSweep(float start_angle, float sweep_angle, bool recommended) {
	start_angle = clamp(start_angle, kMinDegrees, kMaxDegrees);
	sweep_angle = clamp(sweep_angle, kMinDegrees, kMaxDegrees);

	if (start_angle < 0.0f)
		start_angle += kMaxDegrees;

	if (sweep_angle < 0.0f)
		sweep_angle += kMaxDegrees;
	
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
	gluPartialDisk(quadric_, 50.0f, 105.0f, 32, 2, start_angle, sweep_angle);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);

	glPopMatrix();
}

float GaugeRenderer::clamp(float val, float min, float max) {
	if (val < min)
		return min;
	if (val > max)
		return max;
	return val;
}