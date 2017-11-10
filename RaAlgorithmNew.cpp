#include "src\data\Sense.h"
#include "component\Decider.h"

/*
	"Assuming constant velocities, the horizontal positions of the ownership and intruder
	aircraft at a time t >= 0 are given by this function."
	Function returns the aircraft's horizontal position component after deltaTime has elapsed. 
*/
double horizontalPositionAtT(double currentHorizontalComponentOfPosition, double deltaTime, double currentHorizontalVelocity) {
	return (currentHorizontalComponentOfPosition + (deltaTime * currentHorizontalVelocity));
}

/*
	"The relative horizontal position of the ownship with respect to the traffic aircraft at
	any time t can be defined as follows."
	Function returns the aircraft's relative horizontal position with respect to the traffic
	aircraft after deltaTime has elapsed.
*/
double relativeHorizontalPositionAtT(double currentUserHorizontalPosition, double currentTrafficHorizontalPosition,
	double currentUserHorizontalVelocity, double currentTrafficHorizontalVelocity, double deltaTime) {
	return ((currentUserHorizontalPosition-currentTrafficHorizontalPosition) + (deltaTime * (currentUserHorizontalVelocity - currentTrafficHorizontalVelocity)));
}

/*
	"The range between the aircraft at any time t is given by:"
*/
double range(double currentUserHorizontalPosition, double currentTrafficHorizontalPosition,
	double currentUserHorizontalVelocity, double currentTrafficHorizontalVelocity, double deltaTime);



/*
	"The function Horizontal_RA(subscript Current ownership's sensitivity level) takes as
	parameters the relative horizontal position s and velocity v of the aircraft. It returns
	true if, for the given input and sensitivity level the horizontal thresholds are
	satisfied.
*/
bool horizontalRA(double horizontalPosition, double horizontalVelocity);


/*
	"The function stop accel computes the time at which the ownship reaches the target vertical speed v."
*/
double stopAccel(double userVSpeed, double targetVSpeed, double targetAltitude, int direction, double deltaTime) {
	if (deltaTime <= 0 || (direction * userVSpeed) >= targetVSpeed)
		return 0;
	else
		return ((direction*targetVSpeed) - userVSpeed) / (direction * targetAltitude);
}

/*
	"Computes the vertical altitude of the ownship at time t given a target vertical speed v and acceleration a. The parameter (direction) 
	specifies a possible direction for the vertical ownship maneuver, which is upward when = 1 and downward
	when (direction) = −1. The intruder is assumed to continue its trajectory at its current vertical speed."
*/
double getOwnAltitudeAtT(double userAlt, double userVSpeed, double targetVSpeed, double targetAltitude, int direction, double deltaTime) {
	double s = stopAccel(userVSpeed, targetVSpeed, targetAltitude, direction, deltaTime);
	double q = deltaTime > s ? s : deltaTime;
	double l = 0 > (deltaTime - s) ? 0 : (deltaTime - s);
	return (direction * q * q * (targetAltitude / 2)) + (q * userVSpeed) + userAlt + (direction * l * targetVSpeed);
}

/*
	"The sense of an RA is computed based on the direction for the ownship maneuver that provides a greater
	vertical separation, with a bias towards the non-crossing direction. The function RA sense computes such a
	direction, where ALIM is the altitude limit for a given sensitivity level."
*/
Sense getRaSense(double userAlt, double userVSpeed, double intrAlt, double intrVSpeed, double targetVSpeed, double targetAltitude, double deltaTime) {
	double oUp = getOwnAltitudeAtT(userAlt, userVSpeed, targetVSpeed, targetAltitude, 1, deltaTime);
	double oDown = getOwnAltitudeAtT(userAlt, userVSpeed, targetVSpeed, targetAltitude, -1, deltaTime);
	double i = intrAlt + (deltaTime * intrVSpeed);
	double u = oUp - i;
	double d = i - oDown;
	if (userAlt - intrAlt > 0 && u >= Decider::getAlimFt(userAlt))
		return Sense::UPWARD;
	else if (userAlt - intrAlt < 0 && d >= Decider::getAlimFt(userAlt))
		return Sense::DOWNWARD;
	else if (u >= d)
		return Sense::UNKNOWN; // Not sure is correct, blank in NASA doc so maybe NULL?
	else
		return Sense::DOWNWARD;
}