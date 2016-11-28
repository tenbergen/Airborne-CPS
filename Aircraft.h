#pragma once

#include <string>
#include <mutex>

#include "Velocity.h"
#include "LLA.h"
#include "Vec2.h"

class Aircraft
{
public:
	static Angle VelocityToBearing(Vec2 const * const velocity);
	static Angle HeadingToBearing(Vec2 const * const heading);

	Aircraft(std::string const id);
	Aircraft(std::string const id, LLA position, Angle heading, Velocity vertical_velocity);

	std::mutex lock_;

	std::string const id_;

	/// Values that are updated via datarefs from xplane
	LLA position_current_;
	LLA position_old_;

	Angle heading_;

	// The rate of change of the altitude
	Velocity vertical_velocity_;
};