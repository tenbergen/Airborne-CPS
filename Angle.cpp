#include "Angle.h"

const Angle Angle::ZERO {0.0, DEGREES };

Angle::Angle(double val, Angle::ANGLE_UNITS units) : value_degrees_(units == RADIANS ? DegreesFromRadians(val) : val) {}

double Angle::DegreesFromRadians(double degrees){
	return degrees * (180.0 / M_PI);
}

double Angle::RadiansFromDegrees(double radians) {
	return radians * (M_PI / 180.0);
}

double Angle::ToDegrees() const {
	return value_degrees_;
}

double Angle::ToRadians() const {
	return value_degrees_ * (M_PI / 180.0);
}

double Angle::operator + (Angle const & a) const {
	return value_degrees_ + a.value_degrees_;
}

double Angle::operator - (Angle const & a) const {
	return value_degrees_ - a.value_degrees_;
}