#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

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
	
	Angle(double value, AngleUnits units);
	
	double to_degrees() const;
	double to_radians() const;

	void normalize();

	Angle operator + (Angle const & a) const;
	Angle operator - (Angle const & a) const;
	void operator = (Angle const & a);
private:
	double value_degrees_;
};