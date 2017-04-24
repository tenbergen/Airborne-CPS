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

void Decider::Analyze(Aircraft* intruder, Aircraft* user, ResolutionConnection* rc) {
	Decider::DetermineActionRequired(intruder, user, rc);
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

void Decider::DetermineActionRequired(Aircraft* intruder, Aircraft* user, ResolutionConnection* rc) {
	user->lock_.lock();
	Aircraft user_copy = *(user);
	user->lock_.unlock();

	intruder->lock_.lock();
	Aircraft intr_copy = *(intruder);
	intruder->lock_.unlock();

	rc->lock.lock();
	if (rc->time_of_cpa != std::chrono::milliseconds(0) && rc->time_of_cpa < std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())) {
		rc->time_of_cpa = std::chrono::milliseconds(0);
	}
	rc->lock.unlock();

	Distance prevHorizontalSeparation = user_copy.position_old_.Range(&intr_copy.position_old_);
	Distance prevVerticalSeparation = Distance(abs(user_copy.position_old_.altitude_.to_feet() - intr_copy.position_old_.altitude_.to_feet()), Distance::DistanceUnits::FEET);
	Distance prevSlantRange = Decider::CalculateSlantRange(prevHorizontalSeparation, prevVerticalSeparation);

	Distance currHorizontalSeparation = user_copy.position_current_.Range(&intr_copy.position_current_);
	Distance currVerticalSeparation = Distance(abs(user_copy.position_current_.altitude_.to_feet() - intr_copy.position_current_.altitude_.to_feet()), Distance::DistanceUnits::FEET);
	Distance currSlantRange = Decider::CalculateSlantRange(currHorizontalSeparation, currVerticalSeparation);

	double intr_elapsed_time_minutes = Decider::ToMinutes(intr_copy.position_current_time_) - Decider::ToMinutes(intr_copy.position_old_time_);
	double user_elapsed_time_minutes = Decider::ToMinutes(user_copy.position_current_time_) - Decider::ToMinutes(user_copy.position_old_time_);

	Distance alt_diff = user_copy.position_current_.altitude_ - user_copy.position_old_.altitude_;

	Aircraft::ThreatClassification threat_class;
	Velocity horizontalRate = Velocity((currHorizontalSeparation - prevHorizontalSeparation).to_feet() / user_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);
	Velocity verticalRate = Velocity((currVerticalSeparation - prevVerticalSeparation).to_feet() / user_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);
	double horizontalTauSeconds = abs(currHorizontalSeparation.to_feet() / horizontalRate.to_feet_per_min()) * 60.0;
	double verticalTauSeconds = abs(currVerticalSeparation.to_feet() / verticalRate.to_feet_per_min()) * 60.0;
	Velocity slantRangeRate = Decider::CalculateSlantRangeRate(horizontalRate, verticalRate, user_elapsed_time_minutes);
	double slantRangeTauSeconds = abs(currSlantRange.to_feet() / slantRangeRate.to_feet_per_min()) * 60.0; //Time to Closest Point of Approach (CPA)
	std::chrono::milliseconds time_of_cpa = std::chrono::milliseconds(0);
	if (intr_copy.threat_classification_ == Aircraft::ThreatClassification::RESOLUTION_ADVISORY
		|| intr_copy.threat_classification_ == Aircraft::ThreatClassification::TRAFFIC_ADVISORY) {
		rc->lock.lock();
		if (rc->time_of_cpa == std::chrono::milliseconds(0)) {
			rc->time_of_cpa = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) + std::chrono::milliseconds((int)(slantRangeTauSeconds * 1000));
		}
		time_of_cpa = rc->time_of_cpa;
		rc->lock.unlock();
	}

	if (user_elapsed_time_minutes != 0.0 && intr_elapsed_time_minutes != 0.0 &&  alt_diff.to_feet() != 0.0 && user_copy.position_current_.altitude_.to_feet() > 100.0) {
		
		// Ensure the aircraft are actually closing on each other
		if (!signbit(horizontalRate.to_feet_per_min()) && !signbit(verticalRate.to_feet_per_min())) {

			Velocity inHorizontalVelocity = Velocity(intr_copy.position_current_.Range(&intr_copy.position_old_).to_feet() / intr_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);
			Velocity inVerticalVelocity = Velocity((intr_copy.position_current_.altitude_ - intr_copy.position_old_.altitude_).to_feet() / intr_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);

			Velocity taHorizontalVelocity = Velocity(user_copy.position_current_.Range(&user_copy.position_old_).to_feet() / user_elapsed_time_minutes, Velocity::VelocityUnits::FEET_PER_MIN);
			
			ResolutionConnection* connection = (*active_connections)[intr_copy.id_];
			Sense sense = Decider::DetermineResolutionSense(user_copy.position_current_.altitude_, intr_copy.position_current_.altitude_, user_copy.vertical_velocity_, inVerticalVelocity, slantRangeTauSeconds);
			bool consensus = false;

			std::chrono::milliseconds last_analyzed_copy = connection->last_analyzed;

			if (currSlantRange.to_feet() < kProtectionVolumeRadius_.to_feet()) {
				threat_class = ReevaluateProximinityIntruderThreatClassification(horizontalTauSeconds, verticalTauSeconds, intr_copy.threat_classification_, time_of_cpa);

				if (threat_class == Aircraft::ThreatClassification::RESOLUTION_ADVISORY) {
					Sense sense_copy;

					connection->lock.lock();
					consensus = connection->consensusAchieved;
					sense_copy = connection->current_sense;
					connection->last_analyzed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

					if (consensus) {
						sense = sense_copy;
						connection->lock.unlock();

						RecommendationRangePair strength = Decider::DetermineStrength(sense, user_copy.vertical_velocity_, inVerticalVelocity,
							user_copy.position_current_.altitude_, intr_copy.position_current_.altitude_, slantRangeTauSeconds);

						recommendation_range_lock_.lock();
						positive_recommendation_range_ = strength.positive;
						negative_recommendation_range_ = strength.negative;
						recommendation_range_lock_.unlock();
					}
					else if (sense_copy == Sense::UNKNOWN && sense != Sense::UNKNOWN) {
						connection->current_sense = sense;
						connection->lock.unlock();

						connection->sendSense(sense);
					}
					else {
						connection->lock.unlock();
					}
				}
				else {
					recommendation_range_lock_.lock();
					positive_recommendation_range_.valid = false;
					negative_recommendation_range_.valid = false;
					recommendation_range_lock_.unlock();
				}
			}
			else {
				threat_class = Aircraft::ThreatClassification::NON_THREAT_TRAFFIC;
				// if the intruder has left the protection volume for more than ten seconds, we should reset the sense consensus
				if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch() - last_analyzed_copy).count() > 10000) {
					connection->lock.lock();
					connection->consensusAchieved = false;
					connection->lock.unlock();
				}

				recommendation_range_lock_.lock();
				positive_recommendation_range_.valid = false;
				negative_recommendation_range_.valid = false;
				recommendation_range_lock_.unlock();
			}

			intruder->lock_.lock();
			intruder->threat_classification_ = threat_class;
			intruder->lock_.unlock();
		}
	}
	else {
		threat_class = ReevaluateProximinityIntruderThreatClassification(horizontalTauSeconds, verticalTauSeconds, intr_copy.threat_classification_, time_of_cpa);
		intruder->lock_.lock();
		intruder->threat_classification_ = threat_class;
		intruder->lock_.unlock();
	}
}

