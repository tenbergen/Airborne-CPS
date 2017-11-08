#pragma once

#include "util/MathUtil.h"

// @author nstemmle
class Angle
{
public:
	enum class AngleUnits { DEGREES, RADIANS };

	static double degreesFromRadians(double degrees);
	static double radiansFromDegrees(double radians);

	static Angle const ZERO;

	static Angle const k0Degrees;
	static Angle const k90Degrees;
	static Angle const k180Degrees;
	static Angle const k360Degrees;

	static double const kMinDegrees;
	static double const kMaxDegrees;

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
	double valueDegrees_;
};