#include "Decider.h"

Distance const Decider::kProtectionVolumeRadius_ = {30.0, Distance::DistanceUnits::NMI};

Decider::Decider(Aircraft* this_Aircraft,
	concurrency::concurrent_unordered_map<std::string, Aircraft*>* intruding_aircraft) : thisAircraft_(this_Aircraft), intruderAircraft_(intruding_aircraft) {}

void Decider::Analyze(Aircraft* intruder) {
	/*char debug_buf[128];
	snprintf(debug_buf, 128, "Decider::Analyze - intruder_id: %s\n", intruder->id_.c_str());
	XPLMDebugString(debug_buf);*/
	Decider::DetermineActionRequired(intruder);
}

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

std::string get_threat_class_str(Aircraft::ThreatClassification threat_class) {
	switch (threat_class) {
	case Aircraft::ThreatClassification::NON_THREAT_TRAFFIC:
		return "Non-Threat Traffic";
	case Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC:
		return "Proximity Intruder Traffic";
	case Aircraft::ThreatClassification::TRAFFIC_ADVISORY:
		return "Traffic Advisory";
	case Aircraft::ThreatClassification::RESOLUTION_ADVISORY:
		return "Resolution Advisory";
	default:
		return "Unknown Threat Class";
	}
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
	double horizontalTau = Decider::CalculateTau(currentHorizontalSeparation, horizontalRate);

	double verticalRate = Decider::CalculateRate(currentVerticalSeparation, previousVerticalSeparation, 1, 2);
	double verticalTau = Decider::CalculateTau(currentVerticalSeparation, verticalRate);

	double slantRangeRate = Decider::CalculateSlantRangeRate(horizontalRate, verticalRate, 1, 2);
	double slantRangeTau = Decider::CalculateTau(currentSlantRange, slantRangeRate);

	Aircraft::ThreatClassification threat_class;

	if (currentSlantRange < kProtectionVolumeRadius_.to_feet()) {
		if (horizontalTau <= raThreshold && verticalTau <= raThreshold) {
			Decider::SetState(intruder, RA);
			threat_class = Aircraft::ThreatClassification::RESOLUTION_ADVISORY;
		}
		else if (horizontalTau <= taThreshold && verticalTau <= taThreshold) {
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

	char debug_buf[256];
	snprintf(debug_buf, 256, "Decider::DetermineActionRequired - intruderId: %s, currentSlantRange: %.3f, horizontalTau: %.3f, verticalTau: %.3f, threat_class: %s \n", intruder->id_.c_str(), currentSlantRange, horizontalTau, verticalTau, get_threat_class_str(threat_class).c_str());
	XPLMDebugString(debug_buf);

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
	return sqrt(horizontalSeparation * horizontalSeparation + verticalSeparation * verticalSeparation);
}

double Decider::CalculateSlantRangeRate(double horizontalRate, double verticalRate, time_t t1, time_t t2) {
	return (sqrt(horizontalRate * horizontalRate + verticalRate * verticalRate)) / (t2 - t1);
}