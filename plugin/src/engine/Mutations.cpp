#include "Mutations.hpp"
#include <cmath>

namespace omnisfear {

static inline float clampf(float x, float lo, float hi) {
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}

void sortByStart(NormalizedMotive& m) {
	for (int i = 1; i < m.count; i++) {
		NormalizedEvent x = m.events[i];
		int j = i - 1;
		while (j >= 0 && m.events[j].startPos > x.startPos) {
			m.events[j + 1] = m.events[j];
			j--;
		}
		m.events[j + 1] = x;
	}
}

void recomputeGrid(NormalizedMotive& m) {
	m.rhythmGrid = 0;
	for (int i = 0; i < m.count; i++) {
		int slot = int(m.events[i].startPos);
		if (slot < 0) slot = 0;
		if (slot >= NormalizedMotive::PHRASE_TICKS) slot = NormalizedMotive::PHRASE_TICKS - 1;
		m.rhythmGrid |= uint16_t(1u << slot);
	}
	m.density = float(m.count) / float(NormalizedMotive::PHRASE_TICKS);
}

void recomputeDirection(NormalizedMotive& m) {
	if (m.count < 2) {
		m.direction = 0;
		return;
	}
	int asc = 0, desc = 0;
	float prev = m.events[0].pitchInterval;
	for (int i = 1; i < m.count; i++) {
		float cur = m.events[i].pitchInterval;
		float d = cur - prev;
		if (d >  1e-4f) asc++;
		else if (d < -1e-4f) desc++;
		prev = cur;
	}
	int total = asc + desc;
	int margin = total / 4;
	if (asc > desc + margin) m.direction = +1;
	else if (desc > asc + margin) m.direction = -1;
	else m.direction = 0;
}

void mutate_transpose(NormalizedMotive& m, float deltaVolts) {
	m.anchorPitch = clampf(m.anchorPitch + deltaVolts, -MAX_ANCHOR, MAX_ANCHOR);
}

void mutate_invertContour(NormalizedMotive& m) {
	for (int i = 0; i < m.count; i++) {
		m.events[i].pitchInterval = clampf(-m.events[i].pitchInterval, -MAX_INTERVAL, MAX_INTERVAL);
	}
	recomputeDirection(m);
}

void mutate_reverseContour(NormalizedMotive& m) {
	for (int i = 0, j = m.count - 1; i < j; i++, j--) {
		float tmp = m.events[i].pitchInterval;
		m.events[i].pitchInterval = m.events[j].pitchInterval;
		m.events[j].pitchInterval = tmp;
	}
	recomputeDirection(m);
}

void mutate_rotateRhythm(NormalizedMotive& m, int ticks) {
	if (m.count == 0) return;
	const float T = float(NormalizedMotive::PHRASE_TICKS);
	float shift = std::fmod(float(ticks), T);
	if (shift < 0.f) shift += T;
	for (int i = 0; i < m.count; i++) {
		float p = m.events[i].startPos + shift;
		if (p >= T) p -= T;
		m.events[i].startPos = p;
	}
	sortByStart(m);
	recomputeGrid(m);
}

void mutate_expandIntervals(NormalizedMotive& m, float factor) {
	for (int i = 0; i < m.count; i++) {
		m.events[i].pitchInterval = clampf(m.events[i].pitchInterval * factor, -MAX_INTERVAL, MAX_INTERVAL);
	}
	recomputeDirection(m);
}

void mutate_octaveDisplacement(NormalizedMotive& m, Rng& rng, float prob) {
	for (int i = 0; i < m.count; i++) {
		if (rng.uniform() < prob) {
			float sign = (rng.uniform() < 0.5f) ? -1.f : 1.f;
			float next = m.events[i].pitchInterval + sign * 1.f;
			if (next > MAX_INTERVAL || next < -MAX_INTERVAL)
				next = m.events[i].pitchInterval - sign * 1.f;
			m.events[i].pitchInterval = clampf(next, -MAX_INTERVAL, MAX_INTERVAL);
		}
	}
	recomputeDirection(m);
}

void mutate_addNeighbor(NormalizedMotive& m, Rng& rng) {
	if (m.count == 0 || m.count >= NormalizedMotive::MAX_EVENTS) return;
	int idx = int(rng.uniform() * float(m.count));
	if (idx >= m.count) idx = m.count - 1;
	const NormalizedEvent& src = m.events[idx];
	NormalizedEvent n;

	float dt = 0.1f + rng.uniform() * 0.4f;
	n.startPos = src.startPos + dt;
	const float T = float(NormalizedMotive::PHRASE_TICKS);
	if (n.startPos >= T) n.startPos -= T;

	n.gateLen = 0.5f + rng.uniform() * 0.5f;

	// Neighbor: ±1 or ±2 semitones from source, excluding zero.
	int step = int(rng.next() % 4) - 2;
	if (step == 0) step = 1;
	n.pitchInterval = clampf(src.pitchInterval + float(step) / 12.f, -MAX_INTERVAL, MAX_INTERVAL);
	n.velocity = src.velocity * 0.7f;

	m.events[m.count++] = n;
	sortByStart(m);
	recomputeGrid(m);
	recomputeDirection(m);
}

void mutate_removeEvent(NormalizedMotive& m, Rng& rng) {
	if (m.count == 0) return;
	int idx = int(rng.uniform() * float(m.count));
	if (idx >= m.count) idx = m.count - 1;
	for (int i = idx; i < m.count - 1; i++) {
		m.events[i] = m.events[i + 1];
	}
	m.count--;
	recomputeGrid(m);
	recomputeDirection(m);
}

static int popcount16(uint16_t x) {
	int c = 0;
	while (x) { c += int(x & 1u); x >>= 1; }
	return c;
}

float similarity(const NormalizedMotive& a, const NormalizedMotive& b) {
	float rhythm;
	{
		int inter = popcount16(uint16_t(a.rhythmGrid & b.rhythmGrid));
		int uni   = popcount16(uint16_t(a.rhythmGrid | b.rhythmGrid));
		rhythm = (uni == 0) ? 1.f : float(inter) / float(uni);
	}

	float pitch = 1.f;
	{
		int n = a.count < b.count ? a.count : b.count;
		if (n > 0) {
			float sum = 0.f;
			for (int k = 0; k < n; k++) {
				float d = std::fabs(a.events[k].pitchInterval - b.events[k].pitchInterval) * 12.f;
				sum += d;
			}
			float mean = sum / float(n);
			float s = 1.f - mean / 12.f;
			if (s < 0.f) s = 0.f;
			pitch = s;
		}
	}

	float density;
	{
		float md = std::fabs(a.density - b.density);
		float mx = a.density > b.density ? a.density : b.density;
		density = (mx <= 1e-6f) ? 1.f : (1.f - md / mx);
		if (density < 0.f) density = 0.f;
	}

	float dir;
	if (a.direction == b.direction) dir = 1.f;
	else if (a.direction == 0 || b.direction == 0) dir = 0.5f;
	else dir = 0.f;

	return 0.4f * rhythm + 0.4f * pitch + 0.15f * density + 0.05f * dir;
}

} // namespace omnisfear
