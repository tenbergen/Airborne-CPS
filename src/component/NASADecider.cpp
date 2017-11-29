#include "NASADecider.h"

NASADecider::NASADecider(Aircraft* thisAircraft, concurrency::concurrent_unordered_map<std::string, ResolutionConnection*>* connections) {
	thisAircraft_ = thisAircraft; activeConnections_ = connections;
}

NASADecider::~NASADecider() {}

void NASADecider::analyze(Aircraft* intruder) {
	thisAircraft_->lock.lock();
	thisAircraftAltitude_ = thisAircraft_->positionCurrent.altitude.toFeet();
	thisAircraft_->lock.unlock();

	setSensitivityLevel();

	intruder->lock.lock();
	Aircraft intrCopy = *(intruder);
	intruder->lock.unlock();

	ResolutionConnection* connection = (*activeConnections_)[intrCopy.id];

	doCalculations(&intrCopy, connection);

	Aircraft::ThreatClassification threatClass = NASADecider::determineThreatClass(&intrCopy, connection);
	Sense mySense = tempSense_;

	RecommendationRange green, red;

	if (threatClass == Aircraft::ThreatClassification::RESOLUTION_ADVISORY) {
		connection->lock.lock();
		if (connection->currentSense == Sense::UPWARD || connection->currentSense == Sense::DOWNWARD) {
			mySense = connection->currentSense;
		} else if (tempSense_ == Sense::UNKNOWN) {
			tempSense_ = mySense = senseutil::senseFromInt(raSense(connection->userPosition.altitude.toFeet(), 
				calculations_.userVSpeed, intrCopy.positionCurrent.altitude.toFeet(), calculations_.intrVSpeed, 
				calculations_.userVSpeed, calculations_.userVAccel, calculations_.deltaTime)); // CHECK TARGET VERTICAL SPEED PARAMETER!!! Flight plan perhaps????
			connection->sendSense(mySense);
		}
		connection->lock.unlock();

		RecommendationRangePair recRange = getRecRangePair(mySense, calculations_.userVvel, calculations_.intrVvel, calculations_.userPosition.altitude.toFeet(), intrCopy.positionCurrent.altitude.toFeet(), calculations_.modTau);
		green = recRange.positive;
		red = recRange.negative;

	} else if (threatClass == Aircraft::ThreatClassification::NON_THREAT_TRAFFIC) {
		tempSense_ = Sense::UNKNOWN;
		connection->lock.lock();
		connection->currentSense = Sense::UNKNOWN;
		connection->lock.unlock();
		red.valid = false;
		green.valid = false;
	}


	recommendationRangeLock.lock();
	positiveRecommendationRange = green;
	negativeRecommendationRange = red;
	recommendationRangeLock.unlock();

	intruder->lock.lock();
	intruder->threatClassification = threatClass;
	intruder->lock.unlock();
	
}

Aircraft::ThreatClassification NASADecider::determineThreatClass(Aircraft* intrCopy, ResolutionConnection* conn) {
	
	Aircraft::ThreatClassification prevThreatClass = intrCopy->threatClassification;

	bool zthrFlag = calculations_.altSepFt < zthr() ? true : false;

	Aircraft::ThreatClassification newThreatClass;
	// if within proximity range
	if (calculations_.slantRangeNmi < 6 && abs(calculations_.altSepFt) < 1200) {
		// if passes TA threshold
		if (calculations_.closingSpeedKnots > 0
			&& (prevThreatClass >= Aircraft::ThreatClassification::TRAFFIC_ADVISORY
				|| (calculations_.modTau < tau() && zthrFlag))) {
			taMod_ = true;
			zthrFlag = calculations_.altSepFt < zthr() ? true : false;
			// if passes RA threshold
			if (prevThreatClass == Aircraft::ThreatClassification::RESOLUTION_ADVISORY
				|| (calculations_.modTau < tau() && zthrFlag)) {
				newThreatClass = Aircraft::ThreatClassification::RESOLUTION_ADVISORY;
			} else {
				// did not pass RA threshold -- Traffic Advisory
				newThreatClass = Aircraft::ThreatClassification::TRAFFIC_ADVISORY;
			}
		} else {
			// did not pass TA threshold -- just Proximity Traffic
			newThreatClass = Aircraft::ThreatClassification::PROXIMITY_INTRUDER_TRAFFIC;
			taMod_ = false;
		}
	} else {
		// is not within proximity range
		newThreatClass = Aircraft::ThreatClassification::NON_THREAT_TRAFFIC;
	}

	return newThreatClass;
}

