#pragma once
#include "MotiveEvent.hpp"
#include "PhraseBuffer.hpp"
#include "NormalizedMotive.hpp"
#include "Rng.hpp"

namespace omnisfear {

// Musical FSM: waits for CLOCK, captures a phrase from CV/GATE, then loops it.
// Pure C++ / Rack-independent. Rack adapter feeds it edge-detected inputs.
struct MotiveEngine {
	enum Mode {
		CAPTURE,
		REPLAY
	};

	static constexpr int PHRASE_TICKS = 16;
	static constexpr int BUFFER_CAP   = 128;

	struct Inputs {
		bool  clockEdge;     // rising edge of CLOCK this frame
		bool  resetEdge;     // rising edge of RESET this frame
		bool  gateHigh;      // current GATE state (post-Schmitt)
		bool  gateEdgeUp;    // GATE rising edge this frame
		bool  gateEdgeDown;  // GATE falling edge this frame
		float cv;            // CV IN volts
		float sampleTime;    // seconds per sample
	};

	struct Outputs {
		float cv;
		float gate;          // 10 V high, 0 V low
		float accent;        // 0..10 V (velocity * 10)
		bool  phraseTrigger; // true this frame if a new phrase started
	};

	Mode mode = CAPTURE;
	PhraseBuffer<BUFFER_CAP> buffer;
	NormalizedMotive normalized;
	Rng rng;

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
	Outputs process(const Inputs& in);
};

} // namespace omnisfear
