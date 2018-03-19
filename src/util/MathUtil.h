#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

// @author nstemmle
namespace mathutil {
	float clampf(float val, float min, float max);
	double clampd(double val, double min, double max);
	double roundToNearest(double val, double multiple);
};