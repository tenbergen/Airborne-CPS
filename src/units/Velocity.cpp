#include "Velocity.h"

double const Velocity::kFtPerMinToMetersPerSec_ = 2.54 / 500.0;
double const Velocity::kFtPerMinToMph_ = 60.0 / 5280.0;
double const Velocity::kFtPerMinToKnots_ = 0.00987473;

double const Velocity::kMetersPerSecToFtPerMin_ = 500.0 / 2.54;
double const Velocity::kMphToFtPerMin_ = 5280.0 / 60.0;
double const Velocity::kKnotsToFtPerMin_ = 101.269;

Velocity const Velocity::ZERO = {0.0, VelocityUnits::FEET_PER_MIN};

Velocity::Velocity(double val, VelocityUnits units) : valFtPerMin_(feetPerMinFromUnits(val, units)) {}

double Velocity::feetPerMinFromUnits(double val, VelocityUnits fromUnits) {
	switch (fromUnits) {
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

double Velocity::unitsFromFeetPerMin(double val, VelocityUnits toUnits) {
	switch (toUnits) {
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
	return unitsFromFeetPerMin(valFtPerMin_, units);
}

double Velocity::toFeetPerMin() const {
	return valFtPerMin_;
}

double Velocity::toMph() const {
	return valFtPerMin_ * kFtPerMinToMph_;
}

double Velocity::toMetersPerS() const {
	return valFtPerMin_ * kFtPerMinToMetersPerSec_;
}

double Velocity::toKnots() const {
	return valFtPerMin_ * kFtPerMinToKnots_;
}

void Velocity::operator = (Velocity const & that) {
	valFtPerMin_ = that.valFtPerMin_;
}

Velocity Velocity::operator + (Velocity const & that) const {
	return Velocity(valFtPerMin_ + that.valFtPerMin_, Velocity::VelocityUnits::FEET_PER_MIN);
}

Velocity Velocity::operator - (Velocity const & that) const {
	return Velocity(valFtPerMin_ - that.valFtPerMin_, Velocity::VelocityUnits::FEET_PER_MIN);
}

bool Velocity::operator > (Velocity const & that) const {
	return valFtPerMin_ > that.valFtPerMin_;
}

bool Velocity::operator < (Velocity const & that) const {
	return valFtPerMin_ < that.valFtPerMin_;
}