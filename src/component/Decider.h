#pragma once

#include <concurrent_unordered_map.h>

#include "component/ResolutionConnection.h"
#include "data/RecommendationRange.h"
#include "data/Aircraft.h"
#include "units/Distance.h"

class Decider {
public:
	Decider() {}
	Decider(Aircraft* thisAircraft, concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>*);
	virtual void analyze(Aircraft* intruder);

	std::mutex recommendationRangeLock;
	RecommendationRange positiveRecommendationRange;
	RecommendationRange negativeRecommendationRange;

	/* Returns a String representation of the supplied ThreatClassification */
	std::string getThreatClassStr(Aircraft::ThreatClassification threatClass);

	/* Determines ALIM for supplied altitude. Returns -1 if below 1000 ft */
	static int getAlimFt(double altFt);

	/* Determines RA DMOD for supplied altitude */
	static double getRADmodNmi(double altFt);

	/* Determines TA DMOD for supplied altitude */
	static double getTADmodNmi(double altFt);

	/* Determines RA ZTHR for supplied altitude. Returns -1 if below 1000 ft */
	static int getRAZthrFt(double altFt);

	/* Determines TA ZTHR for supplied altitude. Returns -1 if below 1000 ft */
	static int getTAZthrFt(double altFt);

	/* Calculates modified tau */
	static double getModTauS(double rangeNmi, double closureRateKnots, double dmodNmi);

protected:
	/* Returns the vertical velocity necessary to achieve ALIM */
	double getVvelForAlim(Sense sense, double altFt, double vsepAtCpaFt, double intrProjAltFt, double rangeTauS);

private:
	static Velocity const kMinGaugeVerticalVelocity_;
	static Velocity const kMaxGaugeVerticalVelocity_;

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

	concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* activeConnections_;
	Aircraft* thisAircraft_;

	/* Temporary storage for the user's aircraft's sense, for use when consensus is not acheived, to prevent Sense flipping */
	Sense tempSense_;

	/* Analyzes the supplied intruder, determining if the intruder is a threat, and begins the process of
	determining actions that will avoid potential collisions. */
	void determineActionRequired(Aircraft* intruder);

	/* Determines the appropriate threat classification*/
	Aircraft::ThreatClassification determineThreatClass(Aircraft* intrCopy, ResolutionConnection* conn);

	/* Returns whether the supplied taus trigger a TA at this altitude*/
	bool tauPassesTAThreshold(double altFt, double modTauS, double vertTauS, double vSepFt);

	/* Returns whether the supplied taus trigger a RA at this altitude*/
	bool tauPassesRAThreshold(double altFt, double modTauS, double vertTauS, double vSepFt);

	/* Determines the sense (Sense::UPWARDS or Sense::DOWNWARDS) that the user's aircraft should use when
	resolving an RA with the details of the supplied intruding aircraft.*/
	Sense determineResolutionSense(double userAltFt, double intrAltFt);

	/* Returns a pair of recommendation ranges as for a Resolution Advisory */
	virtual RecommendationRangePair getRecRangePair(Sense sense, double userVvelFtM, double IntrVvelFtM, double userAltFt,
		double intrAltFt, double rangeTauS);
};