#include "MathUtil.h"

namespace mathutil{

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

	double roundToNearest(double val, double multiple) {
		if (multiple != 0.0 && val != 0.0) {
			double sign = val > 0.0 ? 1.0 : -1.0;
			val *= sign;
			val /= multiple;
			int fixedPoint = (int)ceil(val);
			val = fixedPoint * multiple;
			val *= sign;
		}
		return val;
	}
}