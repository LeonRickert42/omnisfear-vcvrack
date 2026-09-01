#include "plugin.hpp"
#include "PngImage.hpp"

// OMNISFEAR SPLASH — v0.2 performance gesture module.
// Big button + ENERGY/CHAOS/COLOR/MEMORY knobs. Emits TRIG or PREV pulses
// and continuous MUT/VAR/DEN CVs intended for MOTIVE's matching inputs.
// MEMORY: at each splash, with probability MEMORY the pulse becomes a rewind.

struct Splash : Module {
	enum ParamId {
		ENERGY_PARAM,
		CHAOS_PARAM,
		COLOR_PARAM,
		MEMORY_PARAM,
		SPLASH_PARAM,
		REJECT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIG_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		TRIG_OUTPUT,
		PREV_OUTPUT,
		MUT_CV_OUTPUT,
		VAR_CV_OUTPUT,
		DEN_CV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		SPLASH_LIGHT,
		REJECT_LIGHT,
		LIGHTS_LEN
	};

	dsp::SchmittTrigger splBtnTrig;
	dsp::SchmittTrigger rejBtnTrig;
	dsp::SchmittTrigger extTrig;
	dsp::PulseGenerator trigPulse;
	dsp::PulseGenerator prevPulse;
	float splashLight = 0.f;
	float rejectLight = 0.f;

	Splash() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(ENERGY_PARAM, 0.f, 1.f, 0.6f, "Energy", "%", 0.f, 100.f);
		configParam(CHAOS_PARAM,  0.f, 1.f, 0.5f, "Chaos",  "%", 0.f, 100.f);
		configParam(COLOR_PARAM,  0.f, 1.f, 0.5f, "Color (rhythm <-> pitch)", "%", 0.f, 100.f);
		configParam(MEMORY_PARAM, 0.f, 1.f, 0.3f, "Memory (chance a splash rewinds)", "%", 0.f, 100.f);
		configButton(SPLASH_PARAM, "Splash");
		configButton(REJECT_PARAM, "Reject");

		configInput(TRIG_INPUT,     "Splash trigger");
		configOutput(TRIG_OUTPUT,   "Splash pulse");
		configOutput(PREV_OUTPUT,   "Rewind pulse");
		configOutput(MUT_CV_OUTPUT, "Mutation CV");
		configOutput(VAR_CV_OUTPUT, "Variation CV");
		configOutput(DEN_CV_OUTPUT, "Density CV (bipolar)");
	}

	void process(const ProcessArgs& args) override {
		bool splEdge = splBtnTrig.process(params[SPLASH_PARAM].getValue());
		bool extEdge = extTrig.process(inputs[TRIG_INPUT].getVoltage(), 0.1f, 1.f);
		bool rejEdge = rejBtnTrig.process(params[REJECT_PARAM].getValue());

		const float memory = params[MEMORY_PARAM].getValue();

		if (splEdge || extEdge) {
			if (random::uniform() < memory) {
				prevPulse.trigger(1e-2f);
				rejectLight = 1.f;
			} else {
				trigPulse.trigger(1e-2f);
				splashLight = 1.f;
			}
		}
		if (rejEdge) {
			prevPulse.trigger(1e-2f);
			rejectLight = 1.f;
		}

		const float energy = params[ENERGY_PARAM].getValue();
		const float chaos  = params[CHAOS_PARAM].getValue();
		const float color  = params[COLOR_PARAM].getValue();

		outputs[TRIG_OUTPUT].setVoltage(trigPulse.process(args.sampleTime) ? 10.f : 0.f);
		outputs[PREV_OUTPUT].setVoltage(prevPulse.process(args.sampleTime) ? 10.f : 0.f);
		outputs[MUT_CV_OUTPUT].setVoltage(energy * 10.f);
		outputs[VAR_CV_OUTPUT].setVoltage(chaos  * 10.f);
		outputs[DEN_CV_OUTPUT].setVoltage((color - 0.5f) * 10.f);

		const float decay = args.sampleTime * 3.f;
		splashLight -= decay; if (splashLight < 0.f) splashLight = 0.f;
		rejectLight -= decay; if (rejectLight < 0.f) rejectLight = 0.f;
		lights[SPLASH_LIGHT].setBrightness(splashLight);
		lights[REJECT_LIGHT].setBrightness(rejectLight);
	}
};

struct SplashPanelText : Widget {
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

