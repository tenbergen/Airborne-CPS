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
	Aircraft user_copy = *(thisAircraft_);
	thisAircraft_->lock_.unlock();

	intruder->lock_.lock();
	Aircraft intr_copy = *(intruder);
	intruder->lock_.unlock();

	Distance prevHorizontalSeparation = user_copy.position_old_.Range(&intr_copy.position_old_);
	Distance prevVerticalSeparation = Distance(abs(user_copy.position_old_.altitude_.to_feet() - intr_copy.position_old_.altitude_.to_feet()), Distance::DistanceUnits::FEET);
	Distance prevSlantRange = Decider::CalculateSlantRange(prevHorizontalSeparation, prevVerticalSeparation);

	Distance currHorizontalSeparation = user_copy.position_current_.Range(&intr_copy.position_current_);
	Distance currVerticalSeparation = Distance(abs(user_copy.position_current_.altitude_.to_feet() - intr_copy.position_current_.altitude_.to_feet()), Distance::DistanceUnits::FEET);
	Distance currSlantRange = Decider::CalculateSlantRange(currHorizontalSeparation, currVerticalSeparation);

	double intr_elapsed_time_minutes = Decider::ToMinutes(intr_copy.position_current_time_) - Decider::ToMinutes(intr_copy.position_old_time_);
	double user_elapsed_time_minutes = Decider::ToMinutes(user_copy.position_current_time_) - Decider::ToMinutes(user_copy.position_old_time_);

	char debug_buf[256];
	Distance alt_diff = user_copy.position_current_.altitude_ - user_copy.position_old_.altitude_;
	snprintf(debug_buf, 256, "Decider::DetermineActionRequired - ta_elapsed_time: %f, intr_el_time: %f, alt_diff: %f\n", user_elapsed_time_minutes, intr_elapsed_time_minutes, alt_diff.to_feet());
	XPLMDebugString(debug_buf);
	
	if (user_elapsed_time_minutes != 0.0 && intr_elapsed_time_minutes != 0.0 &&  alt_diff.to_feet() != 0.0) {
		Velocity horizontalRate = Velocity((currHorizontalSeparation - prevHorizontalSeparation).to_feet() / user_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);
		double horizontalTau = currHorizontalSeparation.to_feet() / horizontalRate.to_feet_per_min();
		Velocity verticalRate = Velocity((currVerticalSeparation - prevVerticalSeparation).to_feet() / user_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);
		double verticalTau = currVerticalSeparation.to_feet() / verticalRate.to_feet_per_min();

		Velocity slantRangeRate = Decider::CalculateSlantRangeRate(horizontalRate, verticalRate, user_elapsed_time_minutes);
		double slantRangeTau = currSlantRange.to_feet() / slantRangeRate.to_feet_per_min(); //Time to Closest Point of Approach (CPA)

		Velocity inHorizontalVelocity = Velocity(intr_copy.position_current_.Range(&intr_copy.position_old_).to_feet() / intr_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);
		Velocity inVerticalVelocity = Velocity((intr_copy.position_current_.altitude_ - intr_copy.position_old_.altitude_).to_feet() / intr_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);

		Velocity taHorizontalVelocity = Velocity(user_copy.position_current_.Range(&user_copy.position_old_).to_feet() / user_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);

		Aircraft::ThreatClassification threat_class;
		ResolutionConnection* connection = (*active_connections)[intr_copy.id_];
		Sense sense = Decider::DetermineResolutionSense(user_copy.position_current_.altitude_, intr_copy.position_current_.altitude_, user_copy.vertical_velocity_, inVerticalVelocity, verticalTau);

		if (currSlantRange.to_feet() < kProtectionVolumeRadius_.to_feet()) {
			bool consensus_copy;
			Sense sense_copy;

			connection->lock.lock();
			consensus_copy = connection->consensusAchieved;
			sense_copy = connection->current_sense;

			if (consensus_copy) {
				XPLMDebugString("\n\nPARTY!\n\n");
				connection->lock.unlock();
			} else if (sense_copy == Sense::UNKNOWN) {
				connection->current_sense = sense;
				connection->lock.unlock();
				connection->sendSense(sense);
			}
			else {
				connection->lock.unlock();
			}

			threat_class = ReevaluateProximinityIntruderThreatClassification(horizontalTau, verticalTau, intr_copy.threat_classification_);
		} else {
			threat_class = Aircraft::ThreatClassification::NON_THREAT_TRAFFIC;
		}

		debug_buf[0] = '\0';
		snprintf(debug_buf, 256, "Decider::DetermineActionRequired - intruderId: %s, currentSlantRange: %.3f, horizontalTau: %.3f, verticalTau: %.3f, threat_class: %s \n", intruder->id_.c_str(), currSlantRange.to_feet(), horizontalTau, verticalTau, get_threat_class_str(threat_class).c_str());
		XPLMDebugString(debug_buf);

		/*double strength = Decider::DetermineStrength(taVerticalVelocity, inVerticalVelocity, sense, slantRangeTau);

		recommendation_range_lock_.lock();

		if (sense == Sense::UPWARD) {
			positive_recommendation_range_.valid = true;
			positive_recommendation_range_.min_vertical_speed = Velocity(strength, Velocity::VelocityUnits::FEET_PER_MIN);
			positive_recommendation_range_.max_vertical_speed = Velocity(-4000.0, Velocity::VelocityUnits::FEET_PER_MIN);
		} else if (sense == Sense::DOWNWARD) {
			negative_recommendation_range_.valid = true;
			negative_recommendation_range_.min_vertical_speed = Velocity(strength, Velocity::VelocityUnits::FEET_PER_MIN);
			negative_recommendation_range_.max_vertical_speed = Velocity(-4000.0, Velocity::VelocityUnits::FEET_PER_MIN);
		}
		recommendation_range_lock_.unlock();*/

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

Sense Decider::DetermineResolutionSense(Distance user_current_altitude, Distance intr_current_altitude, Velocity user_vvel, Velocity intr_vvel, double vertical_tau) {
	Velocity verticalRateDelta = Velocity(1500.0, Velocity::VelocityUnits::FEET_PER_MIN);
	Distance minVertSep = Distance(3000, Distance::DistanceUnits::FEET);

	// The projected altitude of the intruding aircraft at the CPA assuming the intruder maintains its current vertical velocity
	Distance intr_projected_altitude = Distance(intr_current_altitude.to_feet() + intr_vvel.to_feet_per_min() * vertical_tau, Distance::DistanceUnits::FEET);

	// Account for user delay in reacting
	vertical_tau -= 5.0;

	Velocity user_vvel_climb = Velocity(user_vvel.to_feet_per_min() + verticalRateDelta.to_feet_per_min(), Velocity::VelocityUnits::FEET_PER_MIN);
	Distance user_projected_altitude_climbing = Distance(user_current_altitude.to_feet() + user_vvel_climb.to_feet_per_min() * vertical_tau, Distance::DistanceUnits::FEET);

	Velocity user_vvel_descend = Velocity(user_vvel.to_feet_per_min() - verticalRateDelta.to_feet_per_min(), Velocity::VelocityUnits::FEET_PER_MIN);
	Distance user_projected_altitude_descending = Distance(user_current_altitude.to_feet() + user_vvel_descend.to_feet_per_min() * vertical_tau, Distance::DistanceUnits::FEET);

	Distance vertical_separation_at_cpa_climbing = user_projected_altitude_climbing - intr_projected_altitude;
	Distance vertical_separation_at_cpa_descending = user_projected_altitude_descending - intr_projected_altitude;

	if (abs(vertical_separation_at_cpa_climbing.to_feet()) > abs(vertical_separation_at_cpa_descending.to_feet())) {
		return Sense::UPWARD;
	}
	else {
		return Sense::DOWNWARD;
	}
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

Distance Decider::CalculateSlantRange(Distance horizontalSeparation, Distance verticalSeparation) {
	return Distance(sqrt(horizontalSeparation.to_feet() * horizontalSeparation.to_feet() + verticalSeparation.to_feet() * verticalSeparation.to_feet()), Distance::DistanceUnits::FEET);
}

Velocity Decider::CalculateSlantRangeRate(Velocity horizontalRate, Velocity verticalRate, double elapsed_time_minutes) {
	return Velocity(sqrt((horizontalRate.to_feet_per_min() * horizontalRate.to_feet_per_min() 
		+ verticalRate.to_feet_per_min() * verticalRate.to_feet_per_min())) / elapsed_time_minutes, 
		Velocity::VelocityUnits::FEET_PER_MIN);
}