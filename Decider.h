#pragma once

#include "XPLMUtilities.h"
#include "RecommendationRange.h"
#include "Aircraft.h"
#include <map>
#include <concurrent_unordered_map.h>

class Decider {
public:
	Decider(Aircraft* thisAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*>* intruding_aircraft);
	void Start();
	enum State { NORMAL, TA, RA };
	State GetState(Aircraft* intruder);

	void Analyze(Aircraft* intruder);
	
	std::mutex recommendation_range_lock_;
	RecommendationRange positive_recommendation_range_;
	RecommendationRange negative_recommendation_range_;

private:
	static Distance const kProtectionVolumeRadius_;

	Aircraft* thisAircraft_;
	Aircraft* intruderFromMap;

	double taThreshold = 60.0; // seconds
	double raThreshold = 30.0; // seconds
	std::map<std::string, State> stateMap;
	concurrency::concurrent_unordered_map<std::string, Aircraft*>* intruderAircraft_;

	void Analyze(Aircraft* thisAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> intruding_aircraft);
	Aircraft* QueryIntrudingAircraftMap(concurrency::concurrent_unordered_map<std::string, Aircraft*> intruding_aircraft, char* ID);
	void DetermineActionRequired(Aircraft* intruder);
	void SetState(Aircraft* intruder, State state);
	double CalculateVerticalSeparation(double thisAircraftsAltitude, double intrudersAltitude);
	double CalculateRate(double separation, double temp, time_t t1, time_t t2);
	double CalculateTau(double a, double b);
	double CalculateSlantRange(double horizontalSeparation, double verticalSeparation);
	double CalculateSlantRangeRate(double horizontalRate, double verticalRate, time_t t1, time_t t2);
};
