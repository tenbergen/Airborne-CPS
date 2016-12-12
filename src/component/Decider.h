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
	
	static Velocity const kVerticalVelocityClimbDescendDelta_;

	concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* active_connections;
	Aircraft* thisAircraft_;

	double taThreshold = 60.0; //seconds
	double raThreshold = 30.0; //seconds

	void DetermineActionRequired(Aircraft* intruder);

	Distance CalculateSlantRange(Distance horizontalSeparation, Distance verticalSeparation);
	Velocity CalculateSlantRangeRate(Velocity horizontalRate, Velocity verticalRate, double elapsed_time_minutes);
	
	double ToMinutes(std::chrono::milliseconds time);

	Sense DetermineResolutionSense(Distance user_current_altitude, Distance intr_current_altitude, Velocity user_vvel,
		Velocity intr_vvel, double vertical_tau);

	Velocity Decider::DetermineRelativeMinimumVerticalVelocityToAchieveAlim(Distance ALIM, Distance separation_at_cpa, double tau_minutes) const;

	RecommendationRangePair DetermineStrength(Sense sense, Velocity user_vvel, Velocity intr_vvel,
		Distance user_altitude, Distance intr_altitude, double tau_minutes) const;

	Aircraft::ThreatClassification ReevaluateProximinityIntruderThreatClassification(double horizontal_tau, double vertical_tau, 
		Aircraft::ThreatClassification current_threat_class) const;

	Distance DetermineALIM(Distance user_altitude) const;
};