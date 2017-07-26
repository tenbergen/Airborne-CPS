#pragma once

enum class Sense { UPWARD, DOWNWARD, UNKNOWN };

namespace SenseUtil {
	inline Sense SenseFromString(std::string sense_string) {
		if (sense_string == "UPWARD") {
			return Sense::UPWARD;
		} else if (sense_string == "DOWNWARD") {
			return Sense::DOWNWARD;
		} else {
			return Sense::UNKNOWN;
		}
	}

	inline std::string StringFromSense(Sense sense) {
		switch (sense) {
		case Sense::UPWARD: return "UPWARD";
		case Sense::DOWNWARD: return "DOWNWARD";
		default: return "UNKNOWN";
		}
	}

	inline Sense OpositeFromSense(Sense sense) {
		switch (sense) {
		case Sense::UPWARD: return Sense::DOWNWARD;
		case Sense::DOWNWARD: return Sense::UPWARD;
		default: return Sense::UNKNOWN;
		}
	}
}