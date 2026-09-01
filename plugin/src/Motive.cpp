#include "plugin.hpp"
#include "PngImage.hpp"
#include "engine/MotiveEngine.hpp"

// OMNISFEAR MOTIVE — v0.2 capture & replay
// See ../Vorüberlergungen/PROJECT OMNISFEAR.md for the design.

struct Motive : Module {
	enum ParamId {
		MEMORY_PARAM,
		MUTATION_PARAM,
		FOLLOW_PARAM,
		DENSITY_PARAM,
		VARIATION_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_INPUT,
		CV_INPUT,
		GATE_INPUT,
		RESET_INPUT,
		MUT_CV_INPUT,
		VAR_CV_INPUT,
		DEN_CV_INPUT,
		SPLASH_INPUT,
		PREV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,
		GATE_OUTPUT,
		ACCENT_OUTPUT,
		PHRASE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	omnisfear::MotiveEngine engine;
	dsp::SchmittTrigger clockTrig;
	dsp::SchmittTrigger resetTrig;
	dsp::SchmittTrigger gateTrig;
	dsp::SchmittTrigger splashTrig;
	dsp::SchmittTrigger prevTrig;
	dsp::PulseGenerator phrasePulse;

	Motive() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(MEMORY_PARAM,    0.f, 1.f, 0.5f,  "Memory",    "%", 0.f, 100.f);
		configParam(MUTATION_PARAM,  0.f, 1.f, 0.25f, "Mutation",  "%", 0.f, 100.f);
		configParam(FOLLOW_PARAM,    0.f, 1.f, 0.5f,  "Follow",    "%", 0.f, 100.f);
		configParam(DENSITY_PARAM,   0.f, 1.f, 0.5f,  "Density",   "%", 0.f, 100.f);
		configParam(VARIATION_PARAM, 0.f, 1.f, 0.25f, "Variation", "%", 0.f, 100.f);

		configInput(CLOCK_INPUT, "Clock");
		configInput(CV_INPUT,    "Pitch CV");
		configInput(GATE_INPUT,  "Gate");
		configInput(RESET_INPUT, "Reset");
		configInput(MUT_CV_INPUT, "Mutation CV");
		configInput(VAR_CV_INPUT, "Variation CV");
		configInput(DEN_CV_INPUT, "Density CV (bipolar)");
		configInput(SPLASH_INPUT, "Splash trigger");
		configInput(PREV_INPUT,   "Rewind (previous) trigger");

		configOutput(CV_OUTPUT,     "Pitch CV");
		configOutput(GATE_OUTPUT,   "Gate");
		configOutput(ACCENT_OUTPUT, "Accent");
		configOutput(PHRASE_OUTPUT, "Phrase");

		engine.rng.seed(random::u64());
	}

	void onReset() override {
		engine.reset();
		engine.clearMemory();
	}

	void onRandomize() override {
		engine.rng.seed(random::u64());
	}

