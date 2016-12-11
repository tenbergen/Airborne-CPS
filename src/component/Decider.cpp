#include "Decider.h"

Distance const Decider::kProtectionVolumeRadius_ = { 30.0, Distance::DistanceUnits::NMI };

Decider::Decider(Aircraft* this_Aircraft, concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* map) : thisAircraft_(this_Aircraft), active_connections(map) {}

void Decider::Analyze(Aircraft* intruder) {
	Decider::DetermineActionRequired(intruder);
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
	LLA taCurrPosition = thisAircraft_->position_current_;
	LLA taPrevPosition = thisAircraft_->position_old_;
	Aircraft::ThreatClassification current_threat_class = intruder->threat_classification_;
	thisAircraft_->lock_.unlock();

	intruder->lock_.lock();
	LLA const inPrevPosition = intruder->position_old_;
	LLA const inCurrPosition = intruder->position_current_;
	intruder->lock_.unlock();

	double taPrevAltitude = taPrevPosition.altitude_.to_feet();
	double inPrevAltitude = inPrevPosition.altitude_.to_feet();
	double prevHorizontalSeparation = taPrevPosition.Range(&inPrevPosition).to_feet();
	double prevVerticalSeparation = Decider::CalculateVerticalSeparation(taPrevAltitude, inPrevAltitude);
	double prevSlantRange = Decider::CalculateSlantRange(prevHorizontalSeparation, prevVerticalSeparation);

	double taCurrAltitude = taCurrPosition.altitude_.to_feet();
	double inCurrAltitude = inCurrPosition.altitude_.to_feet();
	double currHorizontalSeparation = taCurrPosition.Range(&inCurrPosition).to_feet();
	double currVerticalSeparation = Decider::CalculateVerticalSeparation(taCurrAltitude, inCurrAltitude);
	double currSlantRange = Decider::CalculateSlantRange(currHorizontalSeparation, currVerticalSeparation);

	double inElapsedTime = Decider::CalculateElapsedTime(Decider::ToMinutes(intruder->position_old_time_),
		Decider::ToMinutes(intruder->position_current_time_));
	double taElapsedTime = Decider::CalculateElapsedTime(Decider::ToMinutes(thisAircraft_->position_old_time_),
		Decider::ToMinutes(thisAircraft_->position_current_time_));

	char debug_buf[256];
	double alt_diff = taCurrAltitude - taPrevAltitude;
	snprintf(debug_buf, 256, "Decider::DetermineActionRequired - ta_elapsed_time: %f, intr_el_time: %f, alt_diff: %f\n", taElapsedTime, inElapsedTime, alt_diff);
	XPLMDebugString(debug_buf);
	
	if (taElapsedTime != 0.0 && inElapsedTime != 0.0 &&  taCurrAltitude - taPrevAltitude != 0.0) {
		double horizontalRate = Decider::CalculateRate(currHorizontalSeparation, prevHorizontalSeparation, taElapsedTime);
		double horizontalTau = Decider::CalculateTau(currHorizontalSeparation, horizontalRate);
		double verticalRate = Decider::CalculateRate(currVerticalSeparation, prevVerticalSeparation, taElapsedTime);
		double verticalTau = Decider::CalculateTau(currVerticalSeparation, verticalRate);

		double slantRangeRate = Decider::CalculateSlantRangeRate(horizontalRate, verticalRate, taElapsedTime);
		double slantRangeTau = Decider::CalculateTau(currSlantRange, slantRangeRate); //Time to Closest Point of Approach (CPA)

		double inHorizontalVelocity = inCurrPosition.Range(&inPrevPosition).to_feet() / inElapsedTime;
		double inVerticalVelocity = intruder->vertical_velocity_.to_feet_per_min();
		double taHorizontalVelocity = taCurrPosition.Range(&taPrevPosition).to_feet() / taElapsedTime;
		double taVerticalVelocity = thisAircraft_->vertical_velocity_.to_feet_per_min();

		Aircraft::ThreatClassification threat_class;
		ResolutionConnection* connection = (*active_connections)[intruder->id_];
		Sense sense = Decider::DetermineResolutionSense(taCurrAltitude, taVerticalVelocity, inVerticalVelocity, slantRangeTau);

		if (currSlantRange < kProtectionVolumeRadius_.to_feet()) {
			bool consensus_copy;
			Sense sense_copy;

			connection->lock.lock();
			consensus_copy = connection->consensusAchieved;
			sense_copy = connection->current_sense;

			if  (consensus_copy) {
				XPLMDebugString("\n\nPARTY!\n\n");
				connection->lock.unlock();
			} else if (sense_copy == Sense::UNKNOWN) {
				connection->current_sense = sense;
				connection->lock.unlock();
				connection->sendSense(sense);
			}

			threat_class = ReevaluateProximinityIntruderThreatClassification(horizontalTau, verticalTau, current_threat_class);
		} else {
			threat_class = Aircraft::ThreatClassification::NON_THREAT_TRAFFIC;
		}

		debug_buf[0] = '\0';
		snprintf(debug_buf, 256, "Decider::DetermineActionRequired - intruderId: %s, currentSlantRange: %.3f, horizontalTau: %.3f, verticalTau: %.3f, threat_class: %s \n", intruder->id_.c_str(), currSlantRange, horizontalTau, verticalTau, get_threat_class_str(threat_class).c_str());
		XPLMDebugString(debug_buf);

		double strength = Decider::DetermineStrength(taVerticalVelocity, inVerticalVelocity, sense, slantRangeTau);

		recommendation_range_lock_.lock();

		if (sense == Sense::UPWARD) {
			positive_recommendation_range_.valid = true;
			positive_recommendation_range_.min_vertical_speed = Velocity(strength, Velocity::VelocityUnits::FEET_PER_MIN);
			negative_recommendation_range_.max_vertical_speed = Velocity(-4000.0, Velocity::VelocityUnits::FEET_PER_MIN);
		} else if (sense == Sense::DOWNWARD) {
			negative_recommendation_range_.valid = true;
			negative_recommendation_range_.min_vertical_speed = Velocity(strength, Velocity::VelocityUnits::FEET_PER_MIN);
			negative_recommendation_range_.max_vertical_speed = Velocity(-4000.0, Velocity::VelocityUnits::FEET_PER_MIN);
		}
		recommendation_range_lock_.unlock();

		intruder->lock_.lock();
		intruder->threat_classification_ = threat_class;
		intruder->lock_.unlock();
	}
}

