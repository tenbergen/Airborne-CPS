#pragma once

#include "Angle.h"
#include "Distance.h"
#include "Vec2.h"

/* Representation of a latitude, longitude, and altitude triplet with latitude between [-90,90], with +90 corresponding to north,
longitude between [-180, 180], with +180 corresponding to east, and altitude measured relative to mean sea level. */

class LLA
{
public:
	LLA(double lat, double lon, double alt, Angle::ANGLE_UNITS angle_units, Distance::DistanceUnits dist_units);

	LLA(Angle lat, Angle lon, Distance alt);

	LLA(Angle lat, Angle lon);
	LLA();

	static LLA const ZERO;
	static Vec2 const NORTH;
	static Distance const kRadiusEarth_;

	//https://en.wikipedia.org/wiki/Latitude#Length_of_a_degree_of_latitude
	static double const kMetersPerDegLatConst1_;
	static double const kMetersPerDegLatConst2_;
	static double const kMetersPerDegLatConst3_;
		
	static double const kMetersPerDegLonConst1_;
	static double const kMetersPerDegLonConst2_;
	static double const kMetersPerDegLonConst3_;

	/* Calculates the range (distance) to the supplied LLA using the haversine formula.
	Taken from: https://en.wikipedia.org/wiki/Great-circle_distance#Computational_formulas */
	Distance Range(LLA const *const other) const;

	/* Calculates the north-referenced bearing (aka azimuth).*/
	Angle Bearing(LLA const *const other) const;

	/* Returns an LLA that results from a translatation by the supplied distances according to the distance per degree 
	latitude and longitude at the current position. Any of the parameters may be null, in which case the translation for 
	that parameter is 0.*/
	LLA Translate(Distance const *const delta_lat, Distance const *const delta_lon, Distance const *const delta_alt) const;
	/*Returns an LLA that results from a translation of the supplied distance in the supplied heading (bearing).*/
	LLA Translate(Angle const *const bearing, Distance const *const distance) const;

	LLA operator + (LLA const & l) const;
	LLA operator - (LLA const & l) const;
	LLA operator = (LLA const & l) const;

	/*Returns the distance per 1 degree of latitude at this LLA. Note: the distance per degree latitude is dependent 
	on the latitude so it should be recalculated for different locations.*/
	Distance DistPerDegreeLat() const;

	/*Returns the distance per 1 degree of longitude at this LLA. Note: the distance per degree longitude is dependent
	on the latitude so it should be recalculated for different locations.*/
	Distance DistPerDegreeLon() const;

	Angle const latitude_;
	Angle const longitude_;
	Distance const altitude_;
};