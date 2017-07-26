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
	intruder->lock_.lock();
	Aircraft intr_copy = *(intruder);
	intruder->lock_.unlock();

	ResolutionConnection* connection = (*active_connections)[intr_copy.id_];
	Aircraft::ThreatClassification threat_class = Decider::DetermineThreatClass(&intr_copy, connection);
	Sense my_sense = Sense::UNKNOWN;

	RecommendationRangePair rec_range;

	if (threat_class == Aircraft::ThreatClassification::RESOLUTION_ADVISORY) {
		connection->lock.lock();
		if (connection->consensusAchieved && (connection->current_sense == Sense::UPWARD || connection->current_sense == Sense::DOWNWARD)) {
			my_sense = connection->current_sense;
		} else {
			my_sense = Decider::DetermineResolutionSense(connection->user_position.altitude_.ToUnits(Distance::DistanceUnits::FEET),
				intr_copy.position_current_.altitude_.ToUnits(Distance::DistanceUnits::FEET));
			connection->sendSense(my_sense);
		}
		connection->lock.unlock();

		double user_delta_pos_m = connection->user_position.Range(&connection->user_position_old).to_meters();
		double user_delta_alt_m = abs(connection->user_position.altitude_.to_meters() - connection->user_position_old.altitude_.to_meters());
		double intr_delta_pos_m = intr_copy.position_current_.Range(&intr_copy.position_old_).to_meters();
		double intr_delta_alt_m = abs(intr_copy.position_current_.altitude_.to_meters() - intr_copy.position_old_.altitude_.to_meters());
		double user_elapsed_time_s = (double)(connection->user_position_time - connection->user_position_old_time).count() / 1000;
		double intr_elapsed_time_s = (double)(intr_copy.position_current_time_ - intr_copy.position_old_time_).count() / 1000;
		double slant_range_nmi = abs(connection->user_position.Range(&intr_copy.position_current_).ToUnits(Distance::DistanceUnits::NMI));
		double delta_distance_m = abs(connection->user_position_old.Range(&intr_copy.position_old_).ToUnits(Distance::DistanceUnits::METERS))
			- abs(connection->user_position.Range(&intr_copy.position_current_).ToUnits(Distance::DistanceUnits::METERS));
		double closing_speed_knots = Velocity(delta_distance_m / intr_elapsed_time_s, Velocity::VelocityUnits::METERS_PER_S).ToUnits(Velocity::VelocityUnits::KNOTS);
		Velocity user_vvel = Velocity(user_delta_alt_m / user_elapsed_time_s, Velocity::VelocityUnits::METERS_PER_S);
		Velocity intr_vvel = Velocity(intr_delta_alt_m / intr_elapsed_time_s, Velocity::VelocityUnits::METERS_PER_S);
		double range_tau_s = get_mod_tau_s(slant_range_nmi, closing_speed_knots, get_ra_dmod_nmi(connection->user_position.altitude_.to_feet()));
		rec_range = get_rec_range_pair(my_sense, user_vvel.to_feet_per_min(), intr_vvel.to_feet_per_min(), connection->user_position.altitude_.to_feet(), intr_copy.position_current_.altitude_.to_feet(), range_tau_s);

	} else if (threat_class == Aircraft::ThreatClassification::NON_THREAT_TRAFFIC) {
		connection->lock.lock();
		connection->current_sense = Sense::UNKNOWN;
		connection->lock.unlock();
		rec_range.negative.valid = false;
		rec_range.positive.valid = false;
	}

	
	recommendation_range_lock_.lock();
	positive_recommendation_range_ = rec_range.positive;
	negative_recommendation_range_ = rec_range.negative;
	recommendation_range_lock_.unlock();

	intruder->lock_.lock();
	intruder->threat_classification_ = threat_class;
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
	double slant_range_nmi = abs(user_position.Range(&intr_copy->position_current_).ToUnits(Distance::DistanceUnits::NMI));
	double delta_distance_m = abs(user_position_old.Range(&intr_copy->position_old_).ToUnits(Distance::DistanceUnits::METERS))
		- abs(user_position.Range(&intr_copy->position_current_).ToUnits(Distance::DistanceUnits::METERS));
	double elapsed_time_s = (double)(intr_copy->position_current_time_ - intr_copy->position_old_time_).count() / 1000;
	double closing_speed_knots = Velocity(delta_distance_m / elapsed_time_s, Velocity::VelocityUnits::METERS_PER_S).ToUnits(Velocity::VelocityUnits::KNOTS);
	double alt_sep_ft = abs(intr_copy->position_current_.altitude_.ToUnits(Distance::DistanceUnits::FEET) -
		user_position.altitude_.ToUnits(Distance::DistanceUnits::FEET));
	double delta_distance2_ft = abs(intr_copy->position_old_.altitude_.ToUnits(Distance::DistanceUnits::FEET) -
		user_position_old.altitude_.ToUnits(Distance::DistanceUnits::FEET)) -
		abs(intr_copy->position_current_.altitude_.ToUnits(Distance::DistanceUnits::FEET) -
			user_position.altitude_.ToUnits(Distance::DistanceUnits::FEET));
	double elapsed_time_min = elapsed_time_s / 60;
	double vert_closing_spd_ft_p_min = delta_distance2_ft / elapsed_time_min;
	double range_tau_s = slant_range_nmi / closing_speed_knots * 3600;
	double vertical_tau_s = alt_sep_ft / vert_closing_spd_ft_p_min * 60;
	Velocity user_velocity = Velocity(user_position.Range(&user_position_old).to_meters() / ((user_position_time.count() - user_position_old_time.count()) / 1000), Velocity::VelocityUnits::METERS_PER_S);
	Velocity intr_velocity = Velocity(intr_copy->position_current_.Range(&intr_copy->position_old_).to_meters() / ((intr_copy->position_current_time_.count() - intr_copy->position_old_time_.count()) / 1000), Velocity::VelocityUnits::METERS_PER_S);
	Distance user_distance_by_cpa = Distance(user_velocity.to_meters_per_s() * range_tau_s, Distance::DistanceUnits::METERS);
	Distance intr_distance_by_cpa = Distance(intr_velocity.to_meters_per_s() * range_tau_s, Distance::DistanceUnits::METERS);
	LLA user_position_at_cpa = user_position.Translate(&user_position_old.Bearing(&user_position), &user_distance_by_cpa);
	LLA intr_position_at_cpa = intr_copy->position_current_.Translate(&intr_copy->position_old_.Bearing(&intr_copy->position_current_), &intr_distance_by_cpa);
	double distance_at_cpa_ft = user_position_at_cpa.Range(&intr_position_at_cpa).to_feet();
	double ta_mod_tau_s = get_mod_tau_s(slant_range_nmi, closing_speed_knots, get_ta_dmod_nmi(user_position.altitude_.to_feet()));
	double ra_mod_tau_s = get_mod_tau_s(slant_range_nmi, closing_speed_knots, get_ra_dmod_nmi(user_position.altitude_.to_feet()));

	Aircraft::ThreatClassification new_threat_class;
	// if within proximity range
	if (slant_range_nmi < 6 && abs(alt_sep_ft) < 1200) {
		// if passes TA threshold
		if (closing_speed_knots > 0
			&& (prev_threat_class >= Aircraft::ThreatClassification::TRAFFIC_ADVISORY
			|| tau_passes_TA_threshold(user_position.altitude_.to_feet(), ta_mod_tau_s, vertical_tau_s, alt_sep_ft))) {
			// if passes RA threshold
			if (prev_threat_class == Aircraft::ThreatClassification::RESOLUTION_ADVISORY
				|| tau_passes_RA_threshold(user_position.altitude_.to_feet(), ra_mod_tau_s, vertical_tau_s, alt_sep_ft)) {
				new_threat_class = Aircraft::ThreatClassification::RESOLUTION_ADVISORY;
			} else {
				// did not pass RA threshold -- Traffic Advisory
				new_threat_class = Aircraft::ThreatClassification::TRAFFIC_ADVISORY;
			}
		} else {
			// did not pass TA threshold -- just Proximity Traffic
			new_threat_class = Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC;
		}
	} else {
		// is not within proximity range
		new_threat_class = Aircraft::ThreatClassification::NON_THREAT_TRAFFIC;
	}

	return new_threat_class;
}

