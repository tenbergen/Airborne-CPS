#pragma once

#include <string>
#include <mutex>

#include "LLA.h"
#include "Vec2.h"

class Aircraft
{
public:
	static Angle VelocityToBearing(Vec2 const * const velocity);
	static Angle HeadingToBearing(Vec2 const * const heading);

	Aircraft(std::string const id);
	Aircraft(std::string const id, LLA position, Vec2 horizontal_vel, double vert_velocity);

	std::mutex lock_;

	std::string const id_;

	LLA position_;

	// The horizontal velocity (x,z)
	Vec2 horizontal_velocity_;

	// The rate of change of the altitude
	double vertical_velocity_;
};