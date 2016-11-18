#include "Angle.h"

const Angle Angle::ZERO {0.0, DEGREES };

const Angle Angle::k0Degrees_ = Angle(0.0, Angle::DEGREES);
const Angle Angle::k90Degrees_ = Angle(90.0, Angle::DEGREES);
const Angle Angle::k180Degrees_ = Angle(180.0, Angle::DEGREES);
const Angle Angle::k360Degrees_ = Angle(360.0, Angle::DEGREES);

const double Angle::kMinDegrees_ = -360.0;
const double Angle::kMaxDegrees_ = 360.0;

Angle::Angle(double val, Angle::ANGLE_UNITS units) : value_degrees_(units == RADIANS ? DegreesFromRadians(val) : val) {}

double Angle::DegreesFromRadians(double degrees){
	return degrees * (k180Degrees_.to_degrees() / M_PI);
}

double Angle::RadiansFromDegrees(double radians) {
	return radians * (M_PI / k180Degrees_.to_degrees());
}

double Angle::to_degrees() const {
	return value_degrees_;
}

double Angle::to_radians() const {
	return value_degrees_ * (M_PI / k180Degrees_.to_degrees());
}

Angle Angle::operator + (Angle const & a) const {
	return Angle(value_degrees_ + a.value_degrees_, Angle::DEGREES);
}

Angle Angle::operator - (Angle const & a) const {
	return Angle(value_degrees_ - a.value_degrees_, Angle::DEGREES);
}

void Angle::operator = (Angle const & a) {
	value_degrees_ = a.value_degrees_;
}