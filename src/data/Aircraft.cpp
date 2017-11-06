#include "Aircraft.h"

Aircraft::Aircraft(std::string const id, std::string const ip) : id_(id), ip_(ip), heading(Angle::ZERO), 
	verticalVelocity(Velocity::ZERO), true_airspeed_(Velocity::ZERO) {}

Aircraft::Aircraft(std::string const id, std::string const ip, LLA position, Angle heading, Velocity vert_vel) : 
	id_(id), ip_(ip), positionCurrent(position), positionOld(position), heading(heading), verticalVelocity(vert_vel), 
	true_airspeed_(Velocity::ZERO) {}

Aircraft::Aircraft(Aircraft const & that) : id_(that.id_), ip_(that.ip_), positionCurrentTime(that.positionCurrentTime), 
positionCurrent(that.positionCurrent), positionOldTime(that.positionOldTime), positionOld(that.positionOld),
verticalVelocity(that.verticalVelocity), true_airspeed_(that.true_airspeed_), heading(that.heading), threatClassification(that.threatClassification)
{}