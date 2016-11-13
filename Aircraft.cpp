#include "Aircraft.h"

Aircraft::Aircraft(std::string const id) : id_(id) {}

Angle Aircraft::VelocityToBearing(Vec2 const * const velocity){
	Vec2 vel_nor = velocity->nor();
	return HeadingToBearing(&vel_nor);
}

Angle Aircraft::HeadingToBearing(Vec2 const * const heading) {
	double angle_deg = Angle::DegreesFromRadians(atan2(heading->y_, heading->x_));

	double north_referenced_angle = Angle::k90Degrees_.to_degrees() - angle_deg;
	
	if (north_referenced_angle < 0.0)
		north_referenced_angle += Angle::k360Degrees_.to_degrees();
	
	return Angle(north_referenced_angle, Angle::DEGREES);
}