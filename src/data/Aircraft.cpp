#include "Aircraft.h"

Aircraft::Aircraft(std::string const id, std::string const ip) : id(id), ip(ip), heading(Angle::ZERO), 
	verticalVelocity(Velocity::ZERO), trueAirspeed(Velocity::ZERO), theta(Angle::ZERO), phi(Angle::ZERO) {}

Aircraft::Aircraft(std::string const id, std::string const ip, LLA position, Angle heading, Velocity vertVel, Angle theta, Angle phi) : 
	id(id), ip(ip), positionCurrent(position), positionOld(position), heading(heading), verticalVelocity(vertVel), 
	trueAirspeed(Velocity::ZERO), theta(theta), phi(phi) {}

Aircraft::Aircraft(Aircraft const & that) : id(that.id), ip(that.ip), positionCurrentTime(that.positionCurrentTime), 
positionCurrent(that.positionCurrent), positionOldTime(that.positionOldTime), positionOld(that.positionOld),
verticalVelocity(that.verticalVelocity), theta(that.theta), phi(that.phi),trueAirspeed(that.trueAirspeed),
heading(that.heading), threatClassification(that.threatClassification)
{}