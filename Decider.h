#pragma once

#include "ResolutionConnection.h"
#include <concurrent_unordered_map.h>

#include "XPLMUtilities.h"
#include "Aircraft.h"
#include <map>
#include "Distance.h"
#include "RecommendationRange.h"

class Decider {
public:
	Decider(Aircraft* thisAircraft);
	void Analyze(Aircraft* intruder);
	std::mutex recommendation_range_lock_;
	RecommendationRange positive_recommendation_range_;
	RecommendationRange negative_recommendation_range_;
	void testStart();

private:
	static Distance const kProtectionVolumeRadius_;
	Aircraft* thisAircraft_;
	double taThreshold = 60.0; //seconds
	double raThreshold = 30.0; //seconds
	void DetermineActionRequired(Aircraft* intruder);
	double CalculateVerticalSeparation(double thisAircraftsAltitude, double intrudersAltitude);
	double CalculateRate(double separation, double temp, double elapsedTime);
	double CalculateTau(double a, double b);
	double CalculateSlantRange(double horizontalSeparation, double verticalSeparation);
	double CalculateSlantRangeRate(double horizontalRate, double verticalRate, double elapsedTime);
	double ToMinutes(std::chrono::milliseconds time);
	double CalculateElapsedTime(double t1, double t2);
	Sense DetermineResolutionSense(double thisAircraftCurrentAltitude, double thisAircraftsVerticalVelocity,
		double intruderVerticalVelocity, double slantRangeTau);
	double DetermineStrength(double taVerticalVelocity, double inVerticalVelocity, Sense s, double slantRangeTau);
	concurrency::concurrent_unordered_map<std::string, ResolutionConnection*> active_connections;
};
