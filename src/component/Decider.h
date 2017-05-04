#pragma once

#include <concurrent_unordered_map.h>

#include "component/ResolutionConnection.h"
#include "data/RecommendationRange.h"
#include "data/Aircraft.h"
#include "units/Distance.h"

class Decider {
public:
	Decider(Aircraft* thisAircraft, concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>*);
	void Analyze(Aircraft* intruder);

	std::mutex recommendation_range_lock_;
	RecommendationRange positive_recommendation_range_;
	RecommendationRange negative_recommendation_range_;

	/* Determines ALIM for supplied altitude. Returns -1 if below 1000 ft */
	static int get_alim_ft(double alt_ft);

	/* Determines RA DMOD for supplied altitude */
	static double get_ra_dmod_nmi(double alt_ft);

	/* Determines TA DMOD for supplied altitude */
	static double get_ta_dmod_nmi(double alt_ft);

	/* Determines RA ZTHR for supplied altitude. Returns -1 if below 1000 ft */
	static int get_ra_zthr_ft(double alt_ft);

	/* Determines TA ZTHR for supplied altitude. Returns -1 if below 1000 ft */
	static int get_ta_zthr_ft(double alt_ft);

	/* Calculates modified tau */
	static double get_mod_tau_s(double range_nmi, double closure_rate_knots, double dmod_nmi);

private:
	static Velocity const kMinGaugeVerticalVelocity;
	static Velocity const kMaxGaugeVerticalVelocity;

	static Distance const kProtectionVolumeRadius_;

	static Distance const kAlim350_;
	static Distance const kAlim400_;
	static Distance const kAlim600_;
	static Distance const kAlim700_;

	static Distance const kAltitudeAlim350Threshold_;
	static Distance const kAltitudeAlim400Threshold_;
	static Distance const kAltitudeAlim600Threshold_;

	/* The velocity delta that should be added and subtracted from the user aircraft's vertical velocity
	to estimate a climbing or descending trajectory resprectively*/
	static Velocity const kVerticalVelocityClimbDescendDelta_;

	concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* active_connections;
	Aircraft* thisAircraft_;

	double taThresholdSeconds = 60.0; //seconds
	double raThresholdSeconds = 30.0; //seconds

									  /* Analyzes the supplied intruder, determining if the intruder is a threat, and begins the process of
									  determining actions that will avoid potential collisions. */
	void DetermineActionRequired(Aircraft* intruder);

	/* Determines the appropriate threat classification*/
	Aircraft::ThreatClassification DetermineThreatClass(Aircraft* intr_copy, ResolutionConnection* conn);

	/* Returns whether the supplied taus trigger a TA at this altitude*/
	bool tau_passes_TA_threshold(double alt_ft, double mod_tau_s, double vert_tau_s, double v_sep_ft);

	/* Returns whether the supplied taus trigger a RA at this altitude*/
	bool tau_passes_RA_threshold(double alt_ft, double mod_tau_s, double vert_tau_s, double v_sep_ft);

	// Converts the supplied milliseconds to minutes
	double ToMinutes(std::chrono::milliseconds time);

	/* Determines the sense (Sense::UPWARDS or Sense::DOWNWARDS) that the user's aircraft should use when
	resolving an RA with the details of the supplied intruding aircraft.*/
	Sense DetermineResolutionSense(double user_alt_ft, double intr_alt_ft);

	/* Calculates the minimum vertical velocity required to achieve ALIM separation at the CPA relative to the user's current vertical velocity */
	Velocity Decider::DetermineRelativeMinimumVerticalVelocityToAchieveAlim(Distance ALIM, Distance separation_at_cpa, double tau_seconds) const;

	/* Determines the recommended positive and negative (allowed and disallowed) vertical velocity ranges based upon the sense,
	the user's vertical velocity, and the separation at the CPA.*/
	RecommendationRangePair DetermineStrength(Sense sense, Velocity user_vvel, Velocity intr_vvel,
		Distance user_altitude, Distance intr_altitude, double tau_seconds) const;
	RecommendationRangePair DetermineUpwardSenseStrengh(Velocity user_vvel, Distance ALIM, Distance separation_at_cpa, Velocity relative_min_vvel_to_achieve_alim) const;

	/* Reevaluates the supplied proximity intruder based upon the horizontal and vertical tau (time to close
	based upon closure rate) and ensures the threat classification can only be upgraded and not downgraded. */
	Aircraft::ThreatClassification ReevaluateProximinityIntruderThreatClassification(double horizontal_tau_seconds, double vertical_tau_seconds,
		Aircraft::ThreatClassification current_threat_class) const;

	/* Returns a pair of recommendation ranges as for a Resolution Advisory */
	RecommendationRangePair get_rec_range_pair(Sense sense, double user_vvel_ft_m, double intr_vvel_ft_m, double user_alt_ft,
		double intr_alt_ft, double range_tau_s);

	/* Returns the vertical velocity necessary to achieve ALIM */
	double get_vvel_for_alim(Sense sense, double alt_ft, double vsep_at_cpa_ft, double intr_proj_alt_ft, double range_tau_s);
};