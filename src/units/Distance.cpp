#include "Distance.h"

const double Distance::kFtPerMeter_ = 3.28084;
const double Distance::kFtPerNmi_ = 6076.11568;
const double Distance::kFtPerMile_ = 5280;

const double Distance::kMetersPerFt_ = 0.3048;
const double Distance::kNmiPerFt_ = 0.0001645788;
const double Distance::kMilesPerFt_ = 0.0001893939;

const Distance Distance::ZERO = Distance(0.0, DistanceUnits::FEET);

Distance::Distance(double val, Distance::DistanceUnits units) : valueFt_{ feetFromUnits(val, units) } {};

double Distance::unitsFromFeet(double val, Distance::DistanceUnits units) {
	switch (units) {
	case DistanceUnits::MILES:
		return val * kMilesPerFt_;
	case DistanceUnits::NMI:
		return val * kNmiPerFt_;
	case DistanceUnits::METERS:
		return val * kMetersPerFt_;
	default:
		return val;
	}
}

double Distance::feetFromUnits(double val, Distance::DistanceUnits units) {
	switch (units) {
	case DistanceUnits::MILES:
		return val * kFtPerMile_;
	case DistanceUnits::NMI:
		return val * kFtPerNmi_;
	case DistanceUnits::METERS:
		return val * kFtPerMeter_;
	default:
		return val;
	}
}

double Distance::toFeet() const {
	return valueFt_;
}

double Distance::toMeters() const {
	return valueFt_ * kMetersPerFt_;
}

double Distance::toMiles() const {
	return valueFt_ * kMilesPerFt_;
}

double Distance::toNmi() const {
	return valueFt_ * kNmiPerFt_;
}

double Distance::toUnits(Distance::DistanceUnits units) const {
	return unitsFromFeet(valueFt_, units);
}

Distance Distance::operator + (Distance const & d) const { 
	return Distance(valueFt_ + d.valueFt_, DistanceUnits::FEET);
}

Distance Distance::operator - (Distance const & d) const {
	return Distance(valueFt_ - d.valueFt_, DistanceUnits::FEET);
}

Distance Distance::operator * (Distance const & d) const {
	return Distance(valueFt_ * d.valueFt_, DistanceUnits::FEET);
}

Distance Distance::operator / (Distance const & d) const {
	if (d.valueFt_ != 0.0) {
		return Distance(valueFt_ / d.valueFt_, DistanceUnits::FEET);
	}
	else {
		return Distance::ZERO;
	}
}

void Distance::operator = (Distance const & d) {
	valueFt_ = d.valueFt_;
}

bool Distance::operator < (Distance const & that) const {
	return valueFt_ < that.valueFt_;
}

bool Distance::operator > (Distance const & that) const {
	return valueFt_ > that.valueFt_;
}