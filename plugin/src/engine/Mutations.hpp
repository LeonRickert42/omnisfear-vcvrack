#pragma once
#include "NormalizedMotive.hpp"
#include "Rng.hpp"

namespace omnisfear {

// Pure transformations of a NormalizedMotive. In-place, deterministic given rng.
// Each keeps musical identity as much as possible for its category.

void mutate_transpose         (NormalizedMotive& m, float deltaVolts);
void mutate_invertContour     (NormalizedMotive& m);
void mutate_reverseContour    (NormalizedMotive& m);
void mutate_rotateRhythm      (NormalizedMotive& m, int ticks);
void mutate_expandIntervals   (NormalizedMotive& m, float factor);
void mutate_octaveDisplacement(NormalizedMotive& m, Rng& rng, float prob);
void mutate_addNeighbor       (NormalizedMotive& m, Rng& rng);
void mutate_removeEvent       (NormalizedMotive& m, Rng& rng);

// Derived-field helpers used by mutations. Exposed for tests.
void recomputeGrid            (NormalizedMotive& m);
void recomputeDirection       (NormalizedMotive& m);
void sortByStart              (NormalizedMotive& m);

// Structural similarity in [0, 1]. 1 = identical, 0 = maximally different.
float similarity(const NormalizedMotive& a, const NormalizedMotive& b);

} // namespace omnisfear
