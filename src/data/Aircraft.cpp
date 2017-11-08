#include "Aircraft.h"

Aircraft::Aircraft(std::string const id, std::string const ip) : id(id), ip(ip), heading(Angle::ZERO), 
	verticalVelocity(Velocity::ZERO), trueAirspeed(Velocity::ZERO) {}

Aircraft::Aircraft(std::string const id, std::string const ip, LLA position, Angle heading, Velocity vertVel) : 
	id(id), ip(ip), positionCurrent(position), positionOld(position), heading(heading), verticalVelocity(vertVel), 
	trueAirspeed(Velocity::ZERO) {}

Aircraft::Aircraft(Aircraft const & that) : id(that.id), ip(that.ip), positionCurrentTime(that.positionCurrentTime), 
positionCurrent(that.positionCurrent), positionOldTime(that.positionOldTime), positionOld(that.positionOld),
verticalVelocity(that.verticalVelocity), trueAirspeed(that.trueAirspeed), heading(that.heading), threatClassification(that.threatClassification)
{}