#pragma once

class Distance
{
public:
	enum DISTANCE_UNITS {FEET, METERS, NMI, MILES};
	
	static double UnitsFromFeet(double val, DISTANCE_UNITS units);
	static double FeetFromUnits(double val, DISTANCE_UNITS units);

	static const Distance ZERO;

	double ToFeet() const;
	double ToMeters() const;
	double ToMiles() const;
	double ToNMI() const;
	double ToUnits(DISTANCE_UNITS units) const;

	Distance(double val, DISTANCE_UNITS units);

	double operator + (Distance const & a) const;
	double operator - (Distance const & a) const;
	double operator * (Distance const & a) const;
	double operator / (Distance const & a) const;
private:
	static const double kFtPerMeter_;
	static const double kFtPerNmi_;
	static const double kFtPerMile_;

	static const double kMetersPerFt_;
	static const double kNmiPerFt_;
	static const double kMilesPerFt_;

	const double value_ft_;
};