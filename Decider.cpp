#include "Decider.h"

Distance const Decider::kProtectionVolumeRadius_ = { 30.0, Distance::DistanceUnits::NMI };

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

<<<<<<< HEAD
	double intruderElapsedTime = Decider::CalculateElapsedTime(Decider::ToMinutes(intruder->position_old_time_),
		Decider::ToMinutes(intruder->position_current_time_));
	double thisAircraftsElapsedTime = Decider::CalculateElapsedTime(Decider::ToMinutes(thisAircraft_->position_old_time_),
		Decider::ToMinutes(thisAircraft_->position_current_time_));

	double horizontalRate = Decider::CalculateRate(currentHorizontalSeparation, previousHorizontalSeparation, thisAircraftsElapsedTime);
=======
	double horizontalRate = Decider::CalculateRate(currentHorizontalSeparation, previousHorizontalSeparation, 1, 2);
>>>>>>> eb51575a43423865f6a3a7a90af18061a06eb56a
	double horizontalTau = Decider::CalculateTau(currentHorizontalSeparation, horizontalRate);

	double verticalRate = Decider::CalculateRate(currentVerticalSeparation, previousVerticalSeparation, thisAircraftsElapsedTime);
	double verticalTau = Decider::CalculateTau(currentVerticalSeparation, verticalRate);

<<<<<<< HEAD
	double slantRangeRate = Decider::CalculateSlantRangeRate(horizontalRate, verticalRate, thisAircraftsElapsedTime);
	double slantRangeTau = Decider::CalculateTau(currentSlantRange, slantRangeRate); //Time to Closest Point of Approach

	double intruderHorizontalVelocity = intrudersCurrentPosition.Range(&intrudersPreviousPosition).to_feet() / intruderElapsedTime;
	double intruderVerticalVelocity = intruder->vertical_velocity_.to_feet_per_min;
	double intruderHorizontalProjection = Decider::ProjectPosition(intruderHorizontalVelocity, slantRangeTau);
	double intruderVerticalProjection = Decider::ProjectPosition(intruderVerticalVelocity, slantRangeTau);

	double thisAircraftsHorizontalVelocity = thisAircraftsCurrentPosition.Range(&thisAircraftsPreviousPosition).to_feet() / thisAircraftsElapsedTime;
	double thisAircraftsVerticalVelocity = thisAircraft_->vertical_velocity_.to_feet_per_min;
	double thisAircraftsHorizontalProjection = Decider::ProjectPosition(thisAircraftsHorizontalVelocity, slantRangeTau);
	double thisAircraftsVerticalProjection = Decider::ProjectPosition(thisAircraftsVerticalVelocity, slantRangeTau);

	double separationV = thisAircraftsVerticalProjection - intruderVerticalProjection;
	double resolution = DetermineResolution(separationV);
=======
	double slantRangeRate = Decider::CalculateSlantRangeRate(horizontalRate, verticalRate, 1, 2);
	double slantRangeTau = Decider::CalculateTau(currentSlantRange, slantRangeRate);
>>>>>>> eb51575a43423865f6a3a7a90af18061a06eb56a

	Aircraft::ThreatClassification threat_class;

	if (currentSlantRange < kProtectionVolumeRadius_.to_feet()) {
		if (horizontalTau <= raThreshold && verticalTau <= raThreshold) {
<<<<<<< HEAD
			threat_class = Aircraft::ThreatClassification::RESOLUTION_ADVISORY;
		} else if (horizontalTau <= taThreshold && verticalTau <= taThreshold) {
=======
			Decider::SetState(intruder, RA);
			threat_class = Aircraft::ThreatClassification::RESOLUTION_ADVISORY;
		}
		else if (horizontalTau <= taThreshold && verticalTau <= taThreshold) {
			Decider::SetState(intruder, TA);
>>>>>>> eb51575a43423865f6a3a7a90af18061a06eb56a
			threat_class = Aircraft::ThreatClassification::TRAFFIC_ADVISORY;
		} else {
			threat_class = Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC;
		}
	} else {
		threat_class = Aircraft::ThreatClassification::NON_THREAT_TRAFFIC;
	}
	char debug_buf[256];
	snprintf(debug_buf, 256, "Decider::DetermineActionRequired - intruderId: %s, currentSlantRange: %.3f, horizontalTau: %.3f, verticalTau: %.3f, threat_class: %s \n", intruder->id_.c_str(), currentSlantRange, horizontalTau, verticalTau, get_threat_class_str(threat_class).c_str());
	XPLMDebugString(debug_buf);

	char debug_buf[256];
	snprintf(debug_buf, 256, "Decider::DetermineActionRequired - intruderId: %s, currentSlantRange: %.3f, horizontalTau: %.3f, verticalTau: %.3f, threat_class: %s \n", intruder->id_.c_str(), currentSlantRange, horizontalTau, verticalTau, get_threat_class_str(threat_class).c_str());
	XPLMDebugString(debug_buf);

	intruder->lock_.lock();
	intruder->threat_classification_ = threat_class;
	intruder->lock_.unlock();
}
double DetermineResolution(double separationV) {
	if (separationV <= 500) {
		;//issue a command to climb at 500ft/min
	} else if (separationV <= -500) {
		;//issue a command to descend at 500ft/min
	} else 	if (separationV <= 1000) {
		;//issue a command to climb at 1000ft/min
	} else if (separationV <= -1000) {
		;//issue a command to descend at 1000ft/min
	}
	return 0.0;
}

double Decider::ToMinutes(std::chrono::milliseconds time) {
	long long result1 = time.count();
	double result2 = result1 / 60000;
	return result2;
}

double Decider::CalculateElapsedTime(double t1, double t2) {
	return t2 - t1;
}

double Decider::ProjectPosition(double velocity, double range) {
	return velocity * range;
}

double Decider::CalculateVerticalSeparation(double thisAircraftsAltitude, double intrudersAltitude) {
	return abs(thisAircraftsAltitude - intrudersAltitude);
}

double Decider::CalculateRate(double separation1, double separation2, time_t elapsedTime) {
	return ((separation2 - separation1) / elapsedTime);
}

double Decider::CalculateTau(double separation, double rate) {
	return separation / rate;
}

double Decider::CalculateSlantRange(double horizontalSeparation, double verticalSeparation) {
<<<<<<< HEAD
	return sqrt((pow(horizontalSeparation, 2) + pow(verticalSeparation, 2)));
}

double Decider::CalculateSlantRangeRate(double horizontalRate, double verticalRate, time_t elapsedTime) {
	return sqrt((pow(horizontalRate, 2) + pow(verticalRate, 2))) / elapsedTime;
=======
	return sqrt(horizontalSeparation * horizontalSeparation + verticalSeparation * verticalSeparation);
}

double Decider::CalculateSlantRangeRate(double horizontalRate, double verticalRate, time_t t1, time_t t2) {
	return (sqrt(horizontalRate * horizontalRate + verticalRate * verticalRate)) / (t2 - t1);
>>>>>>> eb51575a43423865f6a3a7a90af18061a06eb56a
}