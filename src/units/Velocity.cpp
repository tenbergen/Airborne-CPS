#include "Velocity.h"

double const Velocity::kFtPerMinToMetersPerSec_ = 2.54 / 500.0;
double const Velocity::kFtPerMinToMph_ = 60.0 / 5280.0;
double const Velocity::kFtPerMinToKnots_ = 0.00987473;

double const Velocity::kMetersPerSecToFtPerMin_ = 500.0 / 2.54;
double const Velocity::kMphToFtPerMin_ = 5280.0 / 60.0;
double const Velocity::kKnotsToFtPerMin_ = 101.269;

Velocity const Velocity::ZERO = {0.0, VelocityUnits::FEET_PER_MIN};

Velocity::Velocity(double val, VelocityUnits units) : val_ft_per_min_(FeetPerMinFromUnits(val, units)) {}

double Velocity::FeetPerMinFromUnits(double val, VelocityUnits from_units) {
	switch (from_units) {
	case VelocityUnits::MPH:
		return val * kMphToFtPerMin_;
	case VelocityUnits::METERS_PER_S:
		return val * kMetersPerSecToFtPerMin_;
	case VelocityUnits::KNOTS:
		return val * kKnotsToFtPerMin_;
	default:
		return val;
	}
}

double Velocity::UnitsFromFeetPerMin(double val, VelocityUnits to_units) {
	switch (to_units) {
	case VelocityUnits::MPH:
		return val * kFtPerMinToMph_;
	case VelocityUnits::METERS_PER_S:
		return val * kFtPerMinToMetersPerSec_;
	case VelocityUnits::KNOTS:
		return val * kFtPerMinToKnots_;
	default:
		return val;
	}
}

double Velocity::toUnits(VelocityUnits units) const {
	return UnitsFromFeetPerMin(val_ft_per_min_, units);
}

double Velocity::toFeetPerMin() const {
	return val_ft_per_min_;
}

double Velocity::to_mph() const {
	return val_ft_per_min_ * kFtPerMinToMph_;
}

double Velocity::toMetersPerS() const {
	return val_ft_per_min_ * kFtPerMinToMetersPerSec_;
}

double Velocity::to_knots() const {
	return val_ft_per_min_ * kFtPerMinToKnots_;
}

void Velocity::operator = (Velocity const & that) {
	val_ft_per_min_ = that.val_ft_per_min_;
}

Velocity Velocity::operator + (Velocity const & that) const {
	return Velocity(val_ft_per_min_ + that.val_ft_per_min_, Velocity::VelocityUnits::FEET_PER_MIN);
}

Velocity Velocity::operator - (Velocity const & that) const {
	return Velocity(val_ft_per_min_ - that.val_ft_per_min_, Velocity::VelocityUnits::FEET_PER_MIN);
}

bool Velocity::operator > (Velocity const & that) const {
	return val_ft_per_min_ > that.val_ft_per_min_;
}

bool Velocity::operator < (Velocity const & that) const {
	return val_ft_per_min_ < that.val_ft_per_min_;
}