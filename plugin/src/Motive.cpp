#include "plugin.hpp"

// OMNISFEAR MOTIVE — v0.1 skeleton
// Only params, I/O, JSON stubs and panel design. No DSP behavior yet.
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
		// TODO: motive engine
		(void) args;
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
		const NVGcolor cSub     = nvgRGB(0xa5, 0xa4, 0x9a);
		const NVGcolor cLabel   = nvgRGB(0xc8, 0xc8, 0xbd);
		const NVGcolor cTag     = nvgRGB(0x5c, 0x5c, 0x56);
		const NVGcolor cSection = nvgRGB(0x8a, 0x88, 0x80);
		const NVGcolor cInHdr   = nvgRGB(0x7a, 0x7a, 0x72);
		const NVGcolor cOutHdr  = nvgRGB(0xc8, 0x58, 0x2a);
		const NVGcolor cIn      = nvgRGB(0xa5, 0xa4, 0x9a);
		const NVGcolor cOut     = nvgRGB(0xd8, 0xd3, 0xc2);

		const int centerBase = NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE;
		const int leftBase   = NVG_ALIGN_LEFT   | NVG_ALIGN_BASELINE;
		const int rightBase  = NVG_ALIGN_RIGHT  | NVG_ALIGN_BASELINE;

		labels.push_back({ 4.00f,  6.9f, 1.6f, 0.15f, leftBase,   cTag,     monoFont.c_str(), "PROJECT OMNISFEAR"});
		labels.push_back({67.12f,  6.9f, 1.6f, 0.15f, rightBase,  cTag,     monoFont.c_str(), "MOTIVE ENGINE"});

		labels.push_back({35.56f, 14.5f, 6.4f, 0.40f, centerBase, cTitle,   boldFont.c_str(), "OMNISFEAR"});
		labels.push_back({35.56f, 21.3f, 2.6f, 1.40f, centerBase, cSub,     monoFont.c_str(), "MOTIVE"});

		labels.push_back({14.00f, 39.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "MEMORY"});
		labels.push_back({35.56f, 39.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "MUTATION"});
		labels.push_back({57.12f, 39.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "FOLLOW"});
		labels.push_back({24.78f, 63.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "DENSITY"});
		labels.push_back({46.34f, 63.5f, 2.3f, 0.55f, centerBase, cLabel,   boldFont.c_str(), "VARIATION"});

		labels.push_back({35.56f, 82.8f, 2.4f, 2.00f, centerBase, cSection, monoFont.c_str(), "SIGNAL"});

		labels.push_back({ 4.00f, 88.8f,  1.9f, 0.50f, leftBase,   cInHdr,  monoFont.c_str(), "IN"});
		labels.push_back({ 9.00f, 103.5f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "CLK"});
		labels.push_back({24.00f, 103.5f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "CV"});
		labels.push_back({39.00f, 103.5f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "GATE"});
		labels.push_back({54.00f, 103.5f, 2.0f, 0.35f, centerBase, cIn,     monoFont.c_str(), "RST"});

		labels.push_back({ 4.00f, 108.8f, 1.9f, 0.50f, leftBase,   cOutHdr, monoFont.c_str(), "OUT"});
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
