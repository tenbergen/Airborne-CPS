#include "LLA.h"

// Mean earth radius is 3958.7613 mi
Distance const LLA::kRadiusEarth_ = Distance(3958.7613, Distance::MILES);
Vec2 const LLA::NORTH = Vec2(0.0, 1.0);
LLA const LLA::ZERO = LLA(Angle::ZERO, Angle::ZERO, Distance::ZERO);

double const LLA::kMetersPerDegLatConst1_ = 111132.954;
double const LLA::kMetersPerDegLatConst2_ = 559.822;
double const LLA::kMetersPerDegLatConst3_ = 1.175;

double const LLA::kMetersPerDegLonConst1_ = 111412.84;
double const LLA::kMetersPerDegLonConst2_ = -93.5;
double const LLA::kMetersPerDegLonConst3_ = 0.118;

LLA::LLA(double lat, double lon, double alt, Angle::ANGLE_UNITS angle_units, Distance::DistanceUnits dist_units) : latitude_(Angle(lat, angle_units)), longitude_(Angle(lon, angle_units)), altitude_(Distance(alt, dist_units)) {}

LLA::LLA(Angle lat, Angle lon, Distance alt) : latitude_(lat), longitude_(lon), altitude_(alt) {}

LLA::LLA(Angle lat, Angle lon) : latitude_(lat), longitude_(lon), altitude_(Distance::ZERO) {}

LLA::LLA() : latitude_(Angle::ZERO), longitude_(Angle::ZERO), altitude_(Distance::ZERO) {}

Distance LLA::Range(LLA const * const other) const {
	LLA diff = *this - *other;
	double lat_other_rad = other->latitude_.to_radians();
	
	double half_lat_delta = fabs(diff.latitude_.to_radians()) / 2.0;
	double half_lon_delta = fabs(diff.longitude_.to_radians()) / 2.0;

	double sin_half_lat_delta = sin(half_lat_delta);
	double sin_half_lon_delta = sin(half_lon_delta);

	double a = sin_half_lat_delta * sin_half_lat_delta + 
		cos(latitude_.to_radians()) * cos(other->latitude_.to_radians()) * sin_half_lon_delta * sin_half_lon_delta;
	kRadiusEarth_.to_feet();
	return Distance(kRadiusEarth_.to_feet() * 2 * asin(sqrt(a)), Distance::FEET);
}

Angle LLA::Bearing(LLA const * const other) const {
	double lat_other_rad = other->latitude_.to_radians();
	double lat_this_rad = latitude_.to_radians();
	double delta_lon_rad = other->longitude_.to_radians()- longitude_.to_radians();
	double cos_lat_other_rad = cos(lat_other_rad);

	double x = cos_lat_other_rad * sin(delta_lon_rad);
	double y = cos(lat_this_rad) * sin(lat_other_rad) - sin(lat_this_rad) * cos_lat_other_rad * cos(delta_lon_rad);
	return Angle(atan2(x, y), Angle::RADIANS);
}

LLA LLA::Translate(Distance const *const delta_lat, Distance const *const delta_lon, Distance const *const delta_alt) const {
	double delta_lat_deg = 0.0;
	double delta_lon_deg = 0.0;
	double d_alt = delta_alt ? delta_alt->to_feet() : 0.0;

	if (delta_lat) {
		double m_per_deg_lat = DistPerDegreeLat().to_meters();
		delta_lat_deg = delta_lat->to_meters() / m_per_deg_lat;
	}

	if (delta_lon) {
		double m_per_deg_lon = DistPerDegreeLon().to_meters();
		delta_lon_deg = delta_lon->to_meters() / m_per_deg_lon;
	}

	return LLA(latitude_.to_degrees() + delta_lat_deg, longitude_.to_degrees() + delta_lon_deg, altitude_.to_feet() + d_alt, Angle::DEGREES, Distance::FEET);
}

LLA LLA::Translate(Angle const *const bearing, Distance const *const distance) const {
	double dist_ratio = distance->to_feet() / kRadiusEarth_.to_feet();
	double sin_dist_ratio = sin(dist_ratio);
	double cos_dist_ratio = cos(dist_ratio);

	double cos_lat = cos(latitude_.to_radians());
	double sin_lat = sin(latitude_.to_radians());

	double bearing_rads = bearing->to_radians();

	Angle translated_lat = Angle(asin(sin_lat * cos_dist_ratio + cos_lat * sin_dist_ratio * cos(bearing_rads)), Angle::RADIANS);

	double y = sin(bearing_rads) * sin_dist_ratio * cos_lat;
	double x = cos_dist_ratio - sin_lat * sin(translated_lat.to_radians());

	Angle translated_lon = Angle(longitude_.to_radians() + atan2(y, x), Angle::RADIANS);

	return LLA(translated_lat, translated_lon, altitude_);
}

Distance LLA::DistPerDegreeLat() const {
	double lat_rad = latitude_.to_radians();
	return Distance(kMetersPerDegLatConst1_ + kMetersPerDegLatConst2_ * cos(2.0 * lat_rad) + kMetersPerDegLatConst3_ * cos(4.0 * lat_rad), Distance::METERS);
}

Distance LLA::DistPerDegreeLon() const {
	double lat_rad = latitude_.to_radians();
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