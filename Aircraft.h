#pragma once

#include <string>
#include <atomic>

#include "LLA.h"
#include "Vec2.h"

class Aircraft
{
public:
	Aircraft(std::string const id);

	std::string const id_;

	// The horizontal velocity (x,z)
	std::atomic<Vec2*> horizontal_velocity_;

	std::atomic<double> vertical_velocity;
	std::atomic<LLA*> position_;
};