#include "MathUtil.h"

namespace math_util{

	float clampf(float val, float min, float max) {
		if (val < min)
			return min;
		if (val > max)
			return max;
		return val;
	}

	double clampd(double val, double min, double max) {
		if (val < min)
			return min;
		if (val > max)
			return max;
		return val;
	}

	Angle BearingToCartesianAngle(Angle const * const bearing) {
		if (bearing->to_degrees() < 0.0) {
			return Angle(fabs(bearing->to_degrees()) + 90.0, Angle::AngleUnits::DEGREES);
		}
		else {
			Angle cartesian_angle = {90.0 - bearing->to_degrees(), Angle::AngleUnits::DEGREES};
			cartesian_angle.normalize();
			return cartesian_angle;
		}
	}
}