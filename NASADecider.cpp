#include "NASADecider.h"

Velocity const Decider::kMinGaugeVerticalVelocity_ = { -4000.0, Velocity::VelocityUnits::FEET_PER_MIN };
Velocity const Decider::kMaxGaugeVerticalVelocity_ = { 4000.0, Velocity::VelocityUnits::FEET_PER_MIN };

Distance const Decider::kProtectionVolumeRadius_ = { 30.0, Distance::DistanceUnits::NMI };

Distance const Decider::kAlim350_ = { 350.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAlim400_ = { 400.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAlim600_ = { 600.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAlim700_ = { 700.0, Distance::DistanceUnits::FEET };

Distance const Decider::kAltitudeAlim350Threshold_ = { 5000.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAltitudeAlim400Threshold_ = { 10000.0, Distance::DistanceUnits::FEET };
Distance const Decider::kAltitudeAlim600Threshold_ = { 20000.0, Distance::DistanceUnits::FEET };

Velocity const Decider::kVerticalVelocityClimbDescendDelta_ = { 1500.0, Velocity::VelocityUnits::FEET_PER_MIN };

NASADecider::NASADecider(Aircraft* thisAircraft, concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* connections) {
	thisAircraft_ = thisAircraft; activeConnections_ = connections;
	thisAircraft_->lock.lock();
}

NASADecider::~NASADecider() {}

void NASADecider::analyze(Aircraft* intruder) {
	thisAircraft_->lock.lock();
	thisAircraftAltitude_ = thisAircraft_->positionCurrent.altitude.toFeet();
	thisAircraft_->lock.unlock();

	setSensitivityLevel(thisAircraftAltitude_);

	intruder->lock.lock();
	Aircraft intrCopy = *(intruder);
	intruder->lock.unlock();

	ResolutionConnection* connection = (*activeConnections_)[intrCopy.id];

	Aircraft::ThreatClassification threatClass = NASADecider::determineThreatClass(&intrCopy, connection);
	Sense mySense = tempSense_;

	RecommendationRange green, red;

	if (threatClass == Aircraft::ThreatClassification::RESOLUTION_ADVISORY) {
		connection->lock.lock();
		if (connection->consensusAchieved && (connection->currentSense == Sense::UPWARD || connection->currentSense == Sense::DOWNWARD)) {
			mySense = connection->currentSense;
		} else if (tempSense_ == Sense::UNKNOWN) {
			tempSense_ = mySense = raSense()
			connection->sendSense(mySense);
		}
		connection->lock.unlock();

		double userDeltaPosM = connection->userPosition.range(&connection->userPositionOld).toMeters();
		double userDeltaAltM = connection->userPosition.altitude.toMeters() - connection->userPositionOld.altitude.toMeters();
		double intrDeltaPosM = intrCopy.positionCurrent.range(&intrCopy.positionOld).toMeters();
		double intrDeltaAltM = intrCopy.positionCurrent.altitude.toMeters() - intrCopy.positionOld.altitude.toMeters();
		double userElapsedTimeS = (double)(connection->userPositionTime - connection->userPositionOldTime).count() / 1000;
		double intrElapsedTimeS = (double)(intrCopy.positionCurrentTime - intrCopy.positionOldTime).count() / 1000;
		double slantRangeNmi = abs(connection->userPosition.range(&intrCopy.positionCurrent).toUnits(Distance::DistanceUnits::NMI));
		double deltaDistanceM = abs(connection->userPositionOld.range(&intrCopy.positionOld).toUnits(Distance::DistanceUnits::METERS))
			- abs(connection->userPosition.range(&intrCopy.positionCurrent).toUnits(Distance::DistanceUnits::METERS));
		double closingSpeedKnots = Velocity(deltaDistanceM / intrElapsedTimeS, Velocity::VelocityUnits::METERS_PER_S).toUnits(Velocity::VelocityUnits::KNOTS);
		Velocity userVvel = Velocity(userDeltaAltM / userElapsedTimeS, Velocity::VelocityUnits::METERS_PER_S);
		Velocity intrVvel = Velocity(intrDeltaAltM / intrElapsedTimeS, Velocity::VelocityUnits::METERS_PER_S);
		double rangeTauS = getModTauS(slantRangeNmi, closingSpeedKnots, getRADmodNmi(connection->userPosition.altitude.toFeet()));
		recRange = getRecRangePair(mySense, userVvel.toFeetPerMin(), intrVvel.toFeetPerMin(), connection->userPosition.altitude.toFeet(), intrCopy.positionCurrent.altitude.toFeet(), rangeTauS);

	} else if (threatClass == Aircraft::ThreatClassification::NON_THREAT_TRAFFIC) {
		tempSense_ = Sense::UNKNOWN;
		connection->lock.lock();
		connection->currentSense = Sense::UNKNOWN;
		connection->lock.unlock();
		recRange.negative.valid = false;
		recRange.positive.valid = false;
	}


	recommendationRangeLock.lock();
	positiveRecommendationRange = recRange.positive;
	negativeRecommendationRange = recRange.negative;
	recommendationRangeLock.unlock();
	
}

Aircraft::ThreatClassification NASADecider::determineThreatClass(Aircraft* intrCopy, ResolutionConnection* conn) {
	conn->lock.lock();
	LLA userPosition = conn->userPosition;
	LLA userPositionOld = conn->userPositionOld;
	std::chrono::milliseconds userPositionTime = conn->userPositionTime;
	std::chrono::milliseconds userPositionOldTime = conn->userPositionOldTime;
	conn->lock.unlock();

	//get relative pos/vel
	Vector2 userHorPos = getHorPos(userPosition);
	Vector2 intrHorPos = getHorPos(intrCopy->positionCurrent);
	Vector2 relativeHorPos = getRelativePos(userPosition, intrCopy->positionCurrent);
	Vector2 relativeHorPosOld = getRelativePos(userPositionOld, intrCopy->positionOld);
	double deltaTime = (double)(intrCopy->positionCurrentTime - intrCopy->positionOldTime).count() / 1000;
	double userVSpeed = std::abs((userPosition.altitude.toFeet() - userPositionOld.altitude.toFeet()) / deltaTime);
	double intrVSpeed = std::abs((intrCopy->positionCurrent.altitude.toFeet() - intrCopy->positionOld.altitude.toFeet()) / deltaTime);
	Vector2 userHorVel = getHorVel(userPosition, userPositionOld, deltaTime);
	Vector2 intrHorVel = getHorVel(intrCopy->positionCurrent, intrCopy->positionOld, deltaTime);
	Vector2 relativeVel = getRelativeVel(relativeHorPos, relativeHorPosOld, deltaTime);
	double modTau = tMod(relativeHorPos, relativeVel);
	

	Aircraft::ThreatClassification prevThreatClass = intrCopy->threatClassification;


	double slantRangeNmi = abs(userPosition.range(&intrCopy->positionCurrent).toNmi());
	double deltaDistanceM = abs(userPositionOld.range(&intrCopy->positionOld).toMeters())
		- abs(userPosition.range(&intrCopy->positionCurrent).toMeters());
	double closingSpeedKnots = Velocity(deltaDistanceM / deltaTime, Velocity::VelocityUnits::METERS_PER_S).toUnits(Velocity::VelocityUnits::KNOTS);
	double altSepFt = abs(intrCopy->positionCurrent.altitude.toFeet() - userPosition.altitude.toFeet());

	bool zthrFlag = altSepFt > zthr() ? true : false;

	Aircraft::ThreatClassification newThreatClass;
	// if within proximity range
	if (slantRangeNmi < 6 && abs(altSepFt) < 1200) {
		// if passes TA threshold
		if (closingSpeedKnots > 0
			&& (prevThreatClass >= Aircraft::ThreatClassification::TRAFFIC_ADVISORY
				|| (modTau < tau() && zthrFlag))) {
			// if passes RA threshold
			if (prevThreatClass == Aircraft::ThreatClassification::RESOLUTION_ADVISORY
				|| tcasIIRa(userHorPos, thisAircraftAltitude_, userHorVel, userVSpeed, intrHorPos, intrCopy->positionCurrent.altitude.toFeet(), intrHorVel, intrVSpeed)) {
				newThreatClass = Aircraft::ThreatClassification::RESOLUTION_ADVISORY;
				raMod_ = true;
			} else {
				// did not pass RA threshold -- Traffic Advisory
				newThreatClass = Aircraft::ThreatClassification::TRAFFIC_ADVISORY;
			}
		} else {
			// did not pass TA threshold -- just Proximity Traffic
			newThreatClass = Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC;
			raMod_ = false;
		}
	} else {
		// is not within proximity range
		newThreatClass = Aircraft::ThreatClassification::NON_THREAT_TRAFFIC;
	}

	return newThreatClass;
}

void NASADecider::setSensitivityLevel(double alt) {
	if (alt < 1000)
		sensitivityLevel_ = 2;
	else if (alt >= 1000 && alt < 2350)
		sensitivityLevel_ = 3;
	else if (alt >= 2350 && alt < 5000)
		sensitivityLevel_ = 4;
	else if (alt >= 5000 && alt < 10000)
		sensitivityLevel_ = 5;
	else if (alt >= 10000 && alt < 20000)
		sensitivityLevel_ = 6;
	else if (alt >= 20000 && alt < 42000)
		sensitivityLevel_ = 7;
	else
		sensitivityLevel_ = 0;
}

int NASADecider::tau() {
	if (raMod_)
		switch (sensitivityLevel_) {
		case 3: return 15;
		case 4: return 20;
		case 5: return 25;
		case 6: return 30;
		case 7: return 35;
		}
	else
		switch (sensitivityLevel_) {
		case 2: return 20;
		case 3: return 25;
		case 4: return 30;
		case 5: return 40;
		case 6: return 45;
		case 7: return 58;
		}
}

int NASADecider::alim() {
	switch (sensitivityLevel_) {
	case 3: return 300;
	case 4: return 300;
	case 5: return 350;
	case 6: return 400;
	case 7: if (thisAircraftAltitude_ < 42000) return 600;
			else return 700;
	}
}

double NASADecider::dmod() {
	if (raMod_)
		switch (sensitivityLevel_) {
		case 3: return 0.20;
		case 4: return 0.35;
		case 5: return 0.55;
		case 6: return 0.80;
		case 7: return 1.10;
		}
	else
		switch (sensitivityLevel_) {
		case 2: return 0.30;
		case 3: return 0.33;
		case 4: return 0.48;
		case 5: return 0.75;
		case 6: return 1.00;
		case 7: return 1.30;
		}
}

double NASADecider::hmd() {
	switch (sensitivityLevel_) {
	case 3: return 0.4;
	case 4: return 0.57;
	case 5: return 0.74;
	case 6: return 0.82;
	case 7: return 0.98;
	}
}

double NASADecider::zthr() {
	if (raMod_)
		switch (sensitivityLevel_) {
		case 3: return 600;
		case 4: return 600;
		case 5: return 600;
		case 6: return 600;
		case 7: if (thisAircraftAltitude_ < 42000) return 700;
				else return 800;
		}
	else
		switch (sensitivityLevel_) {
		case 2: return 850;
		case 3: return 850;
		case 4: return 850;
		case 5: return 850;
		case 6: return 850;
		case 7: if (thisAircraftAltitude_ < 42000) return 850;
				else return 1200;
		}
}

Vector2 NASADecider::getHorPos(LLA position) {
	return Vector2(position.distPerDegreeLat().toFeet(), position.distPerDegreeLon().toFeet());
}

Vector2 NASADecider::getHorVel(LLA position, LLA positionOld, double deltaTime) {
	return Vector2(position.range(&positionOld), position.bearing(&positionOld)).scalarMult(1 / deltaTime);
}

Vector2 NASADecider::getRelativePos(LLA userPos, LLA intrPos) {
	Distance d = userPos.range(&intrPos);
	Angle a = userPos.bearing(&intrPos);
	return Vector2(d, a);
}

Vector2 NASADecider::getRelativeVel(Vector2 relativePos, Vector2 relativePosOld, double deltaTime) {
	return (relativePos - relativePosOld).scalarMult(1 / deltaTime);
}

double NASADecider::tCpa(Vector2 s, Vector2 v) {
	return -1 * (s.dotProduct(v) / std::pow(v.normalize(), 2));
}

double NASADecider::t(Vector2 s, Vector2 v) {
	return -1 * (std::pow(s.normalize(), 2) / s.dotProduct(v));
}

double NASADecider::tMod(Vector2 s, Vector2 v) {
	return (((std::pow(dmod(), 2)) - (s.normalize() * s.normalize())) / (s.dotProduct(v)));
}

bool NASADecider::horizontalRA(Vector2 s, Vector2 v) {
	if (s.dotProduct(v) >= 0)
		return s.normalize() < dmod();
	else
		return tMod(s, v) <= tau();
}

double NASADecider::tCoa(double sz, double vz) {
	return -1 * (sz / vz);
}

bool NASADecider::verticalRA(double sz, double vz) {
	if (sz * vz >= 0) {
		return std::abs(sz) < zthr();
	} else
		return tCoa(sz, vz) <= tau();
}

double NASADecider::delta(Vector2 s, Vector2 v, double d) {
	return (std::pow(d, 2) * std::pow(v.normalize(), 2)) - s.dotProduct(v.rightPerpendicular());
}

bool NASADecider::cd2d(Vector2 s, Vector2 v, double d) {
	if (s.normalize() < d)
		return true;
	else
		return delta(s, v, d) > 0 && s.dotProduct(v) < 0;
}

bool NASADecider::tcasIIRa(Vector2 so, double soz, Vector2 vo, double voz, Vector2 si, double siz, Vector2 vi, double viz) {
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

bool NASADecider::tcasIIRaAt(Vector2 so, double soz, Vector2 vo, double voz, Vector2 si, double siz, Vector2 vi, double viz, double t) {
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

double NASADecider::timeMinTauMod(Vector2 s, Vector2 v, double b, double t) {
	if ((s + v.scalarMult(b)).dotProduct(v) >= 0) {
		return b;
	} else if (delta(s, v, dmod()) < 0) {
		double tmin = 2 * ((std::sqrt(-1 * delta(s, v, dmod())) / (std::pow(v.normalize(), 2)))); // (Formula 19)
		double min = t < (tCpa(s, v) - (tmin / 2)) ? tmin : (tCpa(s, v) - (tmin / 2)); // next two lines (Formula 20)
		return b > min ? b : min;
	} else if ((s + v.scalarMult(t)).dotProduct(v) < 0) {
		return t;
	} else {
		double min = t < tCpa(s, v) ? 0 : tCpa(s, v); // next two lines (Formula 20)
		return b > min ? b : min;
	}
}

bool NASADecider::ra2d(Vector2 horizontalRelativePos, Vector2 horizontalRelativeVel, double lookaheadTimeStart, double lookaheadTimeEnd) {
	if (delta(horizontalRelativePos, horizontalRelativeVel, dmod()) >= 0 && (horizontalRelativePos + horizontalRelativeVel.scalarMult(lookaheadTimeStart)).magnitude() < 0 &&
		(horizontalRelativePos + horizontalRelativeVel.scalarMult(lookaheadTimeEnd)).magnitude() >= 0) return true;
	double t2 = timeMinTauMod(horizontalRelativePos, horizontalRelativeVel, lookaheadTimeStart, lookaheadTimeEnd);
	return horizontalRA(horizontalRelativePos + horizontalRelativeVel.scalarMult(t2), horizontalRelativeVel);
}

double* NASADecider::raTimeInterval(double sz, double vz, double lookaheadTime) {
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

bool NASADecider::ra3d(Vector2 userHorizontalPos, double userAlt, Vector2 userHorizontalVel, double userVSpeed, Vector2 intrHorizontalPos, double intrAlt, Vector2 intrHorizontalVel,
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

double NASADecider::sepAt(double userAlt, double userVSpeed, double intrAlt, double intrVSpeed, double targetVSpeed, double userVAccel, int dir, double deltaTime) {
	double o = ownAltAt(userAlt, userVSpeed, std::abs(targetVSpeed), userVAccel, dir*std::signbit(targetVSpeed), deltaTime);
	double i = intrAlt + (deltaTime * intrVSpeed);
	return (dir * (o - i));
}

double NASADecider::ownAltAt(double userAlt, double userVSpeed, double targetVSpeed, double userVAccel, int dir, double deltaTime) {
	double s = stopAccel(userVSpeed, targetVSpeed, userVAccel, dir, deltaTime);
	double q = (deltaTime < s) ? deltaTime : s;
	double l = (deltaTime - s) > 0 ? (deltaTime - s) : 0;
	return (dir * std::pow(q, 2) * (userVAccel / 2) + (q * userVSpeed) + userAlt + (dir * l * targetVSpeed));
}

double NASADecider::stopAccel(double userVSpeed, double targetVSpeed, double userVAccel, int direction, double deltaTime) {
	if (deltaTime <= 0 || (direction * userVSpeed) >= targetVSpeed)
		return 0;
	else
		return ((direction*targetVSpeed) - userVSpeed) / (direction * userVAccel);
}

int NASADecider::raSense(double userAlt, double userVSpeed, double intrAlt, double intrVSpeed, double targetVSpeed, double userVAccel, double deltaTime) {
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

bool NASADecider::corrective(Vector2 userHorizontalPos, double userAlt, Vector2 userHorizontalVel, double userVSpeed, Vector2 intrHorizontalPos, double intrAlt, Vector2 intrHorizontalVel, double intrVSpeed, double targetVSpeed, double userVAccel) {
	Vector2 s = userHorizontalPos - intrHorizontalPos;
	Vector2 v = userHorizontalVel - intrHorizontalVel;
	double sz = userAlt - intrAlt;
	double vz = userVSpeed - intrVSpeed;
	double t = tMod(s, v);
	int dir = raSense(userAlt, userVSpeed, intrAlt, intrVSpeed, targetVSpeed, userVAccel, t);
	return (s.normalize() < dmod() || ((s.dotProduct(v) < 0) && (dir * (sz + (t * vz)) < alim())));
}