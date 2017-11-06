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
	return degrees * (k180Degrees_.toDegrees() / M_PI);
}

double Angle::RadiansFromDegrees(double radians) {
	return radians * (M_PI / k180Degrees_.toDegrees());
}

double Angle::toDegrees() const {
	return value_degrees_;
}

double Angle::toRadians() const {
	return value_degrees_ * (M_PI / k180Degrees_.toDegrees());
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

Angle Angle::bearingToCartesianAngle(Angle const * const bearing) {
	if (bearing->toDegrees() < 0.0) {
		return Angle(fabs(bearing->toDegrees()) + 90.0, Angle::AngleUnits::DEGREES);
	}
	else {
		Angle cartesian_angle = { 90.0 - bearing->toDegrees(), Angle::AngleUnits::DEGREES };
		cartesian_angle.normalize();
		return cartesian_angle;
	}
}

bool Angle::operator < (Angle const & that) const {
	return value_degrees_ < that.value_degrees_;
}

bool Angle::operator > (Angle const & that) const {
	return value_degrees_ > that.value_degrees_;
}