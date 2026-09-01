#pragma once
#include "NormalizedMotive.hpp"
#include "Rng.hpp"

namespace omnisfear {

// Applies a small chain of mutation operations to the motive in-place.
// `mutation` in [0,1] controls how transformative the ops are and how many are picked.
// `density` in [0,1] biases add-event vs remove-event probability.
void applyMutationCycle(NormalizedMotive& m, Rng& rng, float mutation, float density);

} // namespace omnisfear