bool Decider::tau_passes_TA_threshold(double alt_ft, double mod_tau_s, double vert_tau_s, double v_sep_ft)
{
	if (v_sep_ft > get_ta_zthr_ft(alt_ft)) {
		if (alt_ft < 1000 && mod_tau_s < 20 && vert_tau_s < 20)
			return true;
		else if (alt_ft < 2350 && mod_tau_s < 25 && vert_tau_s < 25)
			return true;
		else if (alt_ft < 5000 && mod_tau_s < 30 && vert_tau_s < 30)
			return true;
		else if (alt_ft < 10000 && mod_tau_s < 40 && vert_tau_s < 40)
			return true;
		else if (alt_ft < 20000 && mod_tau_s < 45 && vert_tau_s < 45)
			return true;
		else if (alt_ft >= 20000 && mod_tau_s < 48 && vert_tau_s < 48)
			return true;
		else
			return false;
	} else {
		if (alt_ft < 1000 && mod_tau_s < 20)
			return true;
		else if (alt_ft < 2350 && mod_tau_s < 25)
			return true;
		else if (alt_ft < 5000 && mod_tau_s < 30)
			return true;
		else if (alt_ft < 10000 && mod_tau_s < 40)
			return true;
		else if (alt_ft < 20000 && mod_tau_s < 45)
			return true;
		else if (alt_ft >= 20000 && mod_tau_s < 48)
			return true;
		else
			return false;
	}
}

