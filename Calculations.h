#include "Vector2.h"

class Calculations {
public:
	LLA userPosition;
	LLA userPositionOld;
	Vector2 userHorPos;
	Vector2 intrHorPos;
	Vector2 relativeHorPos;
	Vector2 relativeHorPosOld;
	double deltaTime;
	double userVSpeed;
	double userVAccel;
	double intrVSpeed;
	Vector2 userHorVel;
	Vector2 intrHorVel;
	Vector2 relativeVel;
	double modTau;
	double slantRangeNmi;
	double deltaDistanceM;
	double closingSpeedKnots;
	double altSepFt;
	double userVvel;
	double intrVvel;
	double userVvelOld;

	Calculations() {}
	~Calculations() {}
};