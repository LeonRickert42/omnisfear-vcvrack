#pragma once

namespace omnisfear {

// A single note event within a phrase.
// Positions are measured in clock ticks; sub-tick fractions carry the sub-tick precision.
struct MotiveEvent {
	float startPos;   // [0, PHRASE_TICKS)
	float gateLen;    // ticks
	float pitch;      // V/Oct
	float velocity;   // 0..1
};

} // namespace omnisfear
