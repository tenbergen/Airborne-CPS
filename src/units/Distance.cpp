#include "Distance.h"

const double Distance::kFtPerMeter_ = 3.28084;
const double Distance::kFtPerNmi_ = 6076.11568;
const double Distance::kFtPerMile_ = 5280;

const double Distance::kMetersPerFt_ = 0.3048;
const double Distance::kNmiPerFt_ = 0.0001645788;
const double Distance::kMilesPerFt_ = 0.0001893939;

const Distance Distance::ZERO = Distance(0.0, DistanceUnits::FEET);

Distance::Distance(double val, Distance::DistanceUnits units) : value_ft_{ FeetFromUnits(val, units) } {};

double Distance::UnitsFromFeet(double val, Distance::DistanceUnits units) {
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

double Distance::FeetFromUnits(double val, Distance::DistanceUnits units) {
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

double Distance::to_feet() const {
	return value_ft_;
}

double Distance::to_meters() const {
	return value_ft_ * kMetersPerFt_;
}

double Distance::to_miles() const {
	return value_ft_ * kMilesPerFt_;
}

double Distance::to_nmi() const {
	return value_ft_ * kNmiPerFt_;
}

double Distance::ToUnits(Distance::DistanceUnits units) const {
	return UnitsFromFeet(value_ft_, units);
}

Distance Distance::operator + (Distance const & d) const { 
	return Distance(value_ft_ + d.value_ft_, DistanceUnits::FEET);
}

Distance Distance::operator - (Distance const & d) const {
	return Distance(value_ft_ - d.value_ft_, DistanceUnits::FEET);
}

Distance Distance::operator * (Distance const & d) const {
	return Distance(value_ft_ * d.value_ft_, DistanceUnits::FEET);
}

Distance Distance::operator / (Distance const & d) const {
	if (d.value_ft_ != 0.0) {
		return Distance(value_ft_ / d.value_ft_, DistanceUnits::FEET);
	}
	else {
		return Distance::ZERO;
	}
}

void Distance::operator = (Distance const & d) {
	value_ft_ = d.value_ft_;
}