RecommendationRangePair NASADecider::getRecRangePair(Sense sense, double userVvelFtPerM, double intrVvelFtPerM, double userAltFt,
	double intrAltFt, double rangeTauS) {

	RecommendationRange positive, negative;

	if (sense != Sense::UNKNOWN && rangeTauS > 0.0) {
		double alimFt = getAlimFt(userAltFt);
		double intrProjectedAltAtCpa = intrAltFt + intrVvelFtPerM * (rangeTauS / 60.0);
		double userProjectedAltAtCpa = userAltFt + userVvelFtPerM * (rangeTauS / 60.0);
		double vsepAtCpaFt = abs(intrProjectedAltAtCpa - userProjectedAltAtCpa);

		// Corrective RA
		Velocity absoluteMinVvelToAchieveAlim = Velocity(getVvelForAlim(sense, userAltFt, vsepAtCpaFt, intrProjectedAltAtCpa, rangeTauS), Velocity::VelocityUnits::FEET_PER_MIN);

		if (sense == Sense::UPWARD) {
			// upward
			positive.maxVerticalSpeed = kMaxGaugeVerticalVelocity_;
			positive.minVerticalSpeed = absoluteMinVvelToAchieveAlim;
			negative.maxVerticalSpeed = absoluteMinVvelToAchieveAlim;
			negative.minVerticalSpeed = kMinGaugeVerticalVelocity_;
		} else {
			// downward
			negative.maxVerticalSpeed = kMaxGaugeVerticalVelocity_;
			negative.minVerticalSpeed = absoluteMinVvelToAchieveAlim;
			positive.maxVerticalSpeed = absoluteMinVvelToAchieveAlim;
			positive.minVerticalSpeed = kMinGaugeVerticalVelocity_;
		}

		positive.valid = true;
		negative.valid = true;
	} else {
		positive.valid = false;
		negative.valid = false;
	}

	return RecommendationRangePair{ positive, negative };
}

void NASADecider::setSensitivityLevel() {
	if (thisAircraftAltitude_ < 1000)
		sensitivityLevel_ = 2;
	else if (thisAircraftAltitude_ >= 1000 && thisAircraftAltitude_ < 2350)
		sensitivityLevel_ = 3;
	else if (thisAircraftAltitude_ >= 2350 && thisAircraftAltitude_ < 5000)
		sensitivityLevel_ = 4;
	else if (thisAircraftAltitude_ >= 5000 && thisAircraftAltitude_ < 10000)
		sensitivityLevel_ = 5;
	else if (thisAircraftAltitude_ >= 10000 && thisAircraftAltitude_ < 20000)
		sensitivityLevel_ = 6;
	else if (thisAircraftAltitude_ >= 20000 && thisAircraftAltitude_ < 42000)
		sensitivityLevel_ = 7;
	else
		sensitivityLevel_ = 0;
}

