#pragma once

// @author nstemmle
class Distance
{
public:
	enum class DistanceUnits {FEET, METERS, NMI, MILES};
	
	static double unitsFromFeet(double val, DistanceUnits units);
	static double feetFromUnits(double val, DistanceUnits units);

	static Distance const ZERO;

	Distance(double val, DistanceUnits units);

	double toUnits(DistanceUnits units) const;

	double toFeet() const;
	double toMeters() const;
	double toMiles() const;
	double toNmi() const;

	Distance operator + (Distance const & that) const;
	Distance operator - (Distance const & that) const;
	Distance operator * (Distance const & that) const;
	/*Division by zero will return Distance::ZERO.*/
	Distance operator / (Distance const & that) const;
	void operator = (Distance const & that);

	bool operator < (Distance const & that) const;
	bool operator > (Distance const & that) const;

private:
	static double const kFtPerMeter_;
	static double const kFtPerNmi_;
	static double const kFtPerMile_;
				  
	static double const kMetersPerFt_;
	static double const kNmiPerFt_;
	static double const kMilesPerFt_;

	double valueFt_;
};