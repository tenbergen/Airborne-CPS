#include <cmath>

class Vector2 {
public:
	double x, y;

	Vector2(double xin, double yin) {
		x = xin; y = yin;
	}

	double dotProduct(Vector2 v2) {
		return (x * v2.x) + (y * v2.y);
	}
	double normalize() {
		return std::sqrt(dotProduct(*this));
	}

	Vector2 operator - (Vector2 const & v) const {
		return Vector2(x - v.x, y - v.y);
	}

	Vector2 scalarMult(double const & d) const {
		return Vector2(d * x, d * y);
	}

	Vector2 operator + (Vector2 const & v) const {
		return Vector2(x + v.x, y + v.y);
	}

	double magnitude() const {
		return sqrt(x * x + y * y);
	}

	Vector2 rightPerpendicular() {
		return Vector2(y, -1 * x);
	}

};

int sensitivityLevel;

int tau() {
	switch (sensitivityLevel) {
	case 3: return 15;
	case 4: return 20;
	case 5: return 25;
	case 6: return 30;
	case 7: return 35;
	}
}

int alim() {
	switch (sensitivityLevel) {
	case 3: return 300;
	case 4: return 300;
	case 5: return 350;
	case 6: return 400;
	case 7: return 600;
	}
}

double dmod() {
	switch (sensitivityLevel) {
	case 3: return 0.20;
	case 4: return 0.35;
	case 5: return 0.55;
	case 6: return 0.80;
	case 7: return 1.10;
	}
}

double hmd() {
	switch (sensitivityLevel) {
	case 3: return 0.4;
	case 4: return 0.57;
	case 5: return 0.74;
	case 6: return 0.82;
	case 7: return 0.98;
	}
}

double zthr() {
	switch (sensitivityLevel) {
	case 3: return 600;
	case 4: return 600;
	case 5: return 600;
	case 6: return 600;
	case 7: return 700;
	}
}

/*
	"Assuming constant velocities, the horizontal positions of the ownership and intruder
	aircraft at a time t >= 0 are given by this function." - so (Formula 3)
*/
Vector2 horizontalPositionAtT(Vector2 currentHorizontalComponentOfPosition, Vector2 currentHorizontalVelocity, double deltaTime) {
	return currentHorizontalComponentOfPosition + currentHorizontalVelocity.scalarMult(deltaTime);
}

/*
	"The relative horizontal position of the ownship with respect to the traffic aircraft at
	any time t can be defined as follows." - s (Formula 5)
*/
Vector2 relativeHorizontalPositionAtT(Vector2 currentUserHorizontalPosition, Vector2 currentUserHorizontalVelocity, double deltaTime) {
	return currentUserHorizontalPosition + currentUserHorizontalVelocity.scalarMult(deltaTime);
}

/*
	"The range between the aircraft at any time t is given by:" - r (Formula 6)
*/
double range(Vector2 currentUserHorizontalPosition, Vector2 currentUserHorizontalVelocity, double deltaTime) {
	return std::sqrt(std::pow(currentUserHorizontalPosition.normalize(), 2) + 
		currentUserHorizontalPosition.scalarMult(2 * deltaTime).dotProduct(currentUserHorizontalVelocity) + 
		(std::pow(deltaTime, 2) * std::pow(currentUserHorizontalVelocity.normalize(), 2)));
}

// NOTHING ABOVE THIS MATTERS

/*
	"Given a relative position s and velocity v, the time of horizontal closest point of approach, denoted tcpa, is
	the time t that satisfies closureRate(t) = 0." - tcpa (Formula 8)
*/
double tCpa(Vector2 s, Vector2 v) {
	return -1 * (s.dotProduct(v) / std::pow(v.normalize(), 2));
}

/*
	"Given a relative position s and velocity v, Formula (1) and Formula (2) can be written in a vector form
	as follows...In the vector form of these formulas, i.e., Formula (9) and Formula (10), the singularity is 
	equivalently expressed as s · v = 0. The dot product s · v also characterizes whether the aircraft are 
	horizontally diverging, i.e., s · v > 0, or horizontally converging, i.e., s · v < 0. It can be seen from 
	Formula (9) that when the aircraft are converging, t is positive." - t (Formula 9)
*/
double t(Vector2 s, Vector2 v) {
	return -1 * (std::pow(s.normalize(), 2) / s.dotProduct(v));
}

