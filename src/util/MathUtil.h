#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

// @author nstemmle
namespace math_util {
	float clampf(float val, float min, float max);
	double clampd(double val, double min, double max);
	double RoundToNearest(double val, double multiple);
};