#pragma once

#include <concurrent_unordered_map.h>

#include "component/ResolutionConnection.h"
#include "data/RecommendationRange.h"
#include "data/Aircraft.h"
#include "units/Distance.h"

class Decider {
public:
	Decider(Aircraft* thisAircraft);
	void Analyze(Aircraft* intruder);
	enum Strength {
		CLIMB, MAINTAIN_CLIMB, DO_NOT_DESCEND_500, DO_NOT_DESCEND_1000, DO_NOT_DESCEND_2000,
		DESCEND, MAINTAIN_DESCEND, DO_NOT_CLIMB_500, DO_NOT_CLIMB_1000, DO_NOT_CLIMB_2000
	};

	std::mutex recommendation_range_lock_;
	RecommendationRange positive_recommendation_range_;
	RecommendationRange negative_recommendation_range_;

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
	Strength DetermineStrength(Sense s, double slantRangeTau);
	concurrency::concurrent_unordered_map<std::string, ResolutionConnection*> active_connections;
};
