#include "Angle.h"

Angle const Angle::ZERO {0.0, AngleUnits::DEGREES };

Angle const Angle::k0Degrees_ = Angle(0.0, AngleUnits::DEGREES);
Angle const Angle::k90Degrees_ = Angle(90.0, AngleUnits::DEGREES);
Angle const Angle::k180Degrees_ = Angle(180.0, AngleUnits::DEGREES);
Angle const Angle::k360Degrees_ = Angle(360.0, AngleUnits::DEGREES);

double const Angle::kMinDegrees_ = -360.0;
double const Angle::kMaxDegrees_ = 360.0;

Angle::Angle(double val, Angle::AngleUnits units) : value_degrees_(units == AngleUnits::RADIANS ? DegreesFromRadians(val) : val) {}

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

void Angle::normalize() {
	while (value_degrees_ < 0.0) 
		value_degrees_ += kMaxDegrees_;

	while (value_degrees_ > kMaxDegrees_)
		value_degrees_ -= kMaxDegrees_;
}

Angle Angle::operator + (Angle const & a) const {
	return Angle(value_degrees_ + a.value_degrees_, AngleUnits::DEGREES);
}

Angle Angle::operator - (Angle const & a) const {
	return Angle(value_degrees_ - a.value_degrees_, AngleUnits::DEGREES);
}

void Angle::operator = (Angle const & a) {
	value_degrees_ = a.value_degrees_;
}