/*
	"Used in a context where the aircraft are horizontally converging. In that case, modified tau,
	i.e., τmod, is positive when the current range is greater than DMOD." - tmod (Formula 10)
*/
double tMod(Vector2 s, Vector2 v) {
	return (((std::pow(dmod(), 2)) - (s.normalize() * s.normalize())) / (s.dotProduct(v)));
}

/*
	"The function Horizontal_RA(subscript Current ownership's sensitivity level) takes as
	parameters the relative horizontal position s and velocity v of the aircraft. It returns
	true if, for the given input and sensitivity level the horizontal thresholds are
	satisfied." - Horizontal_RA (Formula 11)
*/
bool horizontalRA(Vector2 s, Vector2 v) {
	if (s.dotProduct(v) >= 0)
		return s.normalize() < dmod();
	else
		return tMod(s, v) <= tau();
}

/*
	"In order to model the vertical check performed by the TCAS II RA logic, it is necessary to define the
	time to co-altitude tcoa. This time satisfies sz +tcoavz = 0, where sz ≡ soz −siz and vz ≡ voz −viz. Therefore,
	for a given relative altitude sz and relative non-zero vertical speed vz:" - tcoa (Formula 12)
*/
double tCoa(double sz, double vz) {
	return -1 * (sz/vz);
}

/*
	"The function Vertical RA, which returns true when the vertical thresholds are satisfied for a sensitivity level,
	is defined as follows." - Vertical_RA (Formula 13)
*/
bool verticalRA(double sz, double vz) {
	if (sz * vz >= 0) {
		return std::abs(sz) < zthr();
	} else
		return tCoa(sz, vz) <= tau();
}

/*
	delta (Formula 15)
*/
double delta(Vector2 s, Vector2 v, double d) {
	return (std::pow(d, 2) * std::pow(v.normalize(), 2)) - s.dotProduct(v.rightPerpendicular());
}

/*
	"CD2D is a function that takes as parameters the relative state of the aircraft, i.e., relative position s and
	relative velocity v, and a minimum separation distance D. It returns a Boolean value that indicates whether
	the aircraft will be within horizontal distance D of one another at any time in the future along trajectories
	which are linear projections of their current states." - CD2D (Formula 14)
*/
bool cd2d(Vector2 s, Vector2 v, double d) {
	if (s.normalize() < d)
		return true;
	else
		return delta(s, v, d) > 0 && s.dotProduct(v) < 0;
}

/*
	"The function that determines whether or not an RA will be issued for the ownship can be defined as follows."
	- TCASII_RA (Formula 16)
*/
bool tcasIIRa(Vector2 so, double soz, Vector2 vo, double voz, Vector2 si, double siz, Vector2 vi, double viz) {
	Vector2 s = so - si;
	Vector2 v = vo - vi;
	double sz = soz - siz;
	double vz = voz - viz;
	if (!horizontalRA(s, v))
		return false;
	else if (!verticalRA(sz, vz))
		return false;
	else
		return cd2d(s, v, hmd());
}

/*
	"The function that checks if an RA will be issued for the ownship at a given future time
	t < tcpa(s, v) can be defined as follows." - TCASII_RA_at (Formula 17)
*/
bool tcasIIRaAt(Vector2 so, double soz, Vector2 vo, double voz, Vector2 si, double siz, Vector2 vi, double viz, double t) {
	Vector2 s = so - si;
	Vector2 v = vo - vi;
	double sz = soz - siz;
	double vz = voz - viz;
	if (!horizontalRA(s + v.scalarMult(t), v))
		return false;
	else if (!verticalRA(sz + (t*vz), vz))
		return false;
	else
		return cd2d(s + v.scalarMult(t), v, hmd());
}

