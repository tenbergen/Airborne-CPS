#include "Angle.h"

#pragma once

namespace math_util {
	float clampf(float val, float min, float max);
	double clampd(double val, double min, double max);

	// Converts an azimuth (north-referenced bearing) to its equivalent unit circle-referenced angle (cartesian angle?)
	Angle BearingToCartesianAngle(Angle const * const bearing);
};