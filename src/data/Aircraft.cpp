#include "Aircraft.h"

Aircraft::Aircraft(std::string const id, std::string const ip) : id_(id), ip_(ip), heading_(Angle::ZERO), 
	vertical_velocity_(Velocity::ZERO), true_airspeed_(Velocity::ZERO) {}

Aircraft::Aircraft(std::string const id, std::string const ip, LLA position, Angle heading, Velocity vert_vel) : 
	id_(id), ip_(ip), position_current_(position), position_old_(position), heading_(heading), vertical_velocity_(vert_vel), 
	true_airspeed_(Velocity::ZERO) {}