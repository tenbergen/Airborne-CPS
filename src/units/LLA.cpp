#include "LLA.h"

// Mean earth radius is 3958.7613 mi
Distance const LLA::kRadiusEarth_ = Distance(3958.7613, Distance::DistanceUnits::MILES);
Vec2 const LLA::NORTH = Vec2(0.0, 1.0);
LLA const LLA::ZERO = LLA(Angle::ZERO, Angle::ZERO, Distance::ZERO);

double const LLA::kMetersPerDegLatConst1_ = 111132.954;
double const LLA::kMetersPerDegLatConst2_ = 559.822;
double const LLA::kMetersPerDegLatConst3_ = 1.175;

double const LLA::kMetersPerDegLonConst1_ = 111412.84;
double const LLA::kMetersPerDegLonConst2_ = -93.5;
double const LLA::kMetersPerDegLonConst3_ = 0.118;

LLA::LLA(double lat, double lon, double alt, Angle::AngleUnits angle_units, Distance::DistanceUnits dist_units) : latitude_(Angle(lat, angle_units)), longitude_(Angle(lon, angle_units)), altitude(Distance(alt, dist_units)) {}

LLA::LLA(Angle lat, Angle lon, Distance alt) : latitude_(lat), longitude_(lon), altitude(alt) {}

LLA::LLA(Angle lat, Angle lon) : latitude_(lat), longitude_(lon), altitude(Distance::ZERO) {}

LLA::LLA() : latitude_(Angle::ZERO), longitude_(Angle::ZERO), altitude(Distance::ZERO) {}

Distance LLA::range(LLA const * const other) const {
	LLA diff = *this - *other;
	double lat_other_rad = other->latitude_.toRadians();
	
	double half_lat_delta = fabs(diff.latitude_.toRadians()) / 2.0;
	double half_lon_delta = fabs(diff.longitude_.toRadians()) / 2.0;

	double sin_half_lat_delta = sin(half_lat_delta);
	double sin_half_lon_delta = sin(half_lon_delta);

	double a = sin_half_lat_delta * sin_half_lat_delta + 
		cos(latitude_.toRadians()) * cos(other->latitude_.toRadians()) * sin_half_lon_delta * sin_half_lon_delta;
	kRadiusEarth_.toFeet();
	return Distance(kRadiusEarth_.toFeet() * 2 * asin(sqrt(a)), Distance::DistanceUnits::FEET);
}

Angle LLA::bearing(LLA const * const other) const {
	double lat_other_rad = other->latitude_.toRadians();
	double lat_this_rad = latitude_.toRadians();
	double delta_lon_rad = other->longitude_.toRadians()- longitude_.toRadians();
	double cos_lat_other_rad = cos(lat_other_rad);

	double x = cos_lat_other_rad * sin(delta_lon_rad);
	double y = cos(lat_this_rad) * sin(lat_other_rad) - sin(lat_this_rad) * cos_lat_other_rad * cos(delta_lon_rad);
	return Angle(atan2(x, y), Angle::AngleUnits::RADIANS);
}

LLA LLA::translate(Angle const *const bearing, Distance const *const distance) const {
	double dist_ratio = distance->toFeet() / kRadiusEarth_.toFeet();
	double sin_dist_ratio = sin(dist_ratio);
	double cos_dist_ratio = cos(dist_ratio);

	double cos_lat = cos(latitude_.toRadians());
	double sin_lat = sin(latitude_.toRadians());

	double bearing_rads = bearing->toRadians();

	Angle translated_lat = Angle(asin(sin_lat * cos_dist_ratio + cos_lat * sin_dist_ratio * cos(bearing_rads)), Angle::AngleUnits::RADIANS);

	double y = sin(bearing_rads) * sin_dist_ratio * cos_lat;
	double x = cos_dist_ratio - sin_lat * sin(translated_lat.toRadians());

	Angle translated_lon = Angle(longitude_.toRadians() + atan2(y, x), Angle::AngleUnits::RADIANS);

	return LLA(translated_lat, translated_lon, altitude);
}

Distance LLA::DistPerDegreeLat() const {
	double lat_rad = latitude_.toRadians();
	return Distance(kMetersPerDegLatConst1_ + kMetersPerDegLatConst2_ * cos(2.0 * lat_rad) + kMetersPerDegLatConst3_ * cos(4.0 * lat_rad), Distance::DistanceUnits::METERS);
}

Distance LLA::DistPerDegreeLon() const {
	double lat_rad = latitude_.toRadians();
	return Distance(kMetersPerDegLonConst1_ * cos(lat_rad) + kMetersPerDegLonConst2_ * cos(3.0 * lat_rad) + kMetersPerDegLonConst3_ * cos(5.0 * lat_rad), Distance::DistanceUnits::METERS);
}

LLA LLA::operator + (LLA const & l) const {
	return LLA(latitude_ + l.latitude_, longitude_ + l.longitude_, altitude + l.altitude);
}

LLA LLA::operator - (LLA const & l) const {
	return LLA(latitude_ - l.latitude_, longitude_ - l.longitude_, altitude - l.altitude);
}

void LLA::operator = (LLA const & l) {
	latitude_ = l.latitude_;
	longitude_ = l.longitude_;
	altitude = l.altitude;
}