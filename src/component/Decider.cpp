#include "Decider.h"

Velocity const Decider::kMinGaugeVerticalVelocity = { -4000.0, Velocity::VelocityUnits::FEET_PER_MIN };
Velocity const Decider::kMaxGaugeVerticalVelocity = { 4000.0, Velocity::VelocityUnits::FEET_PER_MIN };

Distance const Decider::kProtectionVolumeRadius_ = { 30.0, Distance::DistanceUnits::NMI };

Distance const Decider::kAlim350_ = { 350.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAlim400_ = { 400.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAlim600_ = { 600.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAlim700_ = { 700.0, Distance::DistanceUnits::FEET };

Distance const Decider::kAltitudeAlim350Threshold_ = { 5000.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAltitudeAlim400Threshold_ = { 10000.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAltitudeAlim600Threshold_ = { 20000.0, Distance::DistanceUnits::FEET };

Velocity const Decider::kVerticalVelocityClimbDescendDelta_ = { 1500.0, Velocity::VelocityUnits::FEET_PER_MIN };

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

	ResolutionConnection* connection = (*active_connections)[intr_copy.id_];
	Aircraft::ThreatClassification threat_class = Decider::DetermineThreatClass(&intr_copy, connection);
	Sense my_sense = Sense::UNKNOWN;

	if (threat_class == Aircraft::ThreatClassification::RESOLUTION_ADVISORY) {
		connection->lock.lock();
		if (connection->consensusAchieved) {
			my_sense = connection->current_sense;
		} else {
			my_sense = Decider::DetermineResolutionSense(user_copy.position_current_.altitude_.ToUnits(Distance::DistanceUnits::FEET),
				intr_copy.position_current_.altitude_.ToUnits(Distance::DistanceUnits::FEET));
			connection->sendSense(my_sense);
		}
		connection->lock.unlock();
	} else if (my_sense != Sense::UNKNOWN && threat_class == Aircraft::ThreatClassification::NON_THREAT_TRAFFIC) {
		connection->lock.lock();
		connection->consensusAchieved = false;
		connection->lock.unlock();
		my_sense = Sense::UNKNOWN;
	}

	intruder->lock_.lock();
	intruder->threat_classification_ = threat_class;
	intruder->time_of_cpa = intr_copy.time_of_cpa;
	intruder->lock_.unlock();
}

Aircraft::ThreatClassification Decider::DetermineThreatClass(Aircraft* intr_copy, ResolutionConnection* conn) {
	conn->lock.lock();
	LLA user_position = conn->user_position;
	LLA user_position_old = conn->user_position_old;
	std::chrono::milliseconds user_position_time = conn->user_position_time;
	std::chrono::milliseconds user_position_old_time = conn->user_position_old_time;
	conn->lock.unlock();
	Aircraft::ThreatClassification prev_threat_class = intr_copy->threat_classification_;
	double slant_range_nmi = user_position.Range(&intr_copy->position_current_).ToUnits(Distance::DistanceUnits::NMI);
	double delta_distance_m = user_position.Range(&intr_copy->position_current_).ToUnits(Distance::DistanceUnits::METERS)
		- user_position_old.Range(&intr_copy->position_old_).ToUnits(Distance::DistanceUnits::METERS);
	double elapsed_time_s = (double)(intr_copy->position_current_time_ - intr_copy->position_old_time_).count() / 1000;
	double closing_speed_knots = Velocity(delta_distance_m / elapsed_time_s, Velocity::VelocityUnits::METERS_PER_S).ToUnits(Velocity::VelocityUnits::KNOTS);
	double alt_sep_ft = intr_copy->position_current_.altitude_.ToUnits(Distance::DistanceUnits::FEET) -
		user_position.altitude_.ToUnits(Distance::DistanceUnits::FEET);
	double delta_distance2_ft = (intr_copy->position_old_.altitude_.ToUnits(Distance::DistanceUnits::FEET) -
		user_position_old.altitude_.ToUnits(Distance::DistanceUnits::FEET)) -
		(intr_copy->position_current_.altitude_.ToUnits(Distance::DistanceUnits::FEET) -
			user_position.altitude_.ToUnits(Distance::DistanceUnits::FEET));
	double elapsed_time_min = elapsed_time_s / 60;
	double vert_closing_spd_ft_p_min = delta_distance2_ft / elapsed_time_min;
	double range_tau_s = slant_range_nmi / closing_speed_knots * 3600;
	double vertical_tau_s = alt_sep_ft / vert_closing_spd_ft_p_min * 60;

	Aircraft::ThreatClassification new_threat_class;
	// if within proximity range
	if (slant_range_nmi < 6 && abs(alt_sep_ft) < 1200) {
		// if reaches TA threshold
		//if (prev_threat_class >= Aircraft::ThreatClassification::TRAFFIC_ADVISORY
		//	|| tau_passes_TA_threshold(user_position.altitude_.ToUnits(Distance::DistanceUnits::FEET), range_tau_s, vertical_tau_s)) {
		//	// if reaches RA threshold
		//	if (prev_threat_class == Aircraft::ThreatClassification::RESOLUTION_ADVISORY
		//		|| tau_passes_RA_threshold(user_position.altitude_.ToUnits(Distance::DistanceUnits::FEET), range_tau_s, vertical_tau_s)) {
		//		new_threat_class = Aircraft::ThreatClassification::RESOLUTION_ADVISORY;
		//		// if new RA
		//		if (intr_copy->time_of_cpa == std::chrono::milliseconds(0)
		//			|| prev_threat_class == Aircraft::ThreatClassification::TRAFFIC_ADVISORY)
		//			intr_copy->time_of_cpa = std::chrono::milliseconds(std::chrono::system_clock::now().time_since_epoch().count() + (int)(range_tau_s * 1000));
		//	} else {
		//		// did not rach RA threshold
		//		new_threat_class = Aircraft::ThreatClassification::TRAFFIC_ADVISORY;
		//		// if new TA
		//		if (intr_copy->time_of_cpa == std::chrono::milliseconds(0))
		//			intr_copy->time_of_cpa = std::chrono::milliseconds(std::chrono::system_clock::now().time_since_epoch().count() + (int)(range_tau_s * 1000));
		//	}
		//} else {
		// does not reach TA threshold
		new_threat_class = Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC;
		//}
	} else {
		// is not within proximity range
		new_threat_class = Aircraft::ThreatClassification::NON_THREAT_TRAFFIC;
	}

	return new_threat_class;
}

bool Decider::tau_passes_TA_threshold(double alt, double range_tau_s, double vertical_tau_s)
{
	if (alt < 1000 && range_tau_s < 20 && vertical_tau_s < 20) {
		return true;
	} else if (alt >= 1000 && alt < 2350 && range_tau_s < 25 && vertical_tau_s < 25) {
		return true;
	} else if (alt >= 2350 && alt < 5000 && range_tau_s < 30 && vertical_tau_s < 30) {
		return true;
	} else if (alt >= 5000 && alt < 10000 && range_tau_s < 40 && vertical_tau_s < 40) {
		return true;
	} else if (alt >= 10000 && alt < 20000 && range_tau_s < 45 && vertical_tau_s < 45) {
		return true;
	} else if (alt >= 20000 && range_tau_s < 48 && vertical_tau_s < 48) {
		return true;
	}

	return false;
}

bool Decider::tau_passes_RA_threshold(double alt, double range_tau_s, double vertical_tau_s)
{
	if (alt < 1000) {
		return false;
	} else if (alt >= 1000 && alt < 2350 && range_tau_s < 15 && vertical_tau_s < 15) {
		return true;
	} else if (alt >= 2350 && alt < 5000 && range_tau_s < 20 && vertical_tau_s < 20) {
		return true;
	} else if (alt >= 5000 && alt < 10000 && range_tau_s < 25 && vertical_tau_s < 25) {
		return true;
	} else if (alt >= 10000 && alt < 20000 && range_tau_s < 30 && vertical_tau_s < 30) {
		return true;
	} else if (alt >= 20000 && range_tau_s < 35 && vertical_tau_s < 35) {
		return true;
	}

	return false;
}

Sense Decider::DetermineResolutionSense(double user_alt_ft, double intr_alt_ft) {
	if (user_alt_ft > intr_alt_ft)
		return Sense::UPWARD;
	else
		return Sense::DOWNWARD;
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
			} else {
				positive_max_vvel = Velocity(absolute_min_vvel_to_achieve_alim.to_feet_per_min() + 500.0, Velocity::VelocityUnits::FEET_PER_MIN);
			}

			negative_max_vvel = kMinGaugeVerticalVelocity;
		} else {
			/* If the user is supposed to be descending and the required vertical velocity to achieve ALIM is negative,
			the recommended max velocity should be 500 fpm less than the absolute velocity; if the velocity to achieve ALIM is
			positive, then the recommended max velocity should be ?*/
			if (signbit(absolute_min_vvel_to_achieve_alim.to_feet_per_min())) {
				positive_max_vvel = Velocity(absolute_min_vvel_to_achieve_alim.to_feet_per_min() - 500.0, Velocity::VelocityUnits::FEET_PER_MIN);
			} else {
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
	} else {
		positive.valid = false;
		negative.valid = false;
	}

	return RecommendationRangePair{ positive, negative };
}

double Decider::ToMinutes(std::chrono::milliseconds time) {
	long long result1 = time.count();
	return ((double)result1) / 60000.0;
}

Distance Decider::DetermineALIM(Distance user_altitude) const {
	if (user_altitude < kAltitudeAlim350Threshold_) {
		return kAlim350_;
	} else if (user_altitude < kAltitudeAlim400Threshold_) {
		return kAlim400_;
	} else if (user_altitude < kAltitudeAlim600Threshold_) {
		return kAlim600_;
	} else {
		return kAlim700_;
	}
}