#pragma once

/* Constants that are specific to textures and their attributes.*/
namespace texture_constants {
	typedef struct TexCoords {
		double const left;
		double const right;
		double const bottom;
		double const top;
	} TexCoords;

	unsigned char const kNumTextures = 3;

	char const * const kGaugeFileNames[kNumTextures] = { "GaugeTex256.bmp", "Needle.bmp",  "NeedleMask.bmp" };
	enum TextureId { GAUGE_ID = 0, NEEDLE_ID = 1, NEEDLE_MASK_ID = 2 };

	double const kGaugeSymbolSizePx = 16.0;
	double const kGaugeSymbolOffset = kGaugeSymbolSizePx / 2.0;

	TexCoords constexpr kNeedleMask = { 0.0, 1.0, 0.0, 1.0 };
	TexCoords constexpr kNeedle = { 0.0, 1.0, 0.0, 1.0 };

	/* Texture coordinates for the GaugeTex256.bmp file. */
	TexCoords constexpr kOuterGauge  { 0.0, 0.5, 0.0, 0.5 };
	TexCoords constexpr kInnerGauge = { 0.5, 1.0, 0.0, 0.5};
	TexCoords constexpr kSymbolBlueDiamondWhole = { 0.0, 0.03125, 0.96875, 1.0 };
	TexCoords constexpr kSymbolBlueDiamondCutout = { 0.0625, 0.09375, 0.96875, 1.0 };
	TexCoords constexpr kSymbolRedSquare = { 0.125, 0.15625, 0.96875, 1.0 };
	TexCoords constexpr kSymbolYellowCircle = { 0.1875, 0.21875, 0.96875, 1.0 };
}