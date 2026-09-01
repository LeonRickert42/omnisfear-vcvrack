#include "MotiveEngine.hpp"
#include "MutationPolicy.hpp"
#include <algorithm>

namespace omnisfear {

void MotiveEngine::reset() {
	mode = CAPTURE;
	buffer.clear();
	normalized.count = 0;
	normalized.anchorPitch = 0.f;
	normalized.density = 0.f;
	normalized.direction = 0;
	normalized.rhythmGrid = 0;
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

MotiveEngine::Outputs MotiveEngine::process(const Inputs& in) {
	Outputs out{latchedCV, latchedGate, latchedAccent, false};

	if (in.resetEdge) {
		reset();
		out.phraseTrigger = true;
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
				}
				else {
					if (in.mutation > 0.01f && in.variation > 0.f && rng.uniform() < in.variation) {
						applyMutationCycle(normalized, rng, in.mutation, in.density);
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
