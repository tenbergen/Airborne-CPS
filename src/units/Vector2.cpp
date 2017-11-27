#include "units\Vector2.h"

Vector2::Vector2() { x = 0; y = 0; }

Vector2::Vector2(double xin, double yin) {
	x = xin; y = yin;
}

Vector2::Vector2(Distance d, Angle a) {
	x = std::cos(a.toRadians()) * d.toFeet();
	y = std::sin(a.toRadians()) * d.toFeet();
}

Vector2::~Vector2() {}

double Vector2::dotProduct(Vector2 v2) {
	return (x * v2.x) + (y * v2.y);
}
double Vector2::normalize() {
	return std::sqrt(dotProduct(*this));
}

Vector2 Vector2::operator - (Vector2 const & v) const {
	return Vector2(x - v.x, y - v.y);
}

Vector2 Vector2::scalarMult(double const & d) const {
	return Vector2(d * x, d * y);
}

Vector2 Vector2::operator + (Vector2 const & v) const {
	return Vector2(x + v.x, y + v.y);
}

double Vector2::magnitude() const {
	return sqrt(x * x + y * y);
}

Vector2 Vector2::rightPerpendicular() {
	return Vector2(y, -1 * x);
}