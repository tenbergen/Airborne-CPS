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

	double RoundToNearest(double val, double multiple) {
		if (multiple != 0.0 && val != 0.0) {
			double sign = val > 0.0 ? 1.0 : -1.0;
			val *= sign;
			val /= multiple;
			int fixed_point = (int)ceil(val);
			val = fixed_point * multiple;
			val *= sign;
		}
		return val;
	}
}