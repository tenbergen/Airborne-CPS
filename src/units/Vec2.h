#pragma once

#include "util/MathUtil.h"

// @author nstemmle
class Vec2
{
public:
	Vec2(Vec2 const & v);
	Vec2(double x = 0.0, double y = 0.0);

	double x;
	double y;

	Vec2 operator + (Vec2 const & v) const;
	Vec2 add(double dx, double dy) const;

	Vec2 operator - (Vec2 const & v) const;
	Vec2 sub(double dx, double dy) const;

	Vec2 operator * (Vec2 const & v) const;
	Vec2 mult(double dx, double dy) const;

	void operator = (Vec2 const & v);

	double len() const;
	Vec2 nor() const;
};