	SplashPanelText(Vec pixelSize) {
		box.pos  = Vec(0, 0);
		box.size = pixelSize;
		boldFont = asset::system("res/fonts/Nunito-Bold.ttf");
		monoFont = asset::system("res/fonts/ShareTechMono-Regular.ttf");

		const NVGcolor cTitle  = nvgRGB(0xe8, 0xe6, 0xdd);
		const NVGcolor cLabel  = nvgRGB(0xc8, 0xc8, 0xbd);
		const NVGcolor cInHdr  = nvgRGB(0x9a, 0xb0, 0xc4);
		const NVGcolor cOutHdr = nvgRGB(0xd0, 0x6a, 0x38);
		const NVGcolor cIn     = nvgRGB(0x9a, 0xa8, 0xba);
		const NVGcolor cOut    = nvgRGB(0xe4, 0xc4, 0xa8);
		const NVGcolor cBrand  = nvgRGB(0xd0, 0x6a, 0x38);
		const NVGcolor cRej    = nvgRGB(0xa8, 0xa8, 0x8c);

		const int centerBase = NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE;

		labels.push_back({30.48f, 14.0f, 6.5f, 0.60f, centerBase, cTitle,  boldFont.c_str(), "SPLASH"});

		labels.push_back({14.00f, 36.5f, 2.3f, 0.55f, centerBase, cLabel,  boldFont.c_str(), "ENERGY"});
		labels.push_back({46.96f, 36.5f, 2.3f, 0.55f, centerBase, cLabel,  boldFont.c_str(), "CHAOS"});
		labels.push_back({14.00f, 56.5f, 2.3f, 0.55f, centerBase, cLabel,  boldFont.c_str(), "COLOR"});
		labels.push_back({46.96f, 56.5f, 2.3f, 0.55f, centerBase, cLabel,  boldFont.c_str(), "MEMORY"});

		labels.push_back({30.48f, 76.5f, 2.0f, 1.20f, centerBase, cBrand,  boldFont.c_str(), "SPLASH"});
		labels.push_back({30.48f, 90.5f, 1.9f, 0.55f, centerBase, cRej,    monoFont.c_str(), "REJECT"});

		labels.push_back({40.64f, 100.5f, 2.4f, 3.20f, centerBase, cInHdr,  boldFont.c_str(), "IN"});
		labels.push_back({10.16f, 103.5f, 1.9f, 0.35f, centerBase, cIn,    monoFont.c_str(), "TRIG"});

		labels.push_back({30.48f, 114.5f, 2.4f, 3.20f, centerBase, cOutHdr, boldFont.c_str(), "OUT"});
		labels.push_back({10.16f, 124.5f, 1.7f, 0.35f, centerBase, cOut,   monoFont.c_str(), "TRG"});
		labels.push_back({20.32f, 124.5f, 1.7f, 0.35f, centerBase, cOut,   monoFont.c_str(), "PRV"});
		labels.push_back({30.48f, 124.5f, 1.7f, 0.35f, centerBase, cOut,   monoFont.c_str(), "MUT"});
		labels.push_back({40.64f, 124.5f, 1.7f, 0.35f, centerBase, cOut,   monoFont.c_str(), "VAR"});
		labels.push_back({50.80f, 124.5f, 1.7f, 0.35f, centerBase, cOut,   monoFont.c_str(), "DEN"});
	}

	void draw(const DrawArgs& args) override {
		for (const Label& l : labels) {
			std::shared_ptr<Font> font = APP->window->loadFont(l.fontPath);
			if (!font) continue;
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

struct SplashWidget : ModuleWidget {
	SplashWidget(Splash* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Splash.svg")));

		addChild(new SplashPanelText(box.size));
		addChild(new PngImage(Vec(19.48f, 4.6f), Vec(22.0f, 1.86f),
			asset::plugin(pluginInstance, "res/wordmark.png")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14.00, 25.0)), module, Splash::ENERGY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.96, 25.0)), module, Splash::CHAOS_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14.00, 45.0)), module, Splash::COLOR_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.96, 45.0)), module, Splash::MEMORY_PARAM));

		addParam(createParamCentered<BefacoPush>(mm2px(Vec(30.48, 68.0)), module, Splash::SPLASH_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(30.48, 78.5)), module, Splash::SPLASH_LIGHT));

		addParam(createParamCentered<TL1105>(mm2px(Vec(30.48, 85.0)), module, Splash::REJECT_PARAM));
		addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(37.5, 85.0)), module, Splash::REJECT_LIGHT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 98.0)), module, Splash::TRIG_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 118.5)), module, Splash::TRIG_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.32, 118.5)), module, Splash::PREV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 118.5)), module, Splash::MUT_CV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(40.64, 118.5)), module, Splash::VAR_CV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.80, 118.5)), module, Splash::DEN_CV_OUTPUT));
	}
};

Model* modelSplash = createModel<Splash, SplashWidget>("Splash");