/*
	"This function computes exactly the time t in [B, T] at which tmod(s + tv, v) attains its minimum value."
	- Time_Min_TAUmod (Formula 18)
*/
double timeMinTauMod(Vector2 s, Vector2 v, double b, double t) {
	if ((s + v.scalarMult(b)).dotProduct(v) >= 0) {
		return b;
	} else if (delta(s, v, dmod()) < 0) {
		double tmin = 2 * ((std::sqrt(-1 * delta(s, v, dmod())) / (std::pow(v.normalize(), 2)))); // (Formula 19)
		double min = t < (tCpa(s, v) - (tmin/2)) ? tmin : (tCpa(s, v) - (tmin / 2)); // next two lines (Formula 20)
		return b > min ? b : min;
	} else if ((s + v.scalarMult(t)).dotProduct(v) < 0) {
		return t;
	} else {
		double min = t < tCpa(s, v) ? 0 : tCpa(s, v); // next two lines (Formula 20)
		return b > min ? b : min;
	}
}

/*
	"For a given sensitivity level, the function RA2D characterizes the set of possible relative states
	that lead to a horizontal RA within a lookahead time interval." - RA2D (Formula 21)
*/
bool ra2d(Vector2 horizontalRelativePos, Vector2 horizontalRelativeVel, double lookaheadTimeStart, double lookaheadTimeEnd) {
	if (delta(horizontalRelativePos, horizontalRelativeVel, dmod()) >= 0 && (horizontalRelativePos + horizontalRelativeVel.scalarMult(lookaheadTimeStart)).magnitude() < 0 && 
		(horizontalRelativePos + horizontalRelativeVel.scalarMult(lookaheadTimeEnd)).magnitude() >= 0) return true;
	double t2 = timeMinTauMod(horizontalRelativePos, horizontalRelativeVel, lookaheadTimeStart, lookaheadTimeEnd);
	return horizontalRA(horizontalRelativePos + horizontalRelativeVel.scalarMult(t2), horizontalRelativeVel);
}

/*
	"A function can be defined that analytically computes the specific time interval, before a 
	specified lookahead time T, where all vertical RAs will occur. This function is defined as follows."
	- RATimeInterval (Formula 22)
*/
double* raTimeInterval(double sz, double vz, double lookaheadTime) {
	double* returnVal = new double[2];
	if (vz == 0) {
		returnVal[0] = 0; returnVal[1] = lookaheadTime;
	} else {
		double h = zthr() > tau() * std::abs(vz) ? zthr() : tau() * std::abs(vz);
		returnVal[0] = (-1 * std::signbit(vz) * h) / vz;
		returnVal[1] = (std::signbit(vz) * h) / vz;
	}
	return returnVal;
}

/*
	"The function RA3D detects RAs by checking the time interval that is returned by RATimeInterval for
	horizontal RAs using the algorithm RA2D" - RA3D (Formula 23)
*/
bool ra3d(Vector2 userHorizontalPos, double userAlt, Vector2 userHorizontalVel, double userVSpeed, Vector2 intrHorizontalPos, double intrAlt, Vector2 intrHorizontalVel, 
	double intrVSpeed, double lookaheadTime) {
	Vector2 s = userHorizontalPos - intrHorizontalPos;
	Vector2 v = userHorizontalVel - intrHorizontalVel;
	double sz = userAlt - intrAlt;
	double vz = userVSpeed - intrVSpeed;
	if (!cd2d(s, v, hmd())) return false;
	if (vz == 0 && std::abs(sz) > zthr()) return false;
	double* tInOut = raTimeInterval(sz, vz, lookaheadTime);
	double tIn = tInOut[0]; double tOut = tInOut[1]; delete(tInOut);
	if (tIn < 0 || tOut > lookaheadTime) return false;
	return ra2d(s, v, tIn > 0 ? tIn : 0, lookaheadTime < tOut ? lookaheadTime : tOut);
}

