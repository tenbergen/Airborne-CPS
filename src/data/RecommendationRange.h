#pragma once

#include "units/Velocity.h"

// @author nstemmle
typedef struct RecommendationRange {
	// Default RecommendationRange to have invalid/not set values
	Velocity minVerticalSpeed = Velocity::ZERO;
	Velocity maxVerticalSpeed = Velocity::ZERO;
	// Valid is a flag which determines if the recommendation range should be drawn
	bool valid = false;
} RecommendationRange;

typedef struct RecommendationRangePair {
	RecommendationRange positive;
	RecommendationRange negative;
} RecommendationRangePair;