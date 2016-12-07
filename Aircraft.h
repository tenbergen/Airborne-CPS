#pragma once

#include <chrono>
#include <string>
#include <mutex>

#include "Velocity.h"
#include "LLA.h"
#include "Vec2.h"

class Aircraft
{
public:
	static Angle VelocityToBearing(Vec2 const * const velocity);
	static Angle HeadingToBearing(Vec2 const * const heading);

	/* From the FAA Advanced Avionics Handbook (p. 97)
	Non-threat traffic - outside of protected distance and altitude range
	Proximity intruder traffic - Within protected distance and altitude range, but not considered a threat
	Traffic Advisory - Within protected range and considered a threat (User should be notified of existence)
	Resolution Advisory - Within protected range and considered an immediate threat. Issue avoidance recommendation.
	*/
	enum class ThreatClassification { NON_THREAT_TRAFFIC, PROXIMITY_INTRUDER_TRAFFIC, TRAFFIC_ADVISORY, RESOLUTION_ADVISORY };

	Aircraft(std::string const id, std::string const ip);
	Aircraft(std::string const id, std::string const ip, LLA position, Angle heading, Velocity vertical_velocity);

	std::mutex lock_;

	std::string const id_;
	std::string const ip_;

	/// Values that are updated via datarefs from xplane
	std::chrono::milliseconds position_current_time_;
	LLA position_current_;

	std::chrono::milliseconds position_old_time_;
	LLA position_old_;

	// The rate of change of the altitude
	Velocity vertical_velocity_;
	Velocity true_airspeed_;
	Angle heading_;

	ThreatClassification threat_classification_;
};