/*
	"The function sep_at, defined by Formula (24), predicts the vertical separation between the aircraft at a
	given time t assuming a target vertical speed v for the ownship. The ownship is assumed to fly at constant
	ground speed and constant vertical acceleration a. Once the target vertical speed v is reached the ownship
	continues to fly at constant vertical speed." - sep_at (Formula 24)
*/
double sepAt(double userAlt, double userVSpeed, double intrAlt, double intrVSpeed, double targetVSpeed, double userVAccel, int dir, double deltaTime) {
	double o = ownAltAt(userAlt, userVSpeed, std::abs(targetVSpeed), userVAccel, dir*std::signbit(targetVSpeed), deltaTime);
	double i = intrAlt + (deltaTime * intrVSpeed);
	return (dir * (o - i));
}

/*
	"The function own alt at, defined by Formula (25), computes the
	vertical altitude of the ownship at time t given a target vertical speed v and acceleration a. The parameter 
	dir specifies a possible direction for the vertical ownship maneuver, which is upward when = 1 and downward
	when = −1. The intruder is assumed to continue its trajectory at its current vertical speed."
	- own_alt_at (Formula 25)
*/
double ownAltAt(double userAlt, double userVSpeed, double targetVSpeed, double userVAccel, int dir, double deltaTime) {
	double s = stopAccel(userVSpeed, targetVSpeed, userVAccel, dir, deltaTime);
	double q = (deltaTime < s) ? deltaTime : s;
	double l = (deltaTime - s) > 0 ? (deltaTime - s) : 0;
	return (dir * std::pow(q,2) * (userVAccel / 2) + (q * userVSpeed) + userAlt + (dir * l * targetVSpeed));
}

/*
	"The function stop accel computes the time at which the ownship reaches the target vertical speed v."
	- stop_accel (Formula 26)
*/
double stopAccel(double userVSpeed, double targetVSpeed, double userVAccel, int direction, double deltaTime) {
	if (deltaTime <= 0 || (direction * userVSpeed) >= targetVSpeed)
		return 0;
	else
		return ((direction*targetVSpeed) - userVSpeed) / (direction * userVAccel);
}

/*
	"The sense of an RA is computed based on the direction for the ownship maneuver that provides a greater
	vertical separation, with a bias towards the non-crossing direction. The function RA sense computes such a
	direction, where ALIM is the altitude limit for a given sensitivity level." - RA_sense (Formula 27)
*/
int raSense(double userAlt, double userVSpeed, double intrAlt, double intrVSpeed, double targetVSpeed, double userVAccel, double deltaTime) {
	double oUp = ownAltAt(userAlt, userVSpeed, targetVSpeed, userVAccel, 1, deltaTime);
	double oDown = ownAltAt(userAlt, userVSpeed, targetVSpeed, userVAccel, -1, deltaTime);
	double i = intrAlt + (deltaTime * intrVSpeed);
	double u = oUp - i;
	double d = i - oDown;
	if (userAlt - intrAlt > 0 && u >= alim())
		return 1;
	else if (userAlt - intrAlt < 0 && d >= alim())
		return -1;
	else if (u >= d)
		return 0; // Not sure is correct, blank in NASA doc
	else
		return -1;
}

/*
	"An RA is corrective if the altitude limit is not cleared when the ownship maneuvers in the direction of the RA 
	sense. The Boolean function corrective specifies an algorithm that returns true when a given RA is corrective."
	- corrective (Formula 28)
*/
bool corrective(Vector2 userHorizontalPos, double userAlt, Vector2 userHorizontalVel, double userVSpeed, Vector2 intrHorizontalPos, double intrAlt, Vector2 intrHorizontalVel, double intrVSpeed, double targetVSpeed, double userVAccel) {
	Vector2 s = userHorizontalPos - intrHorizontalPos;
	Vector2 v = userHorizontalVel - intrHorizontalVel;
	double sz = userAlt - intrAlt;
	double vz = userVSpeed - intrVSpeed;
	double t = tMod(s, v);
	int dir = raSense(userAlt, userVSpeed, intrAlt, intrVSpeed, targetVSpeed, userVAccel, t);
	return (s.normalize() < dmod() || ((s.dotProduct(v) < 0) && (dir * (sz + (t * vz)) < alim())));
}