Aircraft::ThreatClassification Decider::ReevaluateProximinityIntruderThreatClassification(double horizontal_tau, double vertical_tau, Aircraft::ThreatClassification current_threat_class, std::chrono::milliseconds time_of_cpa) const {
	if ((current_threat_class == Aircraft::ThreatClassification::RESOLUTION_ADVISORY || 
		(horizontal_tau < raThresholdSeconds && vertical_tau < raThresholdSeconds))
		&& time_of_cpa > std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())) {
		return Aircraft::ThreatClassification::RESOLUTION_ADVISORY;
	} else if((current_threat_class == Aircraft::ThreatClassification::TRAFFIC_ADVISORY || 
		(horizontal_tau < taThresholdSeconds && vertical_tau < taThresholdSeconds))
		&& time_of_cpa < std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())) {
		return Aircraft::ThreatClassification::TRAFFIC_ADVISORY;
	}
	else {
		return Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC;
	}
}

Sense Decider::DetermineResolutionSense(Distance user_current_altitude, Distance intr_current_altitude, Velocity user_vvel, Velocity intr_vvel, double slant_tau_seconds) {
	Distance ALIM = DetermineALIM(user_current_altitude);

	// The projected altitude of the intruding aircraft at the CPA assuming the intruder maintains its current vertical velocity
	Distance intr_projected_altitude = Distance(intr_current_altitude.to_feet() + intr_vvel.to_feet_per_min() * (slant_tau_seconds / 60), Distance::DistanceUnits::FEET);

	// Used for determining if climb or descend will cross altitude
	bool user_below_intr_initially = user_current_altitude < intr_current_altitude;

	// Account for user delay in reacting as 5.0 seconds; the acceleration of the aircraft at 0.25g should also be taken into account but is not
	slant_tau_seconds -= 5.0;

	if (slant_tau_seconds > 0.0) {
		/* Project the user's final altitude based upon climbining at 1500 fpm greater than the current vertical 
		 velocity for slant_tau_seconds (which is time to CPA)*/
		Velocity user_vvel_climb = user_vvel + kVerticalVelocityClimbDescendDelta_;
		Distance user_projected_altitude_climbing = Distance(user_current_altitude.to_feet() + user_vvel_climb.to_feet_per_min() * (slant_tau_seconds / 60.0), Distance::DistanceUnits::FEET);
		bool climbing_crosses_altitude = user_below_intr_initially && user_projected_altitude_climbing.to_feet() > intr_projected_altitude.to_feet();

		/* Same thing as above but projecting the user's final altitude when descending at 1500 fpm less than current
		vertical velocity*/
		Velocity user_vvel_descend = user_vvel - kVerticalVelocityClimbDescendDelta_;
		Distance user_projected_altitude_descending = Distance(user_current_altitude.to_feet() + user_vvel_descend.to_feet_per_min() * (slant_tau_seconds / 60), Distance::DistanceUnits::FEET);
		bool descending_crosses_altitude = user_below_intr_initially && user_projected_altitude_descending.to_feet() < intr_projected_altitude.to_feet();

		Distance vertical_separation_at_cpa_climbing = user_projected_altitude_climbing - intr_projected_altitude;
		Distance vertical_separation_at_cpa_descending = user_projected_altitude_descending - intr_projected_altitude;

		/* The sense should be the direction that results in the greatest vertical separation at the CPA  but crossing altitudes 
		with the intruding aircraft should be avoided if it can be*/
		if (abs(vertical_separation_at_cpa_climbing.to_feet()) > abs(vertical_separation_at_cpa_descending.to_feet())) {
			/* The sense returned should avoid crossing altitudes unless the desired amount of vertical safe distance, ALIM, 
			 can be acheived */
			// RETURN WITH NO CROSSING ALLOWED TEMPORARILY
			if (!climbing_crosses_altitude) {
				return Sense::UPWARD;
			}
			else {
				return Sense::DOWNWARD;
			}
		}
		else {
			if (!descending_crosses_altitude) {
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

Velocity Decider::DetermineRelativeMinimumVerticalVelocityToAchieveAlim(Distance ALIM, Distance separation_at_cpa, double tau_seconds) const {
	double sign = signbit(separation_at_cpa.to_feet()) ? -1.0 : 1.0;
	return Velocity(sign * (ALIM.to_feet() - abs(separation_at_cpa.to_feet())) / tau_seconds, Velocity::VelocityUnits::FEET_PER_MIN);
}

RecommendationRangePair Decider::DetermineStrength(Sense sense, Velocity user_vvel, Velocity intr_vvel, Distance user_altitude, 
	Distance intr_altitude, double tau_seconds) const {
	RecommendationRange positive, negative;
	// Account for delay in user reaction by subtracting 5 seconds; tau is time to reaching closest of approach (cpa)
	tau_seconds -= 5.0;

	if (sense != Sense::UNKNOWN && tau_seconds > 0.0) {
		Distance ALIM = DetermineALIM(user_altitude);

		// The projected altitude of the intruding aircraft at the CPA assuming the intruder maintains its current vertical velocity
		Distance intr_projected_altitude_at_cpa = Distance(intr_altitude.to_feet() + intr_vvel.to_feet_per_min() * (tau_seconds / 60.0), Distance::DistanceUnits::FEET);
		Distance user_projected_altitude_at_cpa = Distance(user_altitude.to_feet() + user_vvel.to_feet_per_min() * (tau_seconds / 60.0), Distance::DistanceUnits::FEET);

		Distance separation_at_cpa = user_projected_altitude_at_cpa - intr_projected_altitude_at_cpa;
		bool alim_achieved = abs(separation_at_cpa.to_feet()) > ALIM.to_feet();

		Velocity relative_min_vvel_to_achieve_alim = DetermineRelativeMinimumVerticalVelocityToAchieveAlim(ALIM, separation_at_cpa, tau_seconds);
		// Haven't tried this but should probably round the recommended vertical velocity to the nearest 500 fpm multiple once it works
		/*double rounded_to_nearest_500th = math_util::RoundToNearest(relative_min_vvel_to_achieve_alim.to_feet_per_min(), 500.0);
		Velocity relative_min_vvel_rounded = Velocity(rounded_to_nearest_500th, Velocity::VelocityUnits::FEET_PER_MIN);*/

		/* The velocity determined above is relative to the user's vertical velocity as it is calculated based upon the projected separation
		 at the CPA assuming the user and intruder maintain the same trajectory, so to calculate the absolute velocity, the determined 
		 relative vertical velocity should be added to the user's vertical velocity */
		Velocity absolute_min_vvel_to_achieve_alim = alim_achieved ? user_vvel : user_vvel + relative_min_vvel_to_achieve_alim;

		Velocity positive_max_vvel = Velocity::ZERO;
		Velocity negative_max_vvel = Velocity::ZERO;

		/* The only values that should differ between the upward and downward senses are the positive and negative ranges' maximum vertical velocity */
		if (sense == Sense::UPWARD) {
			// This logic needs to be changed; if your sense is upward and you're supposed to descend, what should you do? Maintain?
			if (signbit(absolute_min_vvel_to_achieve_alim.to_feet_per_min())) {
				positive_max_vvel = absolute_min_vvel_to_achieve_alim;
			}
			else {
				positive_max_vvel = Velocity(absolute_min_vvel_to_achieve_alim.to_feet_per_min() + 500.0, Velocity::VelocityUnits::FEET_PER_MIN);
			}

			negative_max_vvel = kMinGaugeVerticalVelocity;
		}
		else {
			/* If the user is supposed to be descending and the required vertical velocity to achieve ALIM is negative,
			the recommended max velocity should be 500 fpm less than the absolute velocity; if the velocity to achieve ALIM is
			positive, then the recommended max velocity should be ?*/
			if (signbit(absolute_min_vvel_to_achieve_alim.to_feet_per_min())) {
				positive_max_vvel = Velocity(absolute_min_vvel_to_achieve_alim.to_feet_per_min() - 500.0, Velocity::VelocityUnits::FEET_PER_MIN);
			}
			else {
				positive_max_vvel = kMinGaugeVerticalVelocity;
			}
			
			negative_max_vvel = kMaxGaugeVerticalVelocity;
		}

		positive.min_vertical_speed = absolute_min_vvel_to_achieve_alim;
		positive.max_vertical_speed = positive_max_vvel;
		positive.valid = true;

		negative.min_vertical_speed = absolute_min_vvel_to_achieve_alim;
		negative.max_vertical_speed = negative_max_vvel;
		negative.valid = true;
	}
	else {
		positive.valid = false;
		negative.valid = false;
	}

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