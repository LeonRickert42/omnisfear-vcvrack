#pragma once
#include "MotiveEvent.hpp"
#include "PhraseBuffer.hpp"
#include "NormalizedMotive.hpp"
#include "MotiveMemory.hpp"
#include "Rng.hpp"

namespace omnisfear {

// Musical FSM: waits for CLOCK, captures a phrase from CV/GATE, then evolves it
// each phrase by continuing, mutating, recalling from memory, or creating a
// variation. Pure C++ / Rack-independent.
struct MotiveEngine {
	enum Mode {
		CAPTURE,
		REPLAY
	};

	enum PhraseAction {
		ACT_CONTINUE,
		ACT_MUTATE,
		ACT_RECALL,
		ACT_VARIATE
	};

	static constexpr int PHRASE_TICKS = 16;
	static constexpr int BUFFER_CAP   = 128;
	static constexpr int MEMORY_CAP   = 32;

	struct Inputs {
		bool  clockEdge;
		bool  resetEdge;
		bool  gateHigh;
		bool  gateEdgeUp;
		bool  gateEdgeDown;
		bool  splashEdge;
		bool  prevEdge;
		float cv;
		float sampleTime;

		float mutation  = 0.f;
		float variation = 0.f;
		float density   = 0.5f;
		float memory    = 0.5f;
		float follow    = 0.5f;
	};

	struct Outputs {
		float cv;
		float gate;
		float accent;
		bool  phraseTrigger;
	};

	Mode mode = CAPTURE;
	PhraseBuffer<BUFFER_CAP> buffer;
	NormalizedMotive normalized;
	MotiveHistory<MEMORY_CAP> history;
	Rng rng;

	NormalizedMotive snapshot;
	bool hasSnapshot = false;

	PhraseAction lastAction = ACT_CONTINUE;

	int   currentTick        = -1;
	float timeSinceLastTick  = 0.f;
	float estTickDurationS   = 0.125f;

	float pendingGateStart      = 0.f;
	float pendingGatePitch      = 0.f;
	bool  captureGateOpen       = false;

	int   replayCursor          = 0;
	float replayGateOffPos      = -1.f;

	float latchedCV     = 0.f;
	float latchedGate   = 0.f;
	float latchedAccent = 0.f;

	void reset();
	void clearMemory();
	Outputs process(const Inputs& in);

	// Exposed for tests and for the phrase-boundary logic in process().
	PhraseAction decidePhraseAction(const Inputs& in);
	void         applyPhraseAction(PhraseAction a, const Inputs& in);
};

} // namespace omnisfear
