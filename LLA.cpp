#include "LLA.h"

// Mean earth radius is 3958.7613 mi
const Distance LLA::kRadiusEarth_ = Distance(3958.7613, Distance::MILES);
const LLA LLA::ZERO = LLA(Angle::ZERO, Angle::ZERO, Distance::ZERO);

const double LLA::kMetersPerDegLatConst1_ = 111132.954;
const double LLA::kMetersPerDegLatConst2_ = 559.822;
const double LLA::kMetersPerDegLatConst3_ = 1.175;

const double LLA::kMetersPerDegLonConst1_ = 111412.84;
const double LLA::kMetersPerDegLonConst2_ = -93.5;
const double LLA::kMetersPerDegLonConst3_ = 0.118;

LLA::LLA(double lat, double lon, double alt, Angle::ANGLE_UNITS angle_units, Distance::DistanceUnits dist_units) : latitude_(Angle(lat, angle_units)), longitude_(Angle(lon, angle_units)), altitude_(Distance(alt, dist_units)) {}

LLA::LLA(Angle lat, Angle lon, Distance alt) : latitude_(lat), longitude_(lon), altitude_(alt) {}

LLA::LLA(Angle lat, Angle lon) : latitude_(lat), longitude_(lon), altitude_(Distance::ZERO) {}

LLA::LLA() : latitude_(Angle::ZERO), longitude_(Angle::ZERO), altitude_(Distance::ZERO) {}

Distance LLA::Range(LLA * const other) const {
	LLA diff = *this - *other;
	double lat_other_rad = other->latitude_.ToRadians();
	
	double half_lat_delta = fabs(diff.latitude_.ToRadians()) / 2.0;
	double half_lon_delta = fabs(diff.longitude_.ToRadians()) / 2.0;

	double sin_half_lat_delta = sin(half_lat_delta);
	double sin_half_lon_delta = sin(half_lon_delta);

	double a = sin_half_lat_delta * sin_half_lat_delta + 
		cos(latitude_.ToRadians()) * cos(other->latitude_.ToRadians()) * sin_half_lon_delta * sin_half_lon_delta;
	kRadiusEarth_.ToFeet();
	return Distance(kRadiusEarth_.ToFeet() * 2 * asin(sqrt(a)), Distance::FEET);
}

Angle LLA::Bearing(LLA *const other) const {
	double lat_other_rad = other->latitude_.ToRadians();
	double lat_this_rad = latitude_.ToRadians();
	double delta_lon_rad = other->longitude_.ToRadians()- longitude_.ToRadians();
	double cos_lat_other_rad = cos(lat_other_rad);

	double x = cos_lat_other_rad * sin(delta_lon_rad);
	double y = cos(lat_this_rad) * sin(lat_other_rad) - sin(lat_this_rad) * cos_lat_other_rad * cos(delta_lon_rad);
	return Angle(atan2(x, y), Angle::RADIANS);
}

LLA LLA::Translate(Distance const *const delta_lat, Distance const *const delta_lon, Distance const *const delta_alt) const {
	double delta_lat_deg = 0.0;
	double delta_lon_deg = 0.0;
	double d_alt = delta_alt ? delta_alt->ToFeet() : 0.0;

	if (delta_lat) {
		double m_per_deg_lat = DistPerDegreeLat().ToMeters();
		delta_lat_deg = delta_lat->ToMeters() / m_per_deg_lat;
	}

	if (delta_lon) {
		double m_per_deg_lon = DistPerDegreeLon().ToMeters();
		delta_lon_deg = delta_lon->ToMeters() / m_per_deg_lon;
	}

	return LLA(latitude_.ToDegrees() + delta_lat_deg, longitude_.ToDegrees() + delta_lon_deg, altitude_.ToFeet() + d_alt, Angle::DEGREES, Distance::FEET);
}

Distance LLA::DistPerDegreeLat() const {
	double lat_rad = latitude_.ToRadians();
	return Distance(kMetersPerDegLatConst1_ + kMetersPerDegLatConst2_ * cos(2.0 * lat_rad) + kMetersPerDegLatConst3_ * cos(4.0 * lat_rad), Distance::METERS);
}

Distance LLA::DistPerDegreeLon() const {
	double lat_rad = latitude_.ToRadians();
	return Distance(kMetersPerDegLonConst1_ * cos(lat_rad) + kMetersPerDegLonConst2_ * cos(3.0 * lat_rad) + kMetersPerDegLonConst3_ * cos(5.0 * lat_rad), Distance::METERS);
}

LLA LLA::operator + (LLA const & l) const {
	return LLA(latitude_ + l.latitude_, longitude_ + l.longitude_, altitude_ + l.altitude_);
}

LLA LLA::operator - (LLA const & l) const {
	return LLA(latitude_ - l.latitude_, longitude_ - l.longitude_, altitude_ - l.altitude_);
}

LLA LLA::operator = (LLA const & l) const {
	return LLA(latitude_, longitude_, altitude_);
}