Aircraft::ThreatClassification Decider::ReevaluateProximinityIntruderThreatClassification(double horizontal_tau, double vertical_tau, Aircraft::ThreatClassification current_threat_class) const {
	char debug_buf[256];
	snprintf(debug_buf, 256, "Decider::ReevaluateProximityIntruderThreatClassification - horizontal_tau: %f, vert_tau: %f, threat_class: %s\n", horizontal_tau, vertical_tau, get_threat_class_str(current_threat_class).c_str());
	XPLMDebugString(debug_buf);

	if (current_threat_class == Aircraft::ThreatClassification::RESOLUTION_ADVISORY || 
		current_threat_class != Aircraft::ThreatClassification::RESOLUTION_ADVISORY && horizontal_tau < raThreshold && vertical_tau < raThreshold) {
		return Aircraft::ThreatClassification::RESOLUTION_ADVISORY;
	} else if(current_threat_class == Aircraft::ThreatClassification::TRAFFIC_ADVISORY || 
		current_threat_class != Aircraft::ThreatClassification::TRAFFIC_ADVISORY && horizontal_tau < taThreshold && vertical_tau < taThreshold) {
		return Aircraft::ThreatClassification::TRAFFIC_ADVISORY;
	}
	else {
		return Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC;
	}
}

Sense Decider::DetermineResolutionSense(double taCurrAlt, double taVV, double inVV, double slantRangeTau) {
	double verticalRateDelta = 1500;
	double minVertSep = 3000;
	Sense sense;
	double taVertProj = taVV * slantRangeTau;
	double inVertProj = inVV * slantRangeTau;
	double taClimbProj = (taVV + verticalRateDelta) * slantRangeTau;
	double taDescProj = (taVV - verticalRateDelta) * slantRangeTau;

	if (abs(taClimbProj - inVertProj) >= abs(taDescProj - inVertProj)) {
		if (taCurrAlt < inVertProj && taClimbProj > inVertProj) {
			if (inVertProj - taDescProj >= minVertSep) { sense = Sense::DOWNWARD; }
		} else { sense = Sense::UPWARD; }
	} else if (abs(taClimbProj - inVertProj) < abs(taDescProj - inVertProj)) {
		if (taCurrAlt > inVertProj && taDescProj < inVertProj) {
			if (taClimbProj - inVertProj >= minVertSep) { sense = Sense::UPWARD; }
		} else { sense = Sense::DOWNWARD; }
	} else {
		sense = Sense::MAINTAIN;
	}
	return sense;
}

double Decider::DetermineStrength(double taVV, double inVV, Sense sense, double slantRangeTau) {
	double taVertProj = taVV * slantRangeTau;
	double inVertProj = inVV * slantRangeTau;

	if (sense == Sense::UPWARD) {
		if (taVertProj >= inVertProj + 3000) { return taVV; }
		else {return inVertProj + 3000 / slantRangeTau; }
	} else if (sense == Sense::DOWNWARD) {
		if (taVertProj <= inVertProj - 3000) { return taVV; }
		else { return inVertProj - 3000 / slantRangeTau; }
	} else { return taVV; }
}


double Decider::ToMinutes(std::chrono::milliseconds time) {
	long long result1 = time.count();
	return ((double)result1) / 60000.0;
}

double Decider::CalculateElapsedTime(double t1, double t2) {
	return t2 - t1;
}

double Decider::CalculateVerticalSeparation(double thisAircraftsAltitude, double intrudersAltitude) {
	return abs(thisAircraftsAltitude - intrudersAltitude);
}

double Decider::CalculateRate(double separation1, double separation2, double elapsedTime) {
	return ((separation2 - separation1) / elapsedTime);
}

double Decider::CalculateTau(double separation, double rate) {
		return separation / rate;
}

double Decider::CalculateSlantRange(double horizontalSeparation, double verticalSeparation) {
	return sqrt((pow(horizontalSeparation, 2) + pow(verticalSeparation, 2)));
}

double Decider::CalculateSlantRangeRate(double horizontalRate, double verticalRate, double elapsedTime) {
	return sqrt((pow(horizontalRate, 2) + pow(verticalRate, 2))) / elapsedTime;
}