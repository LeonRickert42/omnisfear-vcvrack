#include "plugin.hpp"
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

		configOutput(CV_OUTPUT,     "Pitch CV");
		configOutput(GATE_OUTPUT,   "Gate");
		configOutput(ACCENT_OUTPUT, "Accent");
		configOutput(PHRASE_OUTPUT, "Phrase");
	}

	void process(const ProcessArgs& args) override {
		omnisfear::MotiveEngine::Inputs in;
		in.sampleTime = args.sampleTime;
		in.cv         = inputs[CV_INPUT].getVoltage();
		in.clockEdge  = clockTrig.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 1.f);
		in.resetEdge  = resetTrig.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f);

		auto gEvent = gateTrig.processEvent(inputs[GATE_INPUT].getVoltage(), 0.1f, 1.f);
		in.gateHigh     = gateTrig.isHigh();
		in.gateEdgeUp   = (gEvent == dsp::SchmittTrigger::TRIGGERED);
		in.gateEdgeDown = (gEvent == dsp::SchmittTrigger::UNTRIGGERED);

		in.mutation  = params[MUTATION_PARAM].getValue();
		in.variation = params[VARIATION_PARAM].getValue();
		in.density   = params[DENSITY_PARAM].getValue();
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

	json_t* dataToJson() override {
		json_t* root = json_object();
		// TODO: serialize seed, motive memory, phrase state
		return root;
	}

	void dataFromJson(json_t* root) override {
		// TODO: restore seed, motive memory, phrase state
		(void) root;
	}
};

// Small helper: renders a PNG (with alpha) at an mm-positioned box via NanoVG.
struct PngImage : Widget {
	std::string path;

	PngImage(Vec posMm, Vec sizeMm, const std::string& p) {
		box.pos = mm2px(posMm);
		box.size = mm2px(sizeMm);
		path = p;
	}

	void draw(const DrawArgs& args) override {
		std::shared_ptr<Image> img = APP->window->loadImage(path);
		if (!img)
			return;
		NVGpaint paint = nvgImagePattern(args.vg, 0, 0, box.size.x, box.size.y, 0.f, img->handle, 1.f);
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgFillPaint(args.vg, paint);
		nvgFill(args.vg);
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

		const int centerBase = NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE;

		labels.push_back({35.56f, 17.5f, 7.5f, 0.60f, centerBase, cTitle,   boldFont.c_str(), "MOTIVE"});

		labels.push_back({14.00f, 39.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "MEMORY"});
		labels.push_back({35.56f, 39.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "MUTATION"});
		labels.push_back({57.12f, 39.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "FOLLOW"});
		labels.push_back({24.78f, 63.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "DENSITY"});
		labels.push_back({46.34f, 63.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "VARIATION"});

		labels.push_back({35.56f, 84.5f, 3.4f, 3.20f, centerBase, cInHdr,  boldFont.c_str(), "IN"});
		labels.push_back({ 9.00f, 103.5f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "CLK"});
		labels.push_back({24.00f, 103.5f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "CV"});
		labels.push_back({39.00f, 103.5f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "GATE"});
		labels.push_back({54.00f, 103.5f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "RST"});

		labels.push_back({35.56f, 109.5f, 3.4f, 3.20f, centerBase, cOutHdr, boldFont.c_str(), "OUT"});
		labels.push_back({ 9.00f, 123.5f, 2.0f, 0.35f, centerBase, cOut,    monoFont.c_str(), "CV"});
		labels.push_back({24.00f, 123.5f, 2.0f, 0.35f, centerBase, cOut,    monoFont.c_str(), "GATE"});
		labels.push_back({39.00f, 123.5f, 2.0f, 0.35f, centerBase, cOut,    monoFont.c_str(), "ACC"});
		labels.push_back({54.00f, 123.5f, 2.0f, 0.35f, centerBase, cOut,    monoFont.c_str(), "PHR"});
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
		addChild(new PngImage(Vec(22.56f, 4.6f), Vec(26.0f, 2.2f),
			asset::plugin(pluginInstance, "res/wordmark.png")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Knobs — row 1: MEMORY, MUTATION, FOLLOW
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14.0,  28.0)), module, Motive::MEMORY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.56, 28.0)), module, Motive::MUTATION_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(57.12, 28.0)), module, Motive::FOLLOW_PARAM));

		// Knobs — row 2: DENSITY, VARIATION
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(24.78, 52.0)), module, Motive::DENSITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.34, 52.0)), module, Motive::VARIATION_PARAM));

		// Inputs — CLOCK, CV, GATE, RESET
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec( 9.0, 95.0)), module, Motive::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.0, 95.0)), module, Motive::CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(39.0, 95.0)), module, Motive::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(54.0, 95.0)), module, Motive::RESET_INPUT));

		// Outputs — CV, GATE, ACCENT, PHRASE
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec( 9.0, 115.0)), module, Motive::CV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.0, 115.0)), module, Motive::GATE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(39.0, 115.0)), module, Motive::ACCENT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(54.0, 115.0)), module, Motive::PHRASE_OUTPUT));
	}
};

Model* modelMotive = createModel<Motive, MotiveWidget>("Motive");
