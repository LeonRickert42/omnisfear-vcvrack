#include "plugin.hpp"

// OMNISFEAR MOTIVE — v0.1 skeleton
// Only params, I/O, and JSON stubs. No DSP behavior yet.
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

struct MotiveWidget : ModuleWidget {
	MotiveWidget(Motive* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Motive.svg")));

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
