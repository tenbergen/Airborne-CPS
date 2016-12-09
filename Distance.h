#pragma once

// @author nstemmle
class Distance
{
public:
	enum class DistanceUnits {FEET, METERS, NMI, MILES};
	
	static double UnitsFromFeet(double val, DistanceUnits units);
	static double FeetFromUnits(double val, DistanceUnits units);

	static Distance const ZERO;

	Distance(double val, DistanceUnits units);

	double ToUnits(DistanceUnits units) const;

	double to_feet() const;
	double to_meters() const;
	double to_miles() const;
	double to_nmi() const;

	Distance operator + (Distance const & that) const;
	Distance operator - (Distance const & that) const;
	Distance operator * (Distance const & that) const;
	/*Division by zero will return the ZERO distance.*/
	Distance operator / (Distance const & that) const;
	void operator = (Distance const & that);

private:
	static double const kFtPerMeter_;
	static double const kFtPerNmi_;
	static double const kFtPerMile_;
				  
	static double const kMetersPerFt_;
	static double const kNmiPerFt_;
	static double const kMilesPerFt_;

	double value_ft_;
};