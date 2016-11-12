#pragma once

class Distance
{
public:
	enum DistanceUnits {FEET, METERS, NMI, MILES};
	
	static double UnitsFromFeet(double val, DistanceUnits units);
	static double FeetFromUnits(double val, DistanceUnits units);

	static const Distance ZERO;

	Distance(double val, DistanceUnits units);

	double ToUnits(DistanceUnits units) const;

	double ToFeet() const;
	double ToMeters() const;
	double ToMiles() const;
	double ToNMI() const;

	Distance operator + (Distance const & d) const;
	Distance operator - (Distance const & d) const;
	Distance operator * (Distance const & d) const;
	/*Division by zero will return the ZERO distance.*/
	Distance operator / (Distance const & d) const;
	Distance operator = (Distance const & d) const;

private:
	static const double kFtPerMeter_;
	static const double kFtPerNmi_;
	static const double kFtPerMile_;

	static const double kMetersPerFt_;
	static const double kNmiPerFt_;
	static const double kMilesPerFt_;

	const double value_ft_;
};