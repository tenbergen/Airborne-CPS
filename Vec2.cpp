#include "Vec2.h"

Vec2 Vec2::operator + (Vec2 const & v) const {
	return add(v.x_, v.y_);
}

Vec2 Vec2::add(double dx, double dy) const {
	return Vec2(x_ + dx, y_ + dy);
}

Vec2 Vec2::operator - (Vec2 const & v) const {
	return sub(v.x_, v.y_);
}

Vec2 Vec2::sub(double dx, double dy) const {
	return Vec2(x_ - dx, y_ - dy);
}

Vec2 Vec2::operator * (Vec2 const & v) const {
	return mult(v.x_, v.y_);
}

Vec2 Vec2::mult(double dx, double dy) const {
	return Vec2(x_ * dx, y_ * dy);
}

double Vec2::len() const {
	return sqrt(x_ * x_ + y_ * y_);
}

Vec2 Vec2::nor() const {
	double length = len();
	return Vec2(x_ / length, y_ / length);
}

Vec2 Vec2::operator = (Vec2 const & v) const {
	return Vec2(v.x_, v.y_);
}