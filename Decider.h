#pragma once

#include "Distance.h"
#include "Aircraft.h"
#include <map>
#include <concurrent_unordered_map.h>

class Decider {
public:
	Decider(Aircraft* thisAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*>* intruding_aircraft);
	void Decider::Start();
	enum State { NORMAL, TA, RA };
	Decider::State Decider::GetState(Aircraft* intruder) {}

private:
	Aircraft* thisAircraft_;
	Aircraft* intruderFromMap;
	State currentState;
	double Decider::taThreshold = 60.0; // seconds
	double Decider::raThreshold = 30.0; // seconds
	std::map<std::string, Decider::State> stateMap;
	concurrency::concurrent_unordered_map<std::string, Aircraft*>* intruderAircraft_;

	void Decider::Analyze(Aircraft* thisAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> intruding_aircraft);
	Aircraft* Decider::QueryIntrudingAircraftMap(concurrency::concurrent_unordered_map<std::string, Aircraft*> intruding_aircraft, char* ID);
	void Decider::DetermineActionRequired(Aircraft* intruder) {}
	void Decider::SetState(Aircraft* intruder, State state) {}
	double Decider::CalculateVerticalSeparation(double thisAircraftsAltitude, double intrudersAltitude);
	double Decider::CalculateRate(double separation, double temp, time_t t1, time_t t2) {}
	double Decider::CalculateTau(double a, double b) {}
	double Decider::CalculateSlantRange(double horizontalSeparation, double verticalSeparation) {}
	double Decider::CalculateSlantRangeRate(double horizontalRate, double verticalRate, time_t t1, time_t t2) {}
};