int NASADecider::tau() {
	if (taMod_)
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
	if (taMod_)
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
	if (taMod_)
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

void NASADecider::doCalculations(Aircraft* intrCopy, ResolutionConnection* conn) {
	conn->lock.lock();
	calculations_.userPosition = conn->userPosition;
	calculations_.userPositionOld = conn->userPositionOld;
	std::chrono::milliseconds userPositionTime = conn->userPositionTime;
	std::chrono::milliseconds userPositionOldTime = conn->userPositionOldTime;
	conn->lock.unlock();

	//get relative pos/vel
	calculations_.userHorPos = getHorPos(calculations_.userPosition);
	calculations_.intrHorPos = getHorPos(intrCopy->positionCurrent);
	calculations_.relativeHorPos = getRelativePos(calculations_.userPosition, intrCopy->positionCurrent);
	calculations_.relativeHorPosOld = getRelativePos(calculations_.userPositionOld, intrCopy->positionOld);
	calculations_.deltaTime = (double)(intrCopy->positionCurrentTime - intrCopy->positionOldTime).count() / 1000;
	calculations_.userVSpeed = std::abs((calculations_.userPosition.altitude.toFeet() - calculations_.userPositionOld.altitude.toFeet()) / calculations_.deltaTime);
	calculations_.intrVSpeed = std::abs((intrCopy->positionCurrent.altitude.toFeet() - intrCopy->positionOld.altitude.toFeet()) / calculations_.deltaTime);
	calculations_.userHorVel = getHorVel(calculations_.userPosition, calculations_.userPositionOld, calculations_.deltaTime);
	calculations_.intrHorVel = getHorVel(intrCopy->positionCurrent, intrCopy->positionOld, calculations_.deltaTime);
	calculations_.relativeVel = getRelativeVel(calculations_.relativeHorPos, calculations_.relativeHorPosOld, calculations_.deltaTime);
	calculations_.modTau = tMod(calculations_.relativeHorPos, calculations_.relativeVel);

	calculations_.slantRangeNmi = abs(calculations_.userPosition.range(&intrCopy->positionCurrent).toNmi());
	calculations_.deltaDistanceM = abs(calculations_.userPositionOld.range(&intrCopy->positionOld).toMeters())
		- abs(calculations_.userPosition.range(&intrCopy->positionCurrent).toMeters());
	calculations_.closingSpeedKnots = Velocity(calculations_.deltaDistanceM / calculations_.deltaTime, Velocity::VelocityUnits::METERS_PER_S).toUnits(Velocity::VelocityUnits::KNOTS);
	calculations_.altSepFt = abs(intrCopy->positionCurrent.altitude.toFeet() - calculations_.userPosition.altitude.toFeet());

	double userDeltaAlt = calculations_.userPosition.altitude.toFeet() - calculations_.userPositionOld.altitude.toFeet();
	double intrDeltaAlt = intrCopy->positionCurrent.altitude.toFeet() - intrCopy->positionOld.altitude.toFeet();
	calculations_.userVvelOld = calculations_.userVvel;
	calculations_.userVvel = userDeltaAlt / calculations_.deltaTime;
	calculations_.intrVvel = intrDeltaAlt / calculations_.deltaTime;
	calculations_.userVAccel = (calculations_.userVvel - calculations_.userVvelOld) / calculations_.deltaTime;

}

double NASADecider::tCpa(Vector2 relativePosition, Vector2 relativeVelocity) {
	return -1 * (relativePosition.dotProduct(relativeVelocity) / std::pow(relativeVelocity.normalize(), 2));
}

double NASADecider::t(Vector2 relativePosition, Vector2 relativeVelocity) {
	return -1 * (std::pow(relativePosition.normalize(), 2) / relativePosition.dotProduct(relativeVelocity));
}

double NASADecider::tMod(Vector2 relativePosition, Vector2 relativeVelocity) {
	return (((std::pow(dmod(), 2)) - (relativePosition.normalize() * relativePosition.normalize())) / (relativePosition.dotProduct(relativeVelocity)));
}

bool NASADecider::horizontalRA(Vector2 relativePosition, Vector2 relativeVelocity) {
	if (relativePosition.dotProduct(relativeVelocity) >= 0)
		return relativePosition.normalize() < dmod();
	else
		return tMod(relativePosition, relativeVelocity) <= tau();
}

double NASADecider::tCoa(double relativeAlt, double relativeVSpeed) {
	return -1 * (relativeAlt / relativeVSpeed);
}

bool NASADecider::verticalRA(double relativeAlt, double relativeVSpeed) {
	if (relativeAlt * relativeVSpeed >= 0) {
		return std::abs(relativeAlt) < zthr();
	} else
		return tCoa(relativeAlt, relativeVSpeed) <= tau();
}

double NASADecider::delta(Vector2 relativePosition, Vector2 relativeVelocity, double minimumSeperationDistance) {
	return (std::pow(minimumSeperationDistance, 2) * std::pow(relativeVelocity.normalize(), 2)) - relativePosition.dotProduct(relativeVelocity.rightPerpendicular());
}

bool NASADecider::cd2d(Vector2 relativePosition, Vector2 relativeVelocity, double minimumSeperationDistance) {
	if (relativePosition.normalize() < minimumSeperationDistance)
		return true;
	else
		return delta(relativePosition, relativeVelocity, minimumSeperationDistance) > 0 && relativePosition.dotProduct(relativeVelocity) < 0;
}

bool NASADecider::tcasIIRa(Vector2 userHorPos, double userAlt, Vector2 userHorVel, double userVSpeed, Vector2 intrHorPos, double intrAlt, Vector2 intrHorVel, double intrVSpeed) {
	Vector2 s = userHorPos - intrHorPos;
	Vector2 v = userHorVel - intrHorVel;
	double sz = userAlt - intrAlt;
	double vz = userVSpeed - intrVSpeed;
	if (!horizontalRA(s, v))
		return false;
	else if (!verticalRA(sz, vz))
		return false;
	else
		return cd2d(s, v, hmd());
}

bool NASADecider::tcasIIRaAt(Vector2 userHorPos, double userAlt, Vector2 userHorVel, double userVSpeed, Vector2 intrHorPos, double intrAlt, Vector2 intrHorVel, double intrVSpeed, double deltaTime) {
	Vector2 s = userHorPos - intrHorPos;
	Vector2 v = userHorVel - intrHorVel;
	double sz = userAlt - intrAlt;
	double vz = userVSpeed - intrVSpeed;
	if (!horizontalRA(s + v.scalarMult(deltaTime), v))
		return false;
	else if (!verticalRA(sz + (deltaTime*vz), vz))
		return false;
	else
		return cd2d(s + v.scalarMult(deltaTime), v, hmd());
}

double NASADecider::timeMinTauMod(Vector2 relativePosition, Vector2 relativeVelocity, double timeBoundStart, double timeBoundEnd) {
	if ((relativePosition + relativeVelocity.scalarMult(timeBoundStart)).dotProduct(relativeVelocity) >= 0) {
		return timeBoundStart;
	} else if (delta(relativePosition, relativeVelocity, dmod()) < 0) {
		double tmin = 2 * ((std::sqrt(-1 * delta(relativePosition, relativeVelocity, dmod())) / (std::pow(relativeVelocity.normalize(), 2)))); // (Formula 19)
		double min = timeBoundEnd < (tCpa(relativePosition, relativeVelocity) - (tmin / 2)) ? tmin : (tCpa(relativePosition, relativeVelocity) - (tmin / 2)); // next two lines (Formula 20)
		return timeBoundStart > min ? timeBoundStart : min;
	} else if ((relativePosition + relativeVelocity.scalarMult(timeBoundEnd)).dotProduct(relativeVelocity) < 0) {
		return timeBoundEnd;
	} else {
		double min = timeBoundEnd < tCpa(relativePosition, relativeVelocity) ? 0 : tCpa(relativePosition, relativeVelocity); // next two lines (Formula 20)
		return timeBoundStart > min ? timeBoundStart : min;
	}
}

bool NASADecider::ra2d(Vector2 horizontalRelativePos, Vector2 horizontalRelativeVel, double lookaheadTimeStart, double lookaheadTimeEnd) {
	if (delta(horizontalRelativePos, horizontalRelativeVel, dmod()) >= 0 && (horizontalRelativePos + horizontalRelativeVel.scalarMult(lookaheadTimeStart)).magnitude() < 0 &&
		(horizontalRelativePos + horizontalRelativeVel.scalarMult(lookaheadTimeEnd)).magnitude() >= 0) return true;
	double t2 = timeMinTauMod(horizontalRelativePos, horizontalRelativeVel, lookaheadTimeStart, lookaheadTimeEnd);
	return horizontalRA(horizontalRelativePos + horizontalRelativeVel.scalarMult(t2), horizontalRelativeVel);
}

double* NASADecider::raTimeInterval(double relativeAlt, double relativeVSpeed, double lookaheadTime) {
	double* returnVal = new double[2];
	if (relativeVSpeed == 0) {
		returnVal[0] = 0; returnVal[1] = lookaheadTime;
	} else {
		double h = zthr() > tau() * std::abs(relativeVSpeed) ? zthr() : tau() * std::abs(relativeVSpeed);
		returnVal[0] = (-1 * std::signbit(relativeVSpeed) * h) / relativeVSpeed;
		returnVal[1] = (std::signbit(relativeVSpeed) * h) / relativeVSpeed;
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