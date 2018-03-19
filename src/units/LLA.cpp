#include "LLA.h"

// Mean earth radius is 3958.7613 mi
Distance const LLA::K_RADIUS_EARTH = Distance(3958.7613, Distance::DistanceUnits::MILES);
Vec2 const LLA::NORTH = Vec2(0.0, 1.0);
LLA const LLA::ZERO = LLA(Angle::ZERO, Angle::ZERO, Distance::ZERO);

double const LLA::K_METERS_PER_DEG_LAT_CONST_1 = 111132.954;
double const LLA::K_METERS_PER_DEG_LAT_CONST_2 = 559.822;
double const LLA::K_METERS_PER_DEG_LAT_CONST_3 = 1.175;

double const LLA::K_METERS_PER_DEG_LON_CONST_1 = 111412.84;
double const LLA::K_METERS_PER_DEG_LON_CONST_2 = -93.5;
double const LLA::K_METERS_PER_DEG_LON_CONST_3 = 0.118;

LLA::LLA(double lat, double lon, double alt, Angle::AngleUnits angleUnits, Distance::DistanceUnits distUnits) : latitude(Angle(lat, angleUnits)), longitude(Angle(lon, angleUnits)), altitude(Distance(alt, distUnits)) {}

LLA::LLA(Angle lat, Angle lon, Distance alt) : latitude(lat), longitude(lon), altitude(alt) {}

LLA::LLA(Angle lat, Angle lon) : latitude(lat), longitude(lon), altitude(Distance::ZERO) {}

LLA::LLA() : latitude(Angle::ZERO), longitude(Angle::ZERO), altitude(Distance::ZERO) {}

Distance LLA::range(LLA const * const other) const {
	LLA diff = *this - *other;
	double latOtherRad = other->latitude.toRadians();
	
	double halfLatDelta = fabs(diff.latitude.toRadians()) / 2.0;
	double halfLonDelta = fabs(diff.longitude.toRadians()) / 2.0;

	double sinHalfLatDelta = sin(halfLatDelta);
	double sinHalfLonDelta = sin(halfLonDelta);

	double a = sinHalfLatDelta * sinHalfLatDelta + 
		cos(latitude.toRadians()) * cos(other->latitude.toRadians()) * sinHalfLonDelta * sinHalfLonDelta;
	K_RADIUS_EARTH.toFeet();
	return Distance(K_RADIUS_EARTH.toFeet() * 2 * asin(sqrt(a)), Distance::DistanceUnits::FEET);
}

Angle LLA::bearing(LLA const * const other) const {
	double latOtherRad = other->latitude.toRadians();
	double latThisRad = latitude.toRadians();
	double deltaLonRad = other->longitude.toRadians()- longitude.toRadians();
	double cosLatOtherRad = cos(latOtherRad);

	double x = cosLatOtherRad * sin(deltaLonRad);
	double y = cos(latThisRad) * sin(latOtherRad) - sin(latThisRad) * cosLatOtherRad * cos(deltaLonRad);
	return Angle(atan2(x, y), Angle::AngleUnits::RADIANS);
}

LLA LLA::translate(Angle const *const bearing, Distance const *const distance) const {
	double distRatio = distance->toFeet() / K_RADIUS_EARTH.toFeet();
	double sinDistRatio = sin(distRatio);
	double cosDistRatio = cos(distRatio);

	double cosLat = cos(latitude.toRadians());
	double sinLat = sin(latitude.toRadians());

	double bearingRads = bearing->toRadians();

	Angle translatedLat = Angle(asin(sinLat * cosDistRatio + cosLat * sinDistRatio * cos(bearingRads)), Angle::AngleUnits::RADIANS);

	double y = sin(bearingRads) * sinDistRatio * cosLat;
	double x = cosDistRatio - sinLat * sin(translatedLat.toRadians());

	Angle translatedLon = Angle(longitude.toRadians() + atan2(y, x), Angle::AngleUnits::RADIANS);

	return LLA(translatedLat, translatedLon, altitude);
}

Distance LLA::distPerDegreeLat() const {
	double latRad = latitude.toRadians();
	return Distance(K_METERS_PER_DEG_LAT_CONST_1 + K_METERS_PER_DEG_LAT_CONST_2 * cos(2.0 * latRad) + K_METERS_PER_DEG_LAT_CONST_3 * cos(4.0 * latRad), Distance::DistanceUnits::METERS);
}

Distance LLA::distPerDegreeLon() const {
	double latRad = latitude.toRadians();
	return Distance(K_METERS_PER_DEG_LON_CONST_1 * cos(latRad) + K_METERS_PER_DEG_LON_CONST_2 * cos(3.0 * latRad) + K_METERS_PER_DEG_LON_CONST_3 * cos(5.0 * latRad), Distance::DistanceUnits::METERS);
}

LLA LLA::operator + (LLA const & l) const {
	return LLA(latitude + l.latitude, longitude + l.longitude, altitude + l.altitude);
}

LLA LLA::operator - (LLA const & l) const {
	return LLA(latitude - l.latitude, longitude - l.longitude, altitude - l.altitude);
}

void LLA::operator = (LLA const & l) {
	latitude = l.latitude;
	longitude = l.longitude;
	altitude = l.altitude;
}