#pragma once

#include "RecommendationRange.h"

class Gauge
{
public:
	const int kWindowSize = 256;

	const int kWindowPosLeft = 768;
	const int kWindowPosRight = kWindowPosLeft + kWindowSize;
	const int kWindowPosBot = 0;
	const int kWindowPosTop = kWindowPosBot + kWindowSize;

	//std::atomic<RecommendationRange> recommended_;
	//std::atomic<RecommendationRange> not_recommended_;
};