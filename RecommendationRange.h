#pragma once

#include "Velocity.h"

// @author nstemmle
typedef struct RecommendationRange {
	// Default RecommendationRange to have invalid/not set values
	Velocity min_vertical_speed = Velocity::ZERO;
	Velocity max_vertical_speed = Velocity::ZERO;
	bool valid = false;
} RecommendationRange;