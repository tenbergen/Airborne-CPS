#pragma once

// @author nstemmle
class Velocity
{
public:
	enum class VelocityUnits {FEET_PER_MIN, MPH, METERS_PER_S, KNOTS};

	static double unitsFromFeetPerMin(double val, VelocityUnits toUnits);
	static double feetPerMinFromUnits(double val, VelocityUnits fromUnits);

	static Velocity const ZERO;

	Velocity(double val, VelocityUnits units);

	double toUnits(VelocityUnits units) const;

	double toFeetPerMin() const;
	double toMph() const;
	double toMetersPerS() const;
	// one knot equals 1 nmi/hr
	double toKnots() const;

	void operator = (Velocity const & that);
	Velocity operator + (Velocity const & that) const;
	Velocity operator - (Velocity const & that) const;

	bool operator > (Velocity const & that) const;
	bool operator < (Velocity const & that) const;
private:
	static double const kMphToFtPerMin_;
	static double const kMetersPerSecToFtPerMin_;
	static double const kKnotsToFtPerMin_;

	static double const kFtPerMinToMph_;
	static double const kFtPerMinToMetersPerSec_;
	static double const kFtPerMinToKnots_;

	double valFtPerMin_;
};