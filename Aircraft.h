#pragma once

#include <string>
#include <atomic>

#include "LLA.h"

class Aircraft
{
public:
	Aircraft(std::string const id);

	std::string const id_;
	std::atomic<LLA*> position_;
};