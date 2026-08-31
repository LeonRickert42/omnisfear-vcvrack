#pragma once
#include <cstdint>

namespace omnisfear {

// SplitMix64 — deterministic, fast, no state-dependency shenanigans.
struct Rng {
	uint64_t state = 0x9E3779B97F4A7C15ULL;

	void seed(uint64_t s) {
		state = s ? s : 0x9E3779B97F4A7C15ULL;
	}

	uint64_t next() {
		state += 0x9E3779B97F4A7C15ULL;
		uint64_t z = state;
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
		return z ^ (z >> 31);
	}

	// Uniform in [0, 1).
	float uniform() {
		return (next() >> 40) * (1.0f / float(1 << 24));
	}
};

} // namespace omnisfear
