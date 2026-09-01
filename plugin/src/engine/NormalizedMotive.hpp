#pragma once
#include <cstdint>

namespace omnisfear {

// Pitch stored as interval (volts) relative to the motive's anchor pitch.
struct NormalizedEvent {
	float startPos;       // ticks [0, PHRASE_TICKS)
	float gateLen;        // ticks
	float pitchInterval;  // volts, relative to anchor
	float velocity;       // 0..1
};

// Motive-shaped-representation of a captured phrase.
// Preserves musical identity (contour, rhythm, density) rather than absolute pitches.
struct NormalizedMotive {
	static constexpr int MAX_EVENTS = 128;
	static constexpr int PHRASE_TICKS = 16;

	NormalizedEvent events[MAX_EVENTS];
	int   count = 0;

	float anchorPitch = 0.f;  // first event's absolute pitch (volts)

	// Derived on normalize().
	float density = 0.f;       // count / PHRASE_TICKS
	int   direction = 0;       // -1 desc, 0 osc, +1 asc
	uint16_t rhythmGrid = 0;   // bit i = hit lands within tick i (floor(startPos))
};

// Reads raw MotiveEvents from a PhraseBuffer-like source and fills the normalized form.
// Template so it works with any container exposing .events[] and .count.
template <typename Buffer>
void normalize(const Buffer& src, NormalizedMotive& out);

} // namespace omnisfear

#include "NormalizedMotive.inl"
