#include "Decider.h"

#ifndef _DECIDER_H_
#define _DECIDER_H_
#endif // !_DECIDER_H_

Distance const Decider::kProtectionVolumeRadius_ = {30.0, Distance::DistanceUnits::NMI};

Decider::Decider(Aircraft* this_Aircraft,
	concurrency::concurrent_unordered_map<std::string, Aircraft*>* intruding_aircraft) : thisAircraft_(this_Aircraft), intruderAircraft_(intruding_aircraft) {}

void Decider::Start() {
	Decider::Analyze(thisAircraft_, *intruderAircraft_);
}

void Decider::Analyze(Aircraft * thisAircraft, concurrency::concurrent_unordered_map<std::string, Aircraft*> intruding_aircraft) {
	Aircraft* intruder = Decider::QueryIntrudingAircraftMap(intruding_aircraft, "testAircraftID");
	Decider::DetermineActionRequired(intruder);
}

Aircraft* Decider::QueryIntrudingAircraftMap(concurrency::concurrent_unordered_map<std::string, Aircraft*> intr_aircraft, char* ID) {
	if (Aircraft* intruderFromMap = intr_aircraft[ID]) { return intruderFromMap; } else { return NULL; }
}

void Decider::DetermineActionRequired(Aircraft* intruder) {
	thisAircraft_->lock_.lock();
	LLA thisAircraftsCurrentPosition = thisAircraft_->position_current_;
	LLA thisAircraftsPreviousPosition = thisAircraft_->position_old_;
	thisAircraft_->lock_.unlock();

	intruder->lock_.lock();
	LLA const intrudersPreviousPosition = intruder->position_old_;
	LLA const intrudersCurrentPosition = intruder->position_current_;
	intruder->lock_.unlock();

	double thisAircraftsPreviousAltitude = thisAircraftsPreviousPosition.altitude_.to_feet();
	double intrudersPreviousAltitude = intrudersPreviousPosition.altitude_.to_feet();
	double previousHorizontalSeparation = thisAircraftsPreviousPosition.Range(&intrudersPreviousPosition).to_feet();
	double previousVerticalSeparation = Decider::CalculateVerticalSeparation(thisAircraftsPreviousAltitude, intrudersPreviousAltitude);
	double previousSlantRange = Decider::CalculateSlantRange(previousHorizontalSeparation, previousVerticalSeparation);

	double thisAircraftsCurrentAltitude = thisAircraftsCurrentPosition.altitude_.to_feet();
	double intrudersCurrentAltitude = intrudersCurrentPosition.altitude_.to_feet();
	double currentHorizontalSeparation = thisAircraftsCurrentPosition.Range(&intrudersCurrentPosition).to_feet();
	double currentVerticalSeparation = Decider::CalculateVerticalSeparation(thisAircraftsCurrentAltitude, intrudersCurrentAltitude);
	double currentSlantRange = Decider::CalculateSlantRange(currentHorizontalSeparation, currentVerticalSeparation);

	double horizontalRate = Decider::CalculateRate(currentHorizontalSeparation, previousHorizontalSeparation, 1, 2);
	double hoizontalTau = Decider::CalculateTau(currentHorizontalSeparation, horizontalRate);

	double verticalRate = Decider::CalculateRate(currentVerticalSeparation, previousVerticalSeparation, 1, 2);
	double verticalTau = Decider::CalculateTau(currentVerticalSeparation, verticalRate);

	double slantRangeRate = Decider::CalculateSlantRangeRate(horizontalRate, verticalRate, 0, 0);
	double slantRangeTau = Decider::CalculateTau(currentSlantRange, slantRangeRate);

	Aircraft::ThreatClassification threat_class;

	if (currentSlantRange < kProtectionVolumeRadius_.to_feet()) {
		if (hoizontalTau <= raThreshold && verticalTau <= raThreshold) {
			Decider::SetState(intruder, RA);
			threat_class = Aircraft::ThreatClassification::RESOLUTION_ADVISORY;
		}
		else if (hoizontalTau <= taThreshold && verticalTau <= taThreshold) {
			Decider::SetState(intruder, TA);
			threat_class = Aircraft::ThreatClassification::TRAFFIC_ADVISORY;
		}
		else {
			Decider::SetState(intruder, NORMAL);
			threat_class = Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC;
		}
	}
	else {
		threat_class = Aircraft::ThreatClassification::NON_THREAT_TRAFFIC;
	}

	intruder->lock_.lock();
	intruder->threat_classification_ = threat_class;
	intruder->lock_.unlock();
}

void Decider::SetState(Aircraft* intruder, State state) { //Argument decision: Aircraft or Aircraft ID
	Decider::stateMap[intruder->id_] = state;
}

Decider::State Decider::GetState(Aircraft* intruder) { //Argument decision: Aircraft or Aircraft ID
	return Decider::stateMap[intruder->id_];
}

double Decider::CalculateVerticalSeparation(double thisAircraftsAltitude, double intrudersAltitude) {
	return abs(thisAircraftsAltitude - intrudersAltitude);
}

double Decider::CalculateRate(double separation1, double separation2, time_t t1, time_t t2) {
	return ((separation2 - separation1) / (t2 - t1));
}

double Decider::CalculateTau(double separation, double rate) {
	return separation / rate;
}

double Decider::CalculateSlantRange(double horizontalSeparation, double verticalSeparation) {
	return sqrt(pow((horizontalSeparation, 2), pow(verticalSeparation, 2)));
}

double Decider::CalculateSlantRangeRate(double horizontalRate, double verticalRate, time_t t1, time_t t2) {
	return (sqrt((pow(horizontalRate, 2), pow(verticalRate, 2))) / (t2 - t1));
}