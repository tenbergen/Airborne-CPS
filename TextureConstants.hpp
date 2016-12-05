#pragma once

/* Constants that are specific to textures and their attributes.*/
namespace texture_constants {
	typedef struct TexCoords {
		double const left;
		double const right;
		double const bottom;
		double const top;
	} TexCoords;

	/* An 8-bit RGB color with color values as used by opengl (between 0.0 and 1.0 as determined by x/255 where x in [0,255]) */
	typedef struct GlRgb8Color {
		float const red;
		float const green;
		float const blue;
	} GlRgb8Color;

	unsigned char const kNumTextures = 3;

	char const * const kGaugeFileNames[kNumTextures] = { "GaugeTex256.bmp", "Needle.bmp",  "NeedleMask.bmp" };
	enum TextureId { GAUGE_ID = 0, NEEDLE_ID = 1, NEEDLE_MASK_ID = 2 };

	double const kGaugeSymbolSizePx = 16.0;
	double const kGaugeSymbolOffset = kGaugeSymbolSizePx / 2.0;

	GlRgb8Color constexpr kRecommendationRangePositive = { 0.141f, 0.647f, 0.059f };
	GlRgb8Color constexpr kRecommendationRangeNegative = { 0.602f, 0.102f, 0.09f };

	GlRgb8Color constexpr kSymbolRedSquareColor = { 217.0f / 255.0f, 42.0f / 255.0f, 59.0f / 255.0f};
	GlRgb8Color constexpr kSymbolYellowCircleColor = { 246.0f / 255.0f, 226.0f / 255.0f, 32.0f / 255.0f };
	GlRgb8Color constexpr kSymbolBlueDiamondColor = { 0.0f, 171.0f / 255.0f, 222.0f / 255.0f };

	TexCoords constexpr kNeedleMask = { 0.0, 1.0, 0.0, 1.0 };
	TexCoords constexpr kNeedle = { 0.0, 1.0, 0.0, 1.0 };

	/* Texture coordinates for the GaugeTex256.bmp file. */
	TexCoords constexpr kOuterGauge  { 0.0, 0.5, 0.0, 0.5 };
	TexCoords constexpr kInnerGauge = { 0.5, 1.0, 0.0, 0.5};

	TexCoords constexpr kSymbolBlueDiamondWhole =  { 0.0,           16.0 / 512.0,  496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kSymbolBlueDiamondCutout = { 32.0 / 512.0,  48.0 / 512.0,  496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kSymbolRedSquare =         { 64.0 / 512.0,  80.0 / 512.0,  496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kSymbolYellowCircle =      { 96.0 / 512.0,  112.0 / 512.0, 496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kDebugSymbol =             { 72.0 / 512.0,  88.0 / 512.0,  496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kVertArrowDown =           { 232.0 / 512.0, 240.0 / 512.0, 496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kVertArrowUp =             { 232.0 / 512.0, 240.0 / 512.0, 511.0 / 512.0, 496.0 / 512.0 };
	
	// The text characters are all 6 px wide x 10 px tall but are spaced 8 px apart in the texture
	TexCoords constexpr kCharMinusSign = { 128.0 / 512.0, 134.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kCharPlusSign =  { 136.0 / 512.0, 142.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kCharZero =      { 144.0 / 512.0, 150.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kCharOne =       { 152.0 / 512.0, 158.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kCharTwo =       { 160.0 / 512.0, 166.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kCharThree =     { 168.0 / 512.0, 174.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kCharFour =      { 176.0 / 512.0, 182.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kCharFive =      { 184.0 / 512.0, 190.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kCharSix =       { 192.0 / 512.0, 198.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kCharSeven =     { 200.0 / 512.0, 206.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kCharEight =     { 208.0 / 512.0, 214.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr kCharNine =      { 216.0 / 512.0, 222.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
}