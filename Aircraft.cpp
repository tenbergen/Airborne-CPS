#include "Aircraft.h"

Aircraft::Aircraft(std::string const id, std::string const ip) : id_(id), ip_(ip), heading_(Angle::ZERO), vertical_velocity_(Velocity::ZERO), true_airspeed_(Velocity::ZERO) {}

Aircraft::Aircraft(std::string const id, std::string const ip, LLA position, Angle heading, Velocity vert_vel) : 
	id_(id), ip_(ip), position_current_(position), position_old_(position), heading_(heading), vertical_velocity_(vert_vel), 
	threat_classification_(ThreatClassification::NON_THREAT_TRAFFIC), true_airspeed_(Velocity::ZERO) {}

Angle Aircraft::VelocityToBearing(Vec2 const * const velocity){
	Vec2 vel_nor = velocity->nor();
	return HeadingToBearing(&vel_nor);
}

Angle Aircraft::HeadingToBearing(Vec2 const * const heading) {
	double angle_deg = Angle::DegreesFromRadians(atan2(heading->y_, heading->x_));

	double north_referenced_angle = Angle::k90Degrees_.to_degrees() - angle_deg;
	
	if (north_referenced_angle < 0.0)
		north_referenced_angle += Angle::k360Degrees_.to_degrees();
	
	return Angle(north_referenced_angle, Angle::AngleUnits::DEGREES);
}