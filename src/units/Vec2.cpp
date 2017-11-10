#include "Vec2.h"

Vec2::Vec2(Vec2 const & v) : x(v.x), y(v.y) {}

Vec2::Vec2(double x, double y) : x(x), y(y) {}

Vec2 Vec2::operator + (Vec2 const & v) const {
	return add(v.x, v.y);
}

Vec2 Vec2::add(double dx, double dy) const {
	return Vec2(x + dx, y + dy);
}

Vec2 Vec2::operator - (Vec2 const & v) const {
	return sub(v.x, v.y);
}

Vec2 Vec2::sub(double dx, double dy) const {
	return Vec2(x - dx, y - dy);
}

Vec2 Vec2::operator * (Vec2 const & v) const {
	return mult(v.x, v.y);
}

Vec2 Vec2::mult(double dx, double dy) const {
	return Vec2(x * dx, y * dy);
}

double Vec2::len() const {
	return sqrt(x * x + y * y);
}

Vec2 Vec2::nor() const {
	double length = len();
	return Vec2(x / length, y / length);
}

void Vec2::operator = (Vec2 const & v) {
	x = v.x;
	y = v.y;
}