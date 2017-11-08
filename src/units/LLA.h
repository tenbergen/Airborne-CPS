#pragma once

#include "Angle.h"
#include "Distance.h"
#include "Vec2.h"

/* Representation of a latitude, longitude, and altitude triplet with latitude between [-90,90], with +90 corresponding to north,
longitude between [-180, 180], with +180 corresponding to east, and altitude measured relative to mean sea level. */

// @author nstemmle
class LLA
{
public:
	LLA(double lat, double lon, double alt, Angle::AngleUnits angleUnits, Distance::DistanceUnits distUnits);
	LLA(Angle lat, Angle lon, Distance alt);
	LLA(Angle lat, Angle lon);
	explicit LLA();

	static LLA const ZERO;
	static Vec2 const NORTH;
	static Distance const K_RADIUS_EARTH;

	//https://en.wikipedia.org/wiki/Latitude#Length_of_a_degree_of_latitude
	static double const K_METERS_PER_DEG_LAT_CONST_1;
	static double const K_METERS_PER_DEG_LAT_CONST_2;
	static double const K_METERS_PER_DEG_LAT_CONST_3;
		
	static double const K_METERS_PER_DEG_LON_CONST_1;
	static double const K_METERS_PER_DEG_LON_CONST_2;
	static double const K_METERS_PER_DEG_LON_CONST_3;

	/* Calculates the range (distance) to the supplied LLA using the haversine formula.
	Taken from: https://en.wikipedia.org/wiki/Great-circle_distance#Computational_formulas */
	Distance range(LLA const *const other) const;

	/* Calculates the north-referenced bearing (aka azimuth).*/
	Angle bearing(LLA const *const other) const;

	/*Returns an LLA that results from a translation of the supplied distance in the supplied heading (bearing).*/
	LLA translate(Angle const *const bearing, Distance const *const distance) const;

	LLA operator + (LLA const & l) const;
	LLA operator - (LLA const & l) const;
	void operator = (LLA const & l);

	/*Returns the distance per 1 degree of latitude at this LLA. Note: the distance per degree latitude is dependent 
	on the latitude so it should be recalculated for different locations.*/
	Distance distPerDegreeLat() const;

	/*Returns the distance per 1 degree of longitude at this LLA. Note: the distance per degree longitude is dependent
	on the latitude so it should be recalculated for different locations.*/
	Distance distPerDegreeLon() const;

	Angle latitude;
	Angle longitude;
	Distance altitude;
};