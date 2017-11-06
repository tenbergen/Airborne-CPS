#pragma once

#include "util/MathUtil.h"

// @author nstemmle
class Angle
{
public:
	enum class AngleUnits { DEGREES, RADIANS };

	static double DegreesFromRadians(double degrees);
	static double RadiansFromDegrees(double radians);

	static Angle const ZERO;

	static Angle const k0Degrees_;
	static Angle const k90Degrees_;
	static Angle const k180Degrees_;
	static Angle const k360Degrees_;

	static double const kMinDegrees_;
	static double const kMaxDegrees_;

	// Converts an azimuth (north-referenced bearing) to its equivalent unit circle-referenced angle (cartesian angle?)
	static Angle bearingToCartesianAngle(Angle const * const bearing);
	
	Angle(double value, AngleUnits units);
	
	double toDegrees() const;
	double toRadians() const;

	void normalize();

	Angle operator + (Angle const & a) const;
	Angle operator - (Angle const & a) const;
	void operator = (Angle const & a);

	bool operator < (Angle const & that) const;
	bool operator > (Angle const & that) const;
private:
	double value_degrees_;
};