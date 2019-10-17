#pragma once

// @author nstemmle
/* Constants that are specific to textures and their attributes.*/
namespace vsitextureconstants {
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

	unsigned char const K_NUM_TEXTURES = 3;

	char const * const K_GAUGE_FILENAMES[K_NUM_TEXTURES] = { "GaugeTex256.bmp", "Needle.bmp",  "NeedleMask.bmp" };
	enum TextureId { GAUGE_ID = 0, NEEDLE_ID = 1, NEEDLE_MASK_ID = 2 };

	double const K_GAUGE_SYMBOL_SIZE_PX = 16.0;
	double const K_GAUGE_SYMBOL_OFFSET = K_GAUGE_SYMBOL_SIZE_PX / 2.0;

	GlRgb8Color constexpr K_RECOMMENDATION_RANGE_POSITIVE = { 0.141f, 0.647f, 0.059f };
	GlRgb8Color constexpr K_RECOMMENDATION_RANGE_NEGATIVE = { 0.602f, 0.102f, 0.09f };

	GlRgb8Color constexpr K_RECOMMENDATION_RANGE_NEGATIVE_INVERTED = { 0.141f, 0.647f, 0.059f };
	GlRgb8Color constexpr K_RECOMMENDATION_RANGE_POSITIVE_INVERTED = { 0.602f, 0.102f, 0.09f };

	GlRgb8Color constexpr K_SYMBOL_RED_SQUARE_COLOR = { 217.0f / 255.0f, 42.0f / 255.0f, 59.0f / 255.0f};
	GlRgb8Color constexpr K_SYMBOL_YELLOW_CIRCLE_COLOR = { 246.0f / 255.0f, 226.0f / 255.0f, 32.0f / 255.0f };
	GlRgb8Color constexpr K_SYMBOL_BLUE_DIAMOND_COLOR = { 0.0f, 171.0f / 255.0f, 222.0f / 255.0f };

	TexCoords constexpr K_NEEDLE_MASK = { 0.0, 1.0, 0.0, 1.0 };
	TexCoords constexpr K_NEEDLE = { 0.0, 1.0, 0.0, 1.0 };

	/* Texture coordinates for the GaugeTex256.bmp file. */
	TexCoords constexpr K_OUTER_GAUGE  { 0.0, 0.5, 0.0, 0.5 };
	TexCoords constexpr K_INNER_GAUGE = { 0.5, 1.0, 0.0, 0.5};

	TexCoords constexpr K_SYMBOL_BLUE_DIAMOND_WHOLE = { 0.0, 16.0 / 512.0,  496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_SYMBOL_BLUE_DIAMOND_CUTOUT = { 32.0 / 512.0,  48.0 / 512.0,  496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_SYMBOL_RED_SQUARE = { 64.0 / 512.0,  80.0 / 512.0,  496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_SYMBOL_YELLOW_CIRCLE = { 96.0 / 512.0,  112.0 / 512.0, 496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_DEBUG_SYMBOL = { 72.0 / 512.0,  88.0 / 512.0,  496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_VERT_ARROW_DOWN = { 232.0 / 512.0, 240.0 / 512.0, 496.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_VERT_ARROW_UP = { 232.0 / 512.0, 240.0 / 512.0, 511.0 / 512.0, 496.0 / 512.0 };
	
	// The text characters are all 6 px wide x 10 px tall but are spaced 8 px apart in the texture
	TexCoords constexpr K_CHAR_MINUS_SIGN = { 128.0 / 512.0, 134.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_CHAR_PLUS_SIGN = { 136.0 / 512.0, 142.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_CHAR_ZERO = { 144.0 / 512.0, 150.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_CHAR_ONE = { 152.0 / 512.0, 158.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_CHAR_TWO = { 160.0 / 512.0, 166.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_CHAR_THREE = { 168.0 / 512.0, 174.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_CHAR_FOUR = { 176.0 / 512.0, 182.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_CHAR_FIVE = { 184.0 / 512.0, 190.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_CHAR_SIX = { 192.0 / 512.0, 198.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_CHAR_SEVEN = { 200.0 / 512.0, 206.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_CHAR_EIGHT = { 208.0 / 512.0, 214.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
	TexCoords constexpr K_CHAR_NINE = { 216.0 / 512.0, 222.0 / 512.0, 502.0 / 512.0, 511.0 / 512.0 };
}