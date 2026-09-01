#include "MutationPolicy.hpp"
#include "Mutations.hpp"

namespace omnisfear {

static float clamp01(float x) {
	if (x < 0.f) return 0.f;
	if (x > 1.f) return 1.f;
	return x;
}

void applyMutationCycle(NormalizedMotive& m, Rng& rng, float mutation, float density) {
	if (m.count == 0) return;
	mutation = clamp01(mutation);
	density  = clamp01(density);

	// 1..4 ops depending on mutation strength.
	int nOps = 1 + int(mutation * 3.f + 0.5f);

	for (int i = 0; i < nOps; i++) {
		float r = rng.uniform();

		if (mutation < 0.34f) {
			// Gentle range: preserve identity, add flavor.
			if (r < 0.30f) {
				float dv = (rng.uniform() - 0.5f) * (2.f / 12.f);
				mutate_transpose(m, dv);
			}
			else if (r < 0.55f) {
				mutate_octaveDisplacement(m, rng, 0.06f);
			}
			else if (r < 0.80f && rng.uniform() < density) {
				mutate_addNeighbor(m, rng);
			}
			else if (rng.uniform() > density) {
				mutate_removeEvent(m, rng);
			}
			else {
				float dv = (rng.uniform() - 0.5f) * (1.f / 12.f);
				mutate_transpose(m, dv);
			}
		}
		else if (mutation < 0.67f) {
			// Moderate: rhythm displacement + interval reshaping.
			if (r < 0.20f) {
				int shift = 1 + int(rng.next() % 3u);
				mutate_rotateRhythm(m, shift);
			}
			else if (r < 0.45f) {
				float factor = 0.75f + rng.uniform() * 0.5f;
				mutate_expandIntervals(m, factor);
			}
			else if (r < 0.70f) {
				mutate_octaveDisplacement(m, rng, 0.18f);
			}
			else if (rng.uniform() < density) {
				mutate_addNeighbor(m, rng);
			}
			else {
				mutate_removeEvent(m, rng);
			}
		}
		else {
			// Heavy: invert / reverse / big shifts.
			if (r < 0.15f) {
				mutate_invertContour(m);
			}
			else if (r < 0.30f) {
				mutate_reverseContour(m);
			}
			else if (r < 0.55f) {
				int shift = 2 + int(rng.next() % 5u);
				mutate_rotateRhythm(m, shift);
			}
			else if (r < 0.75f) {
				float factor = (rng.uniform() < 0.5f) ? (0.4f + rng.uniform() * 0.2f)
				                                      : (1.6f + rng.uniform() * 0.6f);
				mutate_expandIntervals(m, factor);
			}
			else if (r < 0.90f) {
				mutate_octaveDisplacement(m, rng, 0.30f);
			}
			else if (rng.uniform() < density) {
				mutate_addNeighbor(m, rng);
			}
			else {
				mutate_removeEvent(m, rng);
			}
		}
	}
}

} // namespace omnisfear
