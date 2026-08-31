#pragma once
#include "MotiveEvent.hpp"

namespace omnisfear {

// Fixed-capacity event buffer — no heap allocations from the audio thread.
template <int Capacity>
struct PhraseBuffer {
	MotiveEvent events[Capacity];
	int count = 0;

	void clear() {
		count = 0;
	}

	bool add(const MotiveEvent& e) {
		if (count >= Capacity)
			return false;
		events[count++] = e;
		return true;
	}

	// Insertion sort by startPos. Only called at phrase boundaries.
	void sortByStart() {
		for (int i = 1; i < count; i++) {
			MotiveEvent x = events[i];
			int j = i - 1;
			while (j >= 0 && events[j].startPos > x.startPos) {
				events[j + 1] = events[j];
				j--;
			}
			events[j + 1] = x;
		}
	}
};

} // namespace omnisfear
