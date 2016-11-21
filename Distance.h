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

	double to_feet() const;
	double to_meters() const;
	double to_miles() const;
	double to_nmi() const;

	Distance operator + (Distance const & d) const;
	Distance operator - (Distance const & d) const;
	Distance operator * (Distance const & d) const;
	/*Division by zero will return the ZERO distance.*/
	Distance operator / (Distance const & d) const;
	void operator = (Distance const & d);

private:
	static const double kFtPerMeter_;
	static const double kFtPerNmi_;
	static const double kFtPerMile_;

	static const double kMetersPerFt_;
	static const double kNmiPerFt_;
	static const double kMilesPerFt_;

	double value_ft_;
};