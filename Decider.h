#pragma once

#include "XPLMUtilities.h"
#include "Aircraft.h"
#include <map>
#include <concurrent_unordered_map.h>
#include "Distance.h"

class Decider {
public:
	Decider(Aircraft* thisAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*>* intruding_aircraft);
	void Start();
	void Analyze(Aircraft* intruder);
	enum Sense { UPWARD, DOWNWARD };

private:
	static Distance const kProtectionVolumeRadius_;

	Aircraft* thisAircraft_;
	Aircraft* intruderFromMap;

	double taThreshold = 60.0; // seconds
	double raThreshold = 30.0; // seconds
	concurrency::concurrent_unordered_map<std::string, Aircraft*>* intruderAircraft_;

	void Analyze(Aircraft* thisAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> intruding_aircraft);
	Aircraft* QueryIntrudingAircraftMap(concurrency::concurrent_unordered_map<std::string, Aircraft*> intruding_aircraft, char* ID);
	void DetermineActionRequired(Aircraft* intruder);
	double CalculateVerticalSeparation(double thisAircraftsAltitude, double intrudersAltitude);
	double CalculateRate(double separation, double temp, time_t elapsedTime);
	double CalculateTau(double a, double b);
	double CalculateSlantRange(double horizontalSeparation, double verticalSeparation);
	double CalculateSlantRangeRate(double horizontalRate, double verticalRate, time_t elapsedTime);
	double ToMinutes(std::chrono::milliseconds time);
	double CalculateElapsedTime(double t1, double t2);
	Sense DetermineResolutionSense(double thisAircraftCurrentAltitude, double thisAircraftsVerticalVelocity,
		double intruderVerticalVelocity, double slantRangeTau);
}
