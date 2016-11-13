#pragma once

/* Constants that are specific to textures and their attributes.*/
namespace texture_constants {
	unsigned char const kNumTextures = 3;

	char const * const kGaugeFileNames[kNumTextures] = { "GaugeTex256.bmp", "Needle.bmp",  "NeedleMask.bmp" };
	enum TextureId {GAUGE_ID = 0, NEEDLE_ID = 1, NEEDLE_MASK_ID = 2};
}