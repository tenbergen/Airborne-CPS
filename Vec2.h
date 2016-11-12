#pragma once

#include <atomic>
#include <math.h>

class Vec2
{
public:
	Vec2(double x, double y) : x_(x), y_(y) {}
	Vec2() : x_(0.0f), y_(0.0f) {}

	double const x_;
	double const y_;

	Vec2 operator + (Vec2 const & v) const;
	Vec2 add(double dx, double dy) const;

	Vec2 operator - (Vec2 const & v) const;
	Vec2 sub(double dx, double dy) const;

	Vec2 operator * (Vec2 const & v) const;
	Vec2 mult(double dx, double dy) const;

	Vec2 operator = (Vec2 const & v) const;

	double len() const;
	Vec2 nor() const;
};