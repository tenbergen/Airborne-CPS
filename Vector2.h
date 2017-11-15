#include <cmath>
#include "units\Distance.h"
#include "units\Angle.h"

class Vector2 {
public:
	double x, y;

	Vector2(double xin, double yin) {
		x = xin; y = yin;
	}

	Vector2(Distance d, Angle a) {
		x = std::cos(a.toRadians()) * d.toFeet();
		y = std::sin(a.toRadians()) * d.toFeet();
	}

	double dotProduct(Vector2 v2) {
		return (x * v2.x) + (y * v2.y);
	}
	double normalize() {
		return std::sqrt(dotProduct(*this));
	}

	Vector2 operator - (Vector2 const & v) const {
		return Vector2(x - v.x, y - v.y);
	}

	Vector2 scalarMult(double const & d) const {
		return Vector2(d * x, d * y);
	}

	Vector2 operator + (Vector2 const & v) const {
		return Vector2(x + v.x, y + v.y);
	}

	double magnitude() const {
		return sqrt(x * x + y * y);
	}

	Vector2 rightPerpendicular() {
		return Vector2(y, -1 * x);
	}

};