bool Decider::tau_passes_RA_threshold(double alt_ft, double mod_tau_s, double vert_tau_s, double v_sep_ft)
{
	if (v_sep_ft > get_ra_zthr_ft(alt_ft)) {
		if (alt_ft < 1000)
			return false;
		else if (alt_ft < 2350 && mod_tau_s < 15 && vert_tau_s < 15)
			return true;
		else if (alt_ft < 5000 && mod_tau_s < 20 && vert_tau_s < 20)
			return true;
		else if (alt_ft < 10000 && mod_tau_s < 25 && vert_tau_s < 25)
			return true;
		else if (alt_ft < 20000 && mod_tau_s < 30 && vert_tau_s < 30)
			return true;
		else if (alt_ft >= 20000 && mod_tau_s < 35 && vert_tau_s < 35)
			return true;
		else
			return false;
	} else {
		if (alt_ft < 1000)
			return false;
		else if (alt_ft < 2350 && mod_tau_s < 15)
			return true;
		else if (alt_ft < 5000 && mod_tau_s < 20)
			return true;
		else if (alt_ft < 10000 && mod_tau_s < 25)
			return true;
		else if (alt_ft < 20000 && mod_tau_s < 30)
			return true;
		else if (alt_ft >= 20000 && mod_tau_s < 35)
			return true;
		else
			return false;
	}
}

Sense Decider::DetermineResolutionSense(double user_alt_ft, double intr_alt_ft) {
	if (user_alt_ft > intr_alt_ft)
		return Sense::UPWARD;
	else
		return Sense::DOWNWARD;
}

int Decider::get_alim_ft(double alt_ft) {
	if (alt_ft < 1000)
		return -1;
	else if (alt_ft < 20000)
		return 600;
	else if (alt_ft < 42000)
		return 700;
	else
		return 800;
}

int Decider::get_ra_zthr_ft(double alt_ft) {
	if (alt_ft < 1000)
		return -1;
	else if (alt_ft < 5000)
		return 300;
	else if (alt_ft < 10000)
		return 350;
	else if (alt_ft < 20000)
		return 400;
	else if (alt_ft < 42000)
		return 600;
	else
		return 700;
}

int Decider::get_ta_zthr_ft(double alt_ft) {
	if (alt_ft < 42000)
		return 850;
	else
		return 1200;
}

double Decider::get_ra_dmod_nmi(double alt_ft) {
	if (alt_ft < 1000)
		return 0;
	else if (alt_ft < 2350)
		return .2;
	else if (alt_ft < 5000)
		return .35;
	else if (alt_ft < 10000)
		return .55;
	else if (alt_ft < 20000)
		return .8;
	else
		return 1.1;
}

double Decider::get_ta_dmod_nmi(double alt_ft) {
	if (alt_ft < 1000)
		return .3;
	else if (alt_ft < 2350)
		return .33;
	else if (alt_ft < 5000)
		return .48;
	else if (alt_ft < 10000)
		return .75;
	else if (alt_ft < 20000)
		return 1.0;
	else
		return 1.3;
}

double Decider::get_mod_tau_s(double range_nmi, double closure_rate_knots, double dmod_nmi) {
	return ((pow(range_nmi, 2) - pow(dmod_nmi, 2)) / (range_nmi * closure_rate_knots)) * 3600;
}

