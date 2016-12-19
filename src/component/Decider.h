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

	/* Calculates the slant range, or the distance between two aircraft as measured by a straight line from
	one aircraft to the other.*/
	Distance CalculateSlantRange(Distance horizontalSeparation, Distance verticalSeparation);
	/* Calcluates the slant range rate, or the rate of closure of the aircraft over the slant range. */
	Velocity CalculateSlantRangeRate(Velocity horizontalRate, Velocity verticalRate, double elapsed_time_minutes);
	
	// Converts the supplied milliseconds to minutes
	double ToMinutes(std::chrono::milliseconds time);

	/* Determines the sense (Sense::UPWARDS or Sense::DOWNWARDS) that the user's aircraft should use when
	resolving an RA with the details of the supplied intruding aircraft. Sense::UNKNOWN is returned in the 
	case where the supplied time to collision is not a positive value.*/
	Sense DetermineResolutionSense(Distance user_current_altitude, Distance intr_current_altitude, Velocity user_vvel,
		Velocity intr_vvel, double slant_tau_seconds);

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

	/* Determines the ALIM, or the minimum required vertical separation between two aircraft, based upon the
	altitude of the aircraft.*/
	Distance DetermineALIM(Distance user_altitude) const;
};