	void process(const ProcessArgs& args) override {
		omnisfear::MotiveEngine::Inputs in;
		in.sampleTime = args.sampleTime;
		in.cv         = inputs[CV_INPUT].getVoltage();
		in.clockEdge  = clockTrig.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 1.f);
		in.resetEdge  = resetTrig.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f);
		in.splashEdge = splashTrig.process(inputs[SPLASH_INPUT].getVoltage(), 0.1f, 1.f);
		in.prevEdge   = prevTrig.process(inputs[PREV_INPUT].getVoltage(), 0.1f, 1.f);

		auto gEvent = gateTrig.processEvent(inputs[GATE_INPUT].getVoltage(), 0.1f, 1.f);
		in.gateHigh     = gateTrig.isHigh();
		in.gateEdgeUp   = (gEvent == dsp::SchmittTrigger::TRIGGERED);
		in.gateEdgeDown = (gEvent == dsp::SchmittTrigger::UNTRIGGERED);

		float mutCv = inputs[MUT_CV_INPUT].getVoltage() / 10.f;
		float varCv = inputs[VAR_CV_INPUT].getVoltage() / 10.f;
		float denCv = inputs[DEN_CV_INPUT].getVoltage() / 10.f;

		in.mutation  = math::clamp(params[MUTATION_PARAM].getValue()  + mutCv, 0.f, 1.f);
		in.variation = math::clamp(params[VARIATION_PARAM].getValue() + varCv, 0.f, 1.f);
		in.density   = math::clamp(params[DENSITY_PARAM].getValue()   + denCv, 0.f, 1.f);
		in.memory    = params[MEMORY_PARAM].getValue();
		in.follow    = params[FOLLOW_PARAM].getValue();

		auto outE = engine.process(in);

		if (outE.phraseTrigger)
			phrasePulse.trigger(1e-3f);

		outputs[CV_OUTPUT].setVoltage(outE.cv);
		outputs[GATE_OUTPUT].setVoltage(outE.gate);
		outputs[ACCENT_OUTPUT].setVoltage(outE.accent);
		outputs[PHRASE_OUTPUT].setVoltage(phrasePulse.process(args.sampleTime) ? 10.f : 0.f);
	}

	static json_t* eventsToJson(const omnisfear::NormalizedEvent* events, int count) {
		json_t* arr = json_array();
		for (int i = 0; i < count; i++) {
			json_t* e = json_object();
			json_object_set_new(e, "s", json_real(events[i].startPos));
			json_object_set_new(e, "g", json_real(events[i].gateLen));
			json_object_set_new(e, "p", json_real(events[i].pitchInterval));
			json_object_set_new(e, "v", json_real(events[i].velocity));
			json_array_append_new(arr, e);
		}
		return arr;
	}

	static void eventsFromJson(json_t* arr, omnisfear::NormalizedEvent* events, int& count) {
		count = 0;
		if (!arr || !json_is_array(arr)) return;
		size_t n = json_array_size(arr);
		if (int(n) > omnisfear::NormalizedMotive::MAX_EVENTS)
			n = omnisfear::NormalizedMotive::MAX_EVENTS;
		for (size_t i = 0; i < n; i++) {
			json_t* e = json_array_get(arr, i);
			if (!e) continue;
			omnisfear::NormalizedEvent& ne = events[count++];
			ne.startPos      = json_number_value(json_object_get(e, "s"));
			ne.gateLen       = json_number_value(json_object_get(e, "g"));
			ne.pitchInterval = json_number_value(json_object_get(e, "p"));
			ne.velocity      = json_number_value(json_object_get(e, "v"));
		}
	}

	static json_t* motiveToJson(const omnisfear::NormalizedMotive& m) {
		json_t* o = json_object();
		json_object_set_new(o, "anchor",    json_real(m.anchorPitch));
		json_object_set_new(o, "density",   json_real(m.density));
		json_object_set_new(o, "direction", json_integer(m.direction));
		json_object_set_new(o, "grid",      json_integer(m.rhythmGrid));
		json_object_set_new(o, "events",    eventsToJson(m.events, m.count));
		return o;
	}

	static void motiveFromJson(json_t* o, omnisfear::NormalizedMotive& m) {
		m.count = 0;
		m.anchorPitch = 0.f;
		m.density = 0.f;
		m.direction = 0;
		m.rhythmGrid = 0;
		if (!o) return;
		if (json_t* j = json_object_get(o, "anchor"))    m.anchorPitch = json_number_value(j);
		if (json_t* j = json_object_get(o, "density"))   m.density     = json_number_value(j);
		if (json_t* j = json_object_get(o, "direction")) m.direction   = int(json_integer_value(j));
		if (json_t* j = json_object_get(o, "grid"))      m.rhythmGrid  = uint16_t(json_integer_value(j));
		eventsFromJson(json_object_get(o, "events"), m.events, m.count);
	}

	json_t* dataToJson() override {
		json_t* root = json_object();

		json_object_set_new(root, "rngState", json_integer(int64_t(engine.rng.state)));
		json_object_set_new(root, "mode",     json_integer(int(engine.mode)));
		json_object_set_new(root, "tickDur",  json_real(engine.estTickDurationS));

		json_object_set_new(root, "motive",   motiveToJson(engine.normalized));

		json_t* hist = json_array();
		for (int i = 0; i < engine.history.count; i++) {
			const auto& mm = engine.history.entries[i];
			json_t* o = json_object();
			json_object_set_new(o, "m",   motiveToJson(mm.motive));
			json_object_set_new(o, "age", json_real(mm.age));
			json_object_set_new(o, "en",  json_real(mm.energy));
			json_object_set_new(o, "nv",  json_real(mm.novelty));
			json_object_set_new(o, "u",   json_integer(mm.usage));
			json_array_append_new(hist, o);
		}
		json_object_set_new(root, "history",    hist);
		json_object_set_new(root, "historyNext", json_integer(engine.history.nextWrite));

		return root;
	}

	void dataFromJson(json_t* root) override {
		if (!root) return;

		if (json_t* j = json_object_get(root, "rngState"))
			engine.rng.state = uint64_t(json_integer_value(j));
		if (json_t* j = json_object_get(root, "mode"))
			engine.mode = (json_integer_value(j) == 0) ? omnisfear::MotiveEngine::CAPTURE : omnisfear::MotiveEngine::REPLAY;
		if (json_t* j = json_object_get(root, "tickDur"))
			engine.estTickDurationS = json_number_value(j);

		motiveFromJson(json_object_get(root, "motive"), engine.normalized);

		engine.history.clear();
		json_t* hist = json_object_get(root, "history");
		if (hist && json_is_array(hist)) {
			size_t n = json_array_size(hist);
			if (int(n) > omnisfear::MotiveEngine::MEMORY_CAP)
				n = omnisfear::MotiveEngine::MEMORY_CAP;
			for (size_t i = 0; i < n; i++) {
				json_t* o = json_array_get(hist, i);
				if (!o) continue;
				auto& mm = engine.history.entries[engine.history.count];
				motiveFromJson(json_object_get(o, "m"), mm.motive);
				mm.age     = json_number_value(json_object_get(o, "age"));
				mm.energy  = json_number_value(json_object_get(o, "en"));
				mm.novelty = json_number_value(json_object_get(o, "nv"));
				mm.usage   = int(json_integer_value(json_object_get(o, "u")));
				engine.history.count++;
			}
		}
		if (json_t* j = json_object_get(root, "historyNext")) {
			int nw = int(json_integer_value(j));
			if (nw < 0) nw = 0;
			if (nw >= omnisfear::MotiveEngine::MEMORY_CAP) nw = 0;
			engine.history.nextWrite = nw;
		}

		// Force resync with next clock — old tick counters are no longer valid.
		engine.currentTick        = -1;
		engine.timeSinceLastTick  = 0.f;
		engine.replayCursor       = 0;
		engine.replayGateOffPos   = -1.f;
		engine.latchedGate        = 0.f;
		engine.latchedAccent      = 0.f;
	}
};

// NanoSVG (used by Rack for panel SVGs) does not render <text>. All labels are drawn here via NanoVG.
struct MotivePanelText : Widget {
	struct Label {
		float xMm, yMm;
		float sizeMm;
		float letterSpacingMm;
		int   align;
		NVGcolor color;
		const char* fontPath;
		std::string text;
	};

	std::vector<Label> labels;
	std::string boldFont;
	std::string monoFont;

	MotivePanelText(Vec pixelSize) {
		box.pos = Vec(0, 0);
		box.size = pixelSize;
		boldFont = asset::system("res/fonts/Nunito-Bold.ttf");
		monoFont = asset::system("res/fonts/ShareTechMono-Regular.ttf");

		const NVGcolor cTitle   = nvgRGB(0xe8, 0xe6, 0xdd);
		const NVGcolor cLabel   = nvgRGB(0xc8, 0xc8, 0xbd);
		const NVGcolor cInHdr   = nvgRGB(0x9a, 0xb0, 0xc4);
		const NVGcolor cOutHdr  = nvgRGB(0xd0, 0x6a, 0x38);
		const NVGcolor cIn      = nvgRGB(0x9a, 0xa8, 0xba);
		const NVGcolor cOut     = nvgRGB(0xe4, 0xc4, 0xa8);
		const NVGcolor cMod     = nvgRGB(0x8c, 0x9a, 0xa8);
		const NVGcolor cSpl     = nvgRGB(0xd0, 0x6a, 0x38);

		const int centerBase = NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE;

		labels.push_back({45.72f, 17.5f, 7.5f, 0.60f, centerBase, cTitle,   boldFont.c_str(), "MOTIVE"});

		labels.push_back({18.00f, 39.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "MEMORY"});
		labels.push_back({45.72f, 39.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "MUTATION"});
		labels.push_back({73.44f, 39.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "FOLLOW"});
		labels.push_back({32.00f, 63.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "DENSITY"});
		labels.push_back({59.44f, 63.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "VARIATION"});

		labels.push_back({14.44f, 85.0f, 2.0f, 0.35f, centerBase, cMod,     monoFont.c_str(), "MUT"});
		labels.push_back({30.08f, 85.0f, 2.0f, 0.35f, centerBase, cMod,     monoFont.c_str(), "VAR"});
		labels.push_back({45.72f, 85.0f, 2.0f, 0.35f, centerBase, cMod,     monoFont.c_str(), "DEN"});
		labels.push_back({61.36f, 85.0f, 2.0f, 0.35f, centerBase, cSpl,     monoFont.c_str(), "SPL"});
		labels.push_back({77.00f, 85.0f, 2.0f, 0.35f, centerBase, cSpl,     monoFont.c_str(), "PREV"});

		labels.push_back({45.72f, 93.5f, 3.0f, 3.20f, centerBase, cInHdr,   boldFont.c_str(), "IN"});
		labels.push_back({12.00f, 105.0f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "CLK"});
		labels.push_back({34.00f, 105.0f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "CV"});
		labels.push_back({57.44f, 105.0f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "GATE"});
		labels.push_back({79.44f, 105.0f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "RST"});

		labels.push_back({45.72f, 113.5f, 3.0f, 3.20f, centerBase, cOutHdr, boldFont.c_str(), "OUT"});
		labels.push_back({12.00f, 123.5f, 2.0f, 0.35f, centerBase, cOut,    monoFont.c_str(), "CV"});
		labels.push_back({34.00f, 123.5f, 2.0f, 0.35f, centerBase, cOut,    monoFont.c_str(), "GATE"});
		labels.push_back({57.44f, 123.5f, 2.0f, 0.35f, centerBase, cOut,    monoFont.c_str(), "ACC"});
		labels.push_back({79.44f, 123.5f, 2.0f, 0.35f, centerBase, cOut,    monoFont.c_str(), "PHR"});
	}

	void draw(const DrawArgs& args) override {
		for (const Label& l : labels) {
			std::shared_ptr<Font> font = APP->window->loadFont(l.fontPath);
			if (!font)
				continue;
			nvgFontFaceId(args.vg, font->handle);
			nvgFontSize(args.vg, mm2px(Vec(l.sizeMm, 0)).x);
			nvgTextAlign(args.vg, l.align);
			nvgTextLetterSpacing(args.vg, mm2px(Vec(l.letterSpacingMm, 0)).x);
			nvgFillColor(args.vg, l.color);
			Vec p = mm2px(Vec(l.xMm, l.yMm));
			nvgText(args.vg, p.x, p.y, l.text.c_str(), NULL);
		}
	}
};

struct MotiveWidget : ModuleWidget {
	MotiveWidget(Motive* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Motive.svg")));

		addChild(new MotivePanelText(box.size));
		addChild(new PngImage(Vec(32.72f, 4.6f), Vec(26.0f, 2.2f),
			asset::plugin(pluginInstance, "res/wordmark.png")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Knobs — row 1: MEMORY, MUTATION, FOLLOW
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(18.00, 28.0)), module, Motive::MEMORY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(45.72, 28.0)), module, Motive::MUTATION_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(73.44, 28.0)), module, Motive::FOLLOW_PARAM));

		// Knobs — row 2: DENSITY, VARIATION
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.00, 52.0)), module, Motive::DENSITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(59.44, 52.0)), module, Motive::VARIATION_PARAM));

		// CV / MOD row — MUT, VAR, DEN, SPL, PREV
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.44, 77.0)), module, Motive::MUT_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.08, 77.0)), module, Motive::VAR_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(45.72, 77.0)), module, Motive::DEN_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(61.36, 77.0)), module, Motive::SPLASH_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(77.00, 77.0)), module, Motive::PREV_INPUT));

		// Inputs — CLOCK, CV, GATE, RESET
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.00, 97.0)), module, Motive::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.00, 97.0)), module, Motive::CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(57.44, 97.0)), module, Motive::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(79.44, 97.0)), module, Motive::RESET_INPUT));

		// Outputs — CV, GATE, ACCENT, PHRASE
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.00, 117.0)), module, Motive::CV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.00, 117.0)), module, Motive::GATE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(57.44, 117.0)), module, Motive::ACCENT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(79.44, 117.0)), module, Motive::PHRASE_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Motive* m = dynamic_cast<Motive*>(module);
		if (!m) return;

		menu->addChild(new MenuSeparator);

		menu->addChild(createMenuItem("New seed", "", [m]() {
			m->engine.rng.seed(random::u64());
		}));
		menu->addChild(createMenuItem("Recapture from input", "", [m]() {
			m->engine.reset();
		}));
		menu->addChild(createMenuItem("Clear memory", "", [m]() {
			m->engine.clearMemory();
		}));
	}
};

Model* modelMotive = createModel<Motive, MotiveWidget>("Motive");
