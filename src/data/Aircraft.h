#pragma once

#include <chrono>
#include <string>
#include <mutex>

#include "units/Velocity.h"
#include "units/LLA.h"
#include "units/Vec2.h"

// @author nstemmle
class Aircraft
{
public:
	/* From the FAA Advanced Avionics Handbook (p. 97)
	Non-threat traffic - outside of protected distance and altitude range
	Proximity intruder traffic - Within protected distance and altitude range, but not considered a threat
	Traffic Advisory - Within protected range and considered a threat (User should be notified of existence)
	Resolution Advisory - Within protected range and considered an immediate threat. Issue avoidance recommendation.*/
	enum class ThreatClassification { NON_THREAT_TRAFFIC, PROXIMITY_INTRUDER_TRAFFIC, TRAFFIC_ADVISORY, RESOLUTION_ADVISORY };

	Aircraft(Aircraft const & that);
	Aircraft(std::string const id, std::string const ip);
	Aircraft(std::string const id, std::string const ip, LLA position, Angle heading, Velocity verticalVelocity, Angle theta, Angle phi);

	// The ID of each aircraft is the MAC Address of the network adapter of the machine for the x-plane instance that runs that aircraft
	std::string const id;
	// The IPv4 address that should be used for communicating with the aircraft
	std::string const ip;

	// Aircraft must be locked before getting or setting any non-const values. Copies should be made of any relevant data and no pointers
	// to fields should be used.
	std::mutex lock;

	// Values that are updated via datarefs from xplane
	std::chrono::milliseconds positionCurrentTime;
	LLA positionCurrent;

	std::chrono::milliseconds positionOldTime;
	LLA positionOld;

	// The rate of change of the altitude
	Velocity verticalVelocity;
	/* The true airspeed of the aircraft relative to the air mass around the craft
	This is only populated for the user's aircraft and is currently not used but might need to be used in decider calculations
	since the user's aircraft position is updated every time the plugins render method is called, which means the elapsed time
	will be extremely small.
	*/
	Velocity trueAirspeed;
	Angle heading;

	/*
	* Note that here, theta refers to the pitch of the aircraft, or the inclination above a horizontal axis parallel
	* to sea level. The roll of the aircraft is denoted by phi.
	* */
	Angle theta;
	Angle phi;

	ThreatClassification threatClassification = Aircraft::ThreatClassification::NON_THREAT_TRAFFIC;

};