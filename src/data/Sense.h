#pragma once

enum class Sense { UPWARD, DOWNWARD, UNKNOWN };

namespace senseutil {
	inline Sense senseFromString(std::string senseString) {
		if (senseString == "UPWARD") {
			return Sense::UPWARD;
		} else if (senseString == "DOWNWARD") {
			return Sense::DOWNWARD;
		} else {
			return Sense::UNKNOWN;
		}
	}

	inline std::string stringFromSense(Sense sense) {
		switch (sense) {
		case Sense::UPWARD: return "UPWARD";
		case Sense::DOWNWARD: return "DOWNWARD";
		default: return "UNKNOWN";
		}
	}

	inline Sense oppositeFromSense(Sense sense) {
		switch (sense) {
		case Sense::UPWARD: return Sense::DOWNWARD;
		case Sense::DOWNWARD: return Sense::UPWARD;
		default: return Sense::UNKNOWN;
		}
	}

	inline Sense senseFromInt(int s) {
		switch (s) {
		case 1: return Sense::UPWARD;
		case -1: return Sense::DOWNWARD;
		case 0: return Sense::UNKNOWN;
		default: return Sense::UNKNOWN;
		}
	}
}