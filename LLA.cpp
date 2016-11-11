#include "LLA.h"

// Mean earth radius is 3958.7613 mi
const Distance LLA::kRadiusEarth = Distance(3958.7613, Distance::MILES);
const LLA LLA::ZERO = LLA(Angle::ZERO, Angle::ZERO, Distance::ZERO);

const double LLA::kDistPerDegLatConst1 = 111132.954;
const double LLA::kDistPerDegLatConst2 = 559.822;
const double LLA::kDistPerDegLatConst3 = 1.175;

const double LLA::kDistPerDegLonConst1 = 111412.84;
const double LLA::kDistPerDegLonConst2 = -93.5;
const double LLA::kDistPerDegLonConst3 = 0.118;

LLA::LLA(double lat, double lon, double alt, Angle::ANGLE_UNITS angle_units, Distance::DISTANCE_UNITS dist_units) : latitude_(Angle(lat, angle_units)), longitude_(Angle(lon, angle_units)), altitude_(Distance(alt, dist_units)) {}

LLA::LLA(Angle lat, Angle lon, Distance alt) : latitude_(lat), longitude_(lon), altitude_(alt) {}

LLA::LLA(Angle lat, Angle lon) : latitude_(lat), longitude_(lon), altitude_(Distance::ZERO) {}

/* Calculates the range (distance) to the supplied LLA using the haversine formula.
Taken from: https://en.wikipedia.org/wiki/Great-circle_distance#Computational_formulas */
Distance* LLA::Range(LLA * const other) const {
	LLA* diff = *this - *other;
	double lat_other_rad = other->latitude_.ToRadians();
	
	double half_lat_delta = fabs(diff->latitude_.ToRadians()) / 2.0;
	double half_lon_delta = fabs(diff->longitude_.ToRadians()) / 2.0;

	double sin_half_lat_delta = sin(half_lat_delta);
	double sin_half_lon_delta = sin(half_lon_delta);

	double a = sin_half_lat_delta * sin_half_lat_delta + 
		cos(latitude_.ToRadians()) * cos(other->latitude_.ToRadians()) * sin_half_lon_delta * sin_half_lon_delta;
	kRadiusEarth.ToFeet();
	return new Distance(kRadiusEarth.ToFeet() * 2 * asin(sqrt(a)), Distance::FEET);
}

/* North-referenced bearing (azimuth)*/
Angle* LLA::Bearing(LLA *const other) const {
	double lat_other_rad = other->latitude_.ToRadians();
	double lat_this_rad = latitude_.ToRadians();
	double delta_lon_rad = other->longitude_.ToRadians()- longitude_.ToRadians();
	double cos_lat_other_rad = cos(lat_other_rad);

	double x = cos_lat_other_rad * sin(delta_lon_rad);
	double y = cos(lat_this_rad) * sin(lat_other_rad) - sin(lat_this_rad) * cos_lat_other_rad * cos(delta_lon_rad);
	return new Angle(atan2(x, y), Angle::RADIANS);
}

/* Returns an LLA that results from a translatation by the supplied distances.*/
LLA* LLA::Translate(Distance *const delta_lat, Distance *const delta_lon, Distance *const delta_alt) const {
	double lat_rad = latitude_.ToRadians();

	double delta_lat_deg, delta_lon_deg;
	double d_lat_m = delta_lat->ToMeters();
	double d_lon_m = delta_lon->ToMeters();

	if (d_lat_m != 0.0) {
		double m_per_deg_lat = kDistPerDegLatConst1 + kDistPerDegLatConst2 * cos(2.0 * lat_rad) + kDistPerDegLatConst3 * cos(4.0 * lat_rad);
		delta_lat_deg = d_lat_m / m_per_deg_lat;
	}

	if (d_lon_m != 0.0) {
		double m_per_deg_lon = kDistPerDegLonConst1 * cos(lat_rad) + kDistPerDegLonConst2 * cos(3.0 * lat_rad) + kDistPerDegLonConst3 * cos(5.0 * lat_rad);
		delta_lon_deg = d_lon_m / m_per_deg_lon;
	}
	return new LLA(latitude_.ToDegrees() + delta_lat_deg, longitude_.ToDegrees() + delta_lon_deg, altitude_ + *delta_alt, Angle::DEGREES, Distance::FEET);
}

LLA* LLA::operator + (const LLA& a) const {
	return new LLA(latitude_ + a.latitude_, longitude_ + a.longitude_, altitude_ + a.altitude_, Angle::DEGREES, Distance::FEET);
}

LLA* LLA::operator - (const LLA& a) const {
	return new LLA(latitude_ - a.latitude_, longitude_ - a.longitude_, altitude_ - a.altitude_, Angle::DEGREES, Distance::FEET);
}