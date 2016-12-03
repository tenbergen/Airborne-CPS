#pragma once

#include "XPLMUtilities.h"
#include "Aircraft.h"
#include <map>
#include <concurrent_unordered_map.h>
#include "Distance.h"

class Decider {
public:
	Decider(Aircraft* thisAircraft);
	void Analyze(Aircraft* intruder);
	enum Sense { UPWARD, DOWNWARD, MAINTAIN };
	enum Strength { CLIMB, MAINTAIN_CLIMB, DO_NOT_DESCEND_500, DO_NOT_DESCEND_1000, DO_NOT_DESCEND_2000, 
		DESCEND, MAINTAIN_DESCEND, DO_NOT_CLIMB_500, DO_NOT_CLIMB_1000, DO_NOT_CLIMB_2000 };

private:
	static Distance const kProtectionVolumeRadius_;
	Aircraft* thisAircraft_;
	double taThreshold = 60.0; //seconds
	double raThreshold = 30.0; //seconds
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
	Strength DetermineStrength(Sense s);
}
