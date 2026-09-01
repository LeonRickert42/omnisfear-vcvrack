#pragma once
#include "NormalizedMotive.hpp"
#include "Mutations.hpp"
#include "Rng.hpp"
#include <cmath>

namespace omnisfear {

// A remembered motive with book-keeping used by the recall policy.
struct MotiveMemory {
	NormalizedMotive motive;
	float age;      // in phrases, decayed by (1 - MEMORY) each boundary
	float energy;   // proxy: density * mean velocity
	float novelty;  // 1 - similarity to previous motive at capture time
	int   usage;    // times recalled
};

// Ring-buffer of remembered motives. Fixed capacity, no heap.
template <int Cap>
struct MotiveHistory {
	MotiveMemory entries[Cap];
	int count = 0;
	int nextWrite = 0;

	static constexpr int capacity() { return Cap; }

	void clear() {
		count = 0;
		nextWrite = 0;
	}

	// Called once per phrase boundary. memoryStrength in [0,1].
	// At memory=1 nothing ages; at memory=0 the age rises fast so old motives fall out of reach.
	void ageAll(float memoryStrength) {
		if (memoryStrength < 0.f) memoryStrength = 0.f;
		if (memoryStrength > 1.f) memoryStrength = 1.f;
		const float step = 0.05f + (1.f - memoryStrength) * 0.95f;
		for (int i = 0; i < count; i++) {
			entries[i].age += step;
		}
	}

	void push(const NormalizedMotive& m, float novelty, float energy) {
		MotiveMemory& slot = entries[nextWrite];
		slot.motive  = m;
		slot.age     = 0.f;
		slot.novelty = novelty;
		slot.energy  = energy;
		slot.usage   = 0;
		nextWrite++;
		if (nextWrite >= Cap) nextWrite = 0;
		if (count < Cap) count++;
	}

	// Weighted pick. Weight = decay(age) * followBias(similarity to current).
	// followStrength in [0,1]: 1 = strongly prefer similar to `current`, 0 = prefer novelty.
	// Returns index in entries[] or -1 if empty.
	int pickWeighted(Rng& rng, const NormalizedMotive& current, float followStrength) const {
		if (count == 0) return -1;
		if (followStrength < 0.f) followStrength = 0.f;
		if (followStrength > 1.f) followStrength = 1.f;

		float total = 0.f;
		float weights[Cap];
		for (int i = 0; i < count; i++) {
			const MotiveMemory& m = entries[i];
			float w = 1.f / (1.f + m.age);
			float sim = similarity(m.motive, current);
			float bias = followStrength * sim + (1.f - followStrength) * (1.f - sim);
			if (bias < 0.05f) bias = 0.05f;
			w *= bias;
			weights[i] = w;
			total += w;
		}
		if (total <= 1e-6f) return int(rng.next() % uint64_t(count));

		float r = rng.uniform() * total;
		float acc = 0.f;
		for (int i = 0; i < count; i++) {
			acc += weights[i];
			if (r <= acc) return i;
		}
		return count - 1;
	}
};

} // namespace omnisfear