RecommendationRangePair Decider::get_rec_range_pair(Sense sense, double user_vvel_ft_m, double intr_vvel_ft_m, double user_alt_ft,
	double intr_alt_ft, double range_tau_s) {

	RecommendationRange positive, negative;

	if (sense != Sense::UNKNOWN && range_tau_s > 0.0) {
		double alim_ft = get_alim_ft(user_alt_ft);
		double intr_projected_altitude_at_cpa = intr_alt_ft + intr_vvel_ft_m * (range_tau_s / 60.0);
		double user_projected_altitude_at_cpa = user_alt_ft + user_vvel_ft_m * (range_tau_s / 60.0);
		double vsep_at_cpa_ft = abs(intr_projected_altitude_at_cpa - user_projected_altitude_at_cpa);

		if (vsep_at_cpa_ft < alim_ft) {
			// Corrective RA
			Velocity absolute_min_vvel_to_achieve_alim = Velocity(get_vvel_for_alim(sense, user_alt_ft, vsep_at_cpa_ft, intr_projected_altitude_at_cpa, range_tau_s), Velocity::VelocityUnits::FEET_PER_MIN);
			char toPrint[100];
			sprintf(toPrint, "result f/m = %f\n", absolute_min_vvel_to_achieve_alim.to_feet_per_min());
			XPLMDebugString(toPrint);

			if (sense == Sense::UPWARD) {
				// upward
				positive.max_vertical_speed = Velocity(absolute_min_vvel_to_achieve_alim.to_feet_per_min() + 500, Velocity::VelocityUnits::FEET_PER_MIN);
				positive.min_vertical_speed = absolute_min_vvel_to_achieve_alim;
				negative.max_vertical_speed = absolute_min_vvel_to_achieve_alim;
				negative.min_vertical_speed = kMinGaugeVerticalVelocity;
			} else {
				// downward
				negative.max_vertical_speed = kMaxGaugeVerticalVelocity;
				negative.min_vertical_speed = absolute_min_vvel_to_achieve_alim;
				positive.max_vertical_speed = absolute_min_vvel_to_achieve_alim;
				positive.min_vertical_speed = Velocity(absolute_min_vvel_to_achieve_alim.to_feet_per_min() - 500, Velocity::VelocityUnits::FEET_PER_MIN);
			}
		} else {
			// Preventative RA
			if (sense == Sense::UPWARD) {
				// upward
				positive.max_vertical_speed = Velocity(user_vvel_ft_m + 500, Velocity::VelocityUnits::FEET_PER_MIN);
				positive.min_vertical_speed = Velocity(user_vvel_ft_m, Velocity::VelocityUnits::FEET_PER_MIN);
				negative.max_vertical_speed = Velocity(user_vvel_ft_m, Velocity::VelocityUnits::FEET_PER_MIN);
				negative.min_vertical_speed = kMinGaugeVerticalVelocity;
			} else {
				// downward
				negative.max_vertical_speed = kMaxGaugeVerticalVelocity;
				negative.min_vertical_speed = Velocity(user_vvel_ft_m, Velocity::VelocityUnits::FEET_PER_MIN);
				positive.max_vertical_speed = Velocity(user_vvel_ft_m, Velocity::VelocityUnits::FEET_PER_MIN);
				positive.min_vertical_speed = Velocity(user_vvel_ft_m - 500, Velocity::VelocityUnits::FEET_PER_MIN);
			}
		}
		positive.valid = true;
		negative.valid = true;
	} else {
		positive.valid = false;
		negative.valid = false;
	}

	return RecommendationRangePair{ positive, negative };
}

double Decider::get_vvel_for_alim(Sense sense, double alt_ft, double vsep_at_cpa_ft, double intr_proj_alt_ft, double range_tau_s) {
	// INSERT PRINT TO LOG HERE
	char* senseAsString = senseToString(sense);
	char toPrint[1000];
	sprintf(toPrint, "Sense = %s, alt_ft = %f, vsep_at_cpa_ft = %f, intr_proj_alt_ft = %f, range_tau = %f\n", senseAsString, alt_ft, vsep_at_cpa_ft, intr_proj_alt_ft, range_tau_s);
	XPLMDebugString(toPrint);

	double toReturn;
	if (sense == Sense::UPWARD) {
		toReturn = (get_alim_ft(alt_ft) - vsep_at_cpa_ft) / (range_tau_s / 60);
		if (toReturn > kMaxGaugeVerticalVelocity.to_feet_per_min()- 500)
			toReturn = kMaxGaugeVerticalVelocity.to_feet_per_min() - 500;
	} else if (sense == Sense::DOWNWARD) {
		toReturn = -(get_alim_ft(alt_ft) - vsep_at_cpa_ft) / (range_tau_s / 60);
		if (toReturn < kMinGaugeVerticalVelocity.to_feet_per_min() + 500)
			toReturn = kMinGaugeVerticalVelocity.to_feet_per_min() + 500;
	} else
		toReturn = 0;

	return toReturn;
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
		Distance ALIM = Distance(get_alim_ft(user_altitude.to_feet()), Distance::DistanceUnits::FEET);

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