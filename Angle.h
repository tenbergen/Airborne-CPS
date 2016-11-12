#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

class Angle
{
public:
	enum ANGLE_UNITS { DEGREES, RADIANS };

	static double DegreesFromRadians(double degrees);
	static double RadiansFromDegrees(double radians);

	static Angle const ZERO;

	Angle(double value, ANGLE_UNITS units);
	
	double ToDegrees() const;
	double ToRadians() const;

	Angle operator + (Angle const & a) const;
	Angle operator - (Angle const & a) const;
	Angle operator = (Angle const & a) const;
private:
	double const value_degrees_;
};