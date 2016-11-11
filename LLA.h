#pragma once

#include "Angle.h"
#include "Distance.h"

/* Representation of a latitude, longitude, and altitude triplet with latitude i [-90,90] and +90 corresponding to north,
longitude between [-180, 180] with +180 corresponding to east, and altitude measured relative to mean sea level. */

class LLA
{
public:
	LLA(double lat, double lon, double alt, Angle::ANGLE_UNITS angle_units, Distance::DISTANCE_UNITS dist_units);

	LLA(Angle lat, Angle lon, Distance alt);

	LLA(Angle lat, Angle lon);

	const static LLA ZERO;
	const static Distance kRadiusEarth;

	//https://en.wikipedia.org/wiki/Latitude#Length_of_a_degree_of_latitude
	static const double kDistPerDegLatConst1;
	static const double kDistPerDegLatConst2;
	static const double kDistPerDegLatConst3;
	
	static const double kDistPerDegLonConst1;
	static const double kDistPerDegLonConst2;
	static const double kDistPerDegLonConst3;

	Angle const latitude_;
	Angle const longitude_;
	Distance const altitude_;

	Distance* Range(LLA *const other) const;
	Angle* Bearing(LLA *const other) const;
	//Angle* Elevation(LLA* const other) const;

	LLA* Translate(Distance *const delta_lat, Distance *const delta_lon, Distance *const delta_alt) const;

	LLA* operator + (LLA const & a) const;
	LLA* operator - (LLA const & a) const;
};