#pragma once
#include "NormalizedMotive.hpp"
#include "MotiveEvent.hpp"

namespace omnisfear {

template <typename Buffer>
inline void normalize(const Buffer& src, NormalizedMotive& out) {
	out.count = 0;
	out.anchorPitch = 0.f;
	out.density = 0.f;
	out.direction = 0;
	out.rhythmGrid = 0;

	const int n = src.count;
	if (n <= 0)
		return;

	out.anchorPitch = src.events[0].pitch;

	const int limit = n < NormalizedMotive::MAX_EVENTS ? n : NormalizedMotive::MAX_EVENTS;
	int ascChanges = 0;
	int descChanges = 0;
	float prevPitch = src.events[0].pitch;

	for (int i = 0; i < limit; i++) {
		const MotiveEvent& e = src.events[i];
		NormalizedEvent& ne = out.events[out.count++];
		ne.startPos      = e.startPos;
		ne.gateLen       = e.gateLen;
		ne.pitchInterval = e.pitch - out.anchorPitch;
		ne.velocity      = e.velocity;

		int tickSlot = int(e.startPos);
		if (tickSlot < 0) tickSlot = 0;
		if (tickSlot >= NormalizedMotive::PHRASE_TICKS) tickSlot = NormalizedMotive::PHRASE_TICKS - 1;
		out.rhythmGrid |= uint16_t(1u << tickSlot);

		if (i > 0) {
			const float d = e.pitch - prevPitch;
			if (d >  1e-4f) ascChanges++;
			else if (d < -1e-4f) descChanges++;
		}
		prevPitch = e.pitch;
	}

	out.density = float(out.count) / float(NormalizedMotive::PHRASE_TICKS);

	if (ascChanges > descChanges + (ascChanges + descChanges) / 4)
		out.direction = +1;
	else if (descChanges > ascChanges + (ascChanges + descChanges) / 4)
		out.direction = -1;
	else
		out.direction = 0;
}

} // namespace omnisfear
