#pragma once

#include <cmath>
#include "units\Distance.h"
#include "units\Angle.h"

class Vector2 {
public:
	double x, y;

	Vector2();

	Vector2(double xin, double yin);

	Vector2(Distance d, Angle a);

	~Vector2();

	double dotProduct(Vector2 v2);
	double normalize();

	Vector2 operator - (Vector2 const & v) const;
	

	Vector2 scalarMult(double const & d) const;

	Vector2 operator + (Vector2 const & v) const;

	double magnitude() const;

	Vector2 rightPerpendicular();

};
