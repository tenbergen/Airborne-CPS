#pragma once

#include <string>
#include <atomic>

#include "LLA.h"
#include "Vec2.h"

class Aircraft
{
public:
	static Angle VelocityToBearing(Vec2 const * const velocity);
	static Angle HeadingToBearing(Vec2 const * const heading);

	Aircraft(std::string const id);

	std::string const id_;

	// The horizontal velocity (x,z)
	std::atomic<Vec2*> horizontal_velocity_;

	std::atomic<double> vertical_velocity;
	std::atomic<LLA*> position_;
};