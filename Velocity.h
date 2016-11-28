#pragma once

class Velocity
{
public:
	enum class VelocityUnits {FEET_PER_MIN, MPH, METERS_PER_S, KNOTS};

	static double UnitsFromFeetPerMin(double val, VelocityUnits to_units);
	static double FeetPerMinFromUnits(double val, VelocityUnits from_units);

	static Velocity const ZERO;

	Velocity(double val, VelocityUnits units);

	double ToUnits(VelocityUnits units) const;

	double to_feet_per_min() const;
	double to_mph() const;
	double to_meters_per_s() const;
	double to_knots() const;

	void operator = (Velocity const & that);

private:
	static double const kMphToFtPerMin_;
	static double const kMetersPerSecToFtPerMin_;
	static double const kKnotsToFtPerMin_;

	static double const kFtPerMinToMph_;
	static double const kFtPerMinToMetersPerSec_;
	static double const kFtPerMinToKnots_;

	double val_ft_per_min_;
};