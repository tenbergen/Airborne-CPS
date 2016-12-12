#include "Decider.h"

Velocity const Decider::kMinGaugeVerticalVelocity = {-4000.0, Velocity::VelocityUnits::FEET_PER_MIN};
Velocity const Decider::kMaxGaugeVerticalVelocity = {4000.0, Velocity::VelocityUnits::FEET_PER_MIN};

Distance const Decider::kProtectionVolumeRadius_ = { 30.0, Distance::DistanceUnits::NMI };

Distance const Decider::kAlim350_ = { 350.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAlim400_ = { 400.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAlim600_ = { 600.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAlim700_ = { 700.0, Distance::DistanceUnits::FEET };

Distance const Decider::kAltitudeAlim350Threshold_ = { 5000.0, Distance::DistanceUnits::FEET};
Distance const Decider::kAltitudeAlim400Threshold_ = { 10000.0, Distance::DistanceUnits::FEET};
Distance const Decider::kAltitudeAlim600Threshold_ = { 20000.0, Distance::DistanceUnits::FEET};

Velocity const Decider::kVerticalVelocityClimbDescendDelta_ = {1500.0, Velocity::VelocityUnits::FEET_PER_MIN};

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
		double horizontalTau = abs(currHorizontalSeparation.to_feet() / horizontalRate.to_feet_per_min());
		Velocity verticalRate = Velocity((currVerticalSeparation - prevVerticalSeparation).to_feet() / user_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);
		double verticalTau = abs(currVerticalSeparation.to_feet() / verticalRate.to_feet_per_min());

		Velocity slantRangeRate = Decider::CalculateSlantRangeRate(horizontalRate, verticalRate, user_elapsed_time_minutes);
		double slantRangeTau = abs(currSlantRange.to_feet() / slantRangeRate.to_feet_per_min()); //Time to Closest Point of Approach (CPA)

		Velocity inHorizontalVelocity = Velocity(intr_copy.position_current_.Range(&intr_copy.position_old_).to_feet() / intr_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);
		Velocity inVerticalVelocity = Velocity((intr_copy.position_current_.altitude_ - intr_copy.position_old_.altitude_).to_feet() / intr_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);

		Velocity taHorizontalVelocity = Velocity(user_copy.position_current_.Range(&user_copy.position_old_).to_feet() / user_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);

		Aircraft::ThreatClassification threat_class;
		ResolutionConnection* connection = (*active_connections)[intr_copy.id_];
		Sense sense = Decider::DetermineResolutionSense(user_copy.position_current_.altitude_, intr_copy.position_current_.altitude_, user_copy.vertical_velocity_, inVerticalVelocity, verticalTau);
		bool consensus = false;

		if (currSlantRange.to_feet() < kProtectionVolumeRadius_.to_feet()) {
			Sense sense_copy;

			connection->lock.lock();
			consensus = connection->consensusAchieved;
			sense_copy = connection->current_sense;

			if (consensus) {
				sense = sense_copy;
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
		snprintf(debug_buf, 256, "Decider::DetermineActionRequired - intruderId: %s, currentSlantRange: %.3f, horizontalTau: %.3f, verticalTau: %.3f, threat_class: %s, consensus: %s, sense: %s\n", intruder->id_.c_str(), currSlantRange.to_feet(), horizontalTau, verticalTau, get_threat_class_str(threat_class).c_str(), consensus ? "true" : "false", SenseUtil::StringFromSense(sense).c_str());
		XPLMDebugString(debug_buf);

		double min_tau_minutes = verticalTau < horizontalTau ? verticalTau : horizontalTau;
		RecommendationRangePair strength = Decider::DetermineStrength(sense, user_copy.vertical_velocity_, inVerticalVelocity, 
			user_copy.position_current_.altitude_, intr_copy.position_current_.altitude_, min_tau_minutes);

		recommendation_range_lock_.lock();

		positive_recommendation_range_ = strength.positive;
		negative_recommendation_range_ = strength.negative;
		
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

Sense Decider::DetermineResolutionSense(Distance user_current_altitude, Distance intr_current_altitude, Velocity user_vvel, Velocity intr_vvel, double vertical_tau) {
	Distance ALIM = DetermineALIM(user_current_altitude);

	// The projected altitude of the intruding aircraft at the CPA assuming the intruder maintains its current vertical velocity
	Distance intr_projected_altitude = Distance(intr_current_altitude.to_feet() + intr_vvel.to_feet_per_min() * vertical_tau, Distance::DistanceUnits::FEET);

	bool user_below_intr_initially = user_current_altitude.to_feet() < intr_current_altitude.to_feet();

	// Account for user delay in reacting as 5.0 seconds = 5/60 = 1/12; the acceleration of the aircraft at 0.25g should also be taken into account but is not
	vertical_tau -= (1.0 / 12.0);
	if (vertical_tau > 0.0) {
		Velocity user_vvel_climb = user_vvel + kVerticalVelocityClimbDescendDelta_;
		Distance user_projected_altitude_climbing = Distance(user_current_altitude.to_feet() + user_vvel_climb.to_feet_per_min() * vertical_tau, Distance::DistanceUnits::FEET);
		bool climbing_crosses_altitude = user_below_intr_initially && user_projected_altitude_climbing.to_feet() > intr_projected_altitude.to_feet();

		Velocity user_vvel_descend = user_vvel - kVerticalVelocityClimbDescendDelta_;
		Distance user_projected_altitude_descending = Distance(user_current_altitude.to_feet() + user_vvel_descend.to_feet_per_min() * vertical_tau, Distance::DistanceUnits::FEET);
		bool descending_crosses_altitude = user_below_intr_initially && user_projected_altitude_descending.to_feet() < intr_projected_altitude.to_feet();

		Distance vertical_separation_at_cpa_climbing = user_projected_altitude_climbing - intr_projected_altitude;
		Distance vertical_separation_at_cpa_descending = user_projected_altitude_descending - intr_projected_altitude;

		if (abs(vertical_separation_at_cpa_climbing.to_feet()) > abs(vertical_separation_at_cpa_descending.to_feet())) {
			// The sense returned should avoid crossing altitudes unless the desired amount of vertical safe distance, ALIM, can be acheived
			if (!climbing_crosses_altitude || climbing_crosses_altitude && abs(vertical_separation_at_cpa_climbing.to_feet()) > ALIM.to_feet()) {
				return Sense::UPWARD;
			}
			else {
				return Sense::UPWARD;
			}
		}
		else {
			if (!descending_crosses_altitude || descending_crosses_altitude && abs(vertical_separation_at_cpa_descending.to_feet()) > ALIM.to_feet()) {
				return Sense::DOWNWARD;
			}
			else {
				return Sense::UPWARD;
			}
		}
	}
	else {
		return Sense::UNKNOWN;
	}
}

bool TrajectoryAchievesALIM(Distance user_initial_altitude, Distance intr_projected_altitude, Distance ALIM, Velocity velocity, double tau) {
	return abs((user_initial_altitude.to_feet() + velocity.to_feet_per_min() * tau) - intr_projected_altitude.to_feet()) > ALIM.to_feet();
}

Velocity Decider::DetermineRelativeMinimumVerticalVelocityToAchieveAlim(Distance ALIM, Distance separation_at_cpa, double tau_minutes) const {
	double sign = signbit(separation_at_cpa.to_feet()) ? -1.0 : 1.0;
	return Velocity(sign * (ALIM.to_feet() - abs(separation_at_cpa.to_feet())) / tau_minutes, Velocity::VelocityUnits::FEET_PER_MIN);
}

RecommendationRangePair Decider::DetermineStrength(Sense sense, Velocity user_vvel, Velocity intr_vvel, Distance user_altitude, 
	Distance intr_altitude, double tau_minutes) const {
	Distance ALIM = DetermineALIM(user_altitude);
	// Account for delay in user reaction of 5 seconds ( 5 / 60 == 1 / 12)
	tau_minutes -= (1.0 / 12.0);

	// The projected altitude of the intruding aircraft at the CPA assuming the intruder maintains its current vertical velocity
	Distance intr_projected_altitude = Distance(intr_altitude.to_feet() + intr_vvel.to_feet_per_min() * tau_minutes, Distance::DistanceUnits::FEET);
	Distance user_projected_altitude = Distance(user_altitude.to_feet() + user_vvel.to_feet_per_min() * tau_minutes, Distance::DistanceUnits::FEET);

	Distance separation_at_cpa = user_projected_altitude - intr_projected_altitude;

	Velocity relative_min_vvel_to_achieve_alim = DetermineRelativeMinimumVerticalVelocityToAchieveAlim(ALIM, separation_at_cpa, tau_minutes);
	/*double rounded_to_nearest_500th = math_util::RoundToNearest(relative_min_vvel_to_achieve_alim.to_feet_per_min(), 500.0);
	Velocity relative_min_vvel_rounded = Velocity(rounded_to_nearest_500th, Velocity::VelocityUnits::FEET_PER_MIN);*/
	Velocity absolute_min_vvel_to_achieve_alim = user_vvel + relative_min_vvel_to_achieve_alim;

	RecommendationRange positive, negative;

	Velocity positive_max_vvel = Velocity::ZERO;
	Velocity negative_max_vvel = Velocity::ZERO;

	if (sense == Sense::UPWARD) {
		positive_max_vvel = signbit(absolute_min_vvel_to_achieve_alim.to_feet_per_min())
			? kMaxGaugeVerticalVelocity :
			Velocity(absolute_min_vvel_to_achieve_alim.to_feet_per_min() + 500.0, Velocity::VelocityUnits::FEET_PER_MIN);
		negative_max_vvel = kMinGaugeVerticalVelocity;
	}
	else {
		positive_max_vvel = signbit(absolute_min_vvel_to_achieve_alim.to_feet_per_min())
			? Velocity(absolute_min_vvel_to_achieve_alim.to_feet_per_min() - 500.0, Velocity::VelocityUnits::FEET_PER_MIN)
			: kMinGaugeVerticalVelocity;
		negative_max_vvel = kMaxGaugeVerticalVelocity;
	}

	positive.min_vertical_speed = absolute_min_vvel_to_achieve_alim;
	positive.max_vertical_speed = positive_max_vvel;
	positive.valid = true;

	negative.min_vertical_speed = absolute_min_vvel_to_achieve_alim;
	negative.max_vertical_speed = negative_max_vvel;
	negative.valid = true;

	return RecommendationRangePair{ positive, negative };
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

Distance Decider::DetermineALIM(Distance user_altitude) const {
	if (user_altitude < kAltitudeAlim350Threshold_) {
		return kAlim350_;
	}
	else if (user_altitude < kAltitudeAlim400Threshold_) {
		return kAlim400_;
	}
	else if (user_altitude < kAltitudeAlim600Threshold_) {
		return kAlim600_;
	}
	else {
		return kAlim700_;
	}
}