#include "MotiveEngine.hpp"
#include "MutationPolicy.hpp"
#include "Mutations.hpp"
#include <algorithm>
#include <cmath>

namespace omnisfear {

static float clamp01(float x) {
	if (x < 0.f) return 0.f;
	if (x > 1.f) return 1.f;
	return x;
}

static float meanVelocity(const NormalizedMotive& m) {
	if (m.count == 0) return 0.f;
	float s = 0.f;
	for (int i = 0; i < m.count; i++) s += m.events[i].velocity;
	return s / float(m.count);
}

void MotiveEngine::reset() {
	mode = CAPTURE;
	buffer.clear();
	normalized.count = 0;
	normalized.anchorPitch = 0.f;
	normalized.density = 0.f;
	normalized.direction = 0;
	normalized.rhythmGrid = 0;
	lastAction        = ACT_CONTINUE;
	currentTick       = -1;
	timeSinceLastTick = 0.f;
	pendingGateStart  = 0.f;
	pendingGatePitch  = 0.f;
	captureGateOpen   = false;
	replayCursor      = 0;
	replayGateOffPos  = -1.f;
	latchedCV     = 0.f;
	latchedGate   = 0.f;
	latchedAccent = 0.f;
}

void MotiveEngine::clearMemory() {
	history.clear();
}

MotiveEngine::PhraseAction MotiveEngine::decidePhraseAction(const Inputs& in) {
	const float variation = clamp01(in.variation);
	const float memory    = clamp01(in.memory);
	const float follow    = clamp01(in.follow);

	if (variation < 1e-3f || rng.uniform() > variation) {
		return ACT_CONTINUE;
	}

	float pRecall = memory * 0.7f;
	if (history.count == 0) pRecall = 0.f;

	float pMutate  = follow * (1.f - pRecall);
	float pVariate = 1.f - pRecall - pMutate;
	if (pVariate < 0.f) pVariate = 0.f;

	float r = rng.uniform();
	if (r < pRecall)             return ACT_RECALL;
	if (r < pRecall + pMutate)   return ACT_MUTATE;
	return ACT_VARIATE;
}

void MotiveEngine::applyPhraseAction(PhraseAction a, const Inputs& in) {
	const float mutation = clamp01(in.mutation);
	const float density  = clamp01(in.density);
	const float follow   = clamp01(in.follow);

	switch (a) {
		case ACT_CONTINUE:
			break;

		case ACT_MUTATE:
			applyMutationCycle(normalized, rng, mutation, density);
			break;

		case ACT_VARIATE: {
			// Drift further while remaining structurally related. Extra ops,
			// stronger transformations, capped at 1.0.
			float m = mutation + 0.35f;
			if (m > 1.f) m = 1.f;
			applyMutationCycle(normalized, rng, m, density);
			break;
		}

		case ACT_RECALL: {
			int idx = history.pickWeighted(rng, normalized, follow);
			if (idx >= 0) {
				normalized = history.entries[idx].motive;
				history.entries[idx].usage++;
			}
			break;
		}
	}
}

MotiveEngine::Outputs MotiveEngine::process(const Inputs& in) {
	Outputs out{latchedCV, latchedGate, latchedAccent, false};

	if (in.resetEdge) {
		reset();
		out.phraseTrigger = true;
	}

	// SPLASH forces a phrase boundary now with a boosted VARIATE.
	if (in.splashEdge && mode == REPLAY && normalized.count > 0) {
		history.ageAll(clamp01(in.memory));

		NormalizedMotive prev = normalized;
		Inputs boosted = in;
		boosted.mutation = std::min(1.f, in.mutation + 0.4f);
		applyPhraseAction(ACT_VARIATE, boosted);
		lastAction = ACT_VARIATE;

		float sim = similarity(prev, normalized);
		float nov = 1.f - sim;
		if (nov > 0.15f) {
			history.push(normalized, nov, normalized.density * meanVelocity(normalized));
		}

		currentTick        = 0;
		replayCursor       = 0;
		replayGateOffPos   = -1.f;
		latchedGate        = 0.f;
		latchedAccent      = 0.f;
		timeSinceLastTick  = 0.f;
		out.phraseTrigger  = true;
	}

	bool phraseStart = false;
	if (in.clockEdge) {
		if (currentTick >= 0 && timeSinceLastTick > 1e-6f) {
			estTickDurationS = 0.9f * estTickDurationS + 0.1f * timeSinceLastTick;
		}
		timeSinceLastTick = 0.f;

		if (currentTick < 0) {
			currentTick = 0;
			phraseStart = true;
		}
		else {
			currentTick++;
			if (currentTick >= PHRASE_TICKS) {
				currentTick = 0;
				phraseStart = true;

				if (mode == CAPTURE) {
					buffer.sortByStart();
					normalize(buffer, normalized);
					mode = REPLAY;
					if (normalized.count > 0) {
						history.push(normalized, 0.f, normalized.density * meanVelocity(normalized));
					}
					lastAction = ACT_CONTINUE;
				}
				else {
					history.ageAll(clamp01(in.memory));

					PhraseAction a = decidePhraseAction(in);
					NormalizedMotive prev = normalized;
					applyPhraseAction(a, in);
					lastAction = a;

					if (a == ACT_MUTATE || a == ACT_VARIATE) {
						float sim = similarity(prev, normalized);
						float nov = 1.f - sim;
						if (nov > 0.15f && normalized.count > 0) {
							history.push(normalized, nov, normalized.density * meanVelocity(normalized));
						}
					}
				}

				replayCursor     = 0;
				replayGateOffPos = -1.f;
				latchedGate      = 0.f;
				latchedAccent    = 0.f;
			}
		}
	}

	if (phraseStart)
		out.phraseTrigger = true;

	if (currentTick < 0) {
		latchedCV = 0.f; latchedGate = 0.f; latchedAccent = 0.f;
		out.cv = 0.f; out.gate = 0.f; out.accent = 0.f;
		return out;
	}

	timeSinceLastTick += in.sampleTime;

	const float safeTick  = std::max(estTickDurationS, 1e-4f);
	const float subPhase  = std::min(timeSinceLastTick / safeTick, 1.0f);
	const float phrasePos = float(currentTick) + subPhase;

	if (mode == CAPTURE) {
		if (in.gateEdgeUp) {
			pendingGateStart = phrasePos;
			pendingGatePitch = in.cv;
			captureGateOpen  = true;
		}
		if (in.gateEdgeDown && captureGateOpen) {
			float gateLen = phrasePos - pendingGateStart;
			if (gateLen < 0.f)
				gateLen += float(PHRASE_TICKS);
			if (gateLen < 0.05f)
				gateLen = 0.05f;
			MotiveEvent e{pendingGateStart, gateLen, pendingGatePitch, 1.f};
			buffer.add(e);
			captureGateOpen = false;
		}
		latchedCV     = in.cv;
		latchedGate   = in.gateHigh ? 10.f : 0.f;
		latchedAccent = 0.f;
	}
	else {
		while (replayCursor < normalized.count && normalized.events[replayCursor].startPos <= phrasePos + 1e-5f) {
			const NormalizedEvent& e = normalized.events[replayCursor];
			latchedCV        = normalized.anchorPitch + e.pitchInterval;
			latchedGate      = 10.f;
			latchedAccent    = e.velocity * 10.f;
			replayGateOffPos = e.startPos + e.gateLen;
			replayCursor++;
		}
		if (replayGateOffPos >= 0.f && phrasePos >= replayGateOffPos) {
			latchedGate      = 0.f;
			replayGateOffPos = -1.f;
		}
	}

	out.cv     = latchedCV;
	out.gate   = latchedGate;
	out.accent = latchedAccent;
	return out;
}

} // namespace omnisfear
