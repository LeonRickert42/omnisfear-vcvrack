#include "plugin.hpp"
#include "PngImage.hpp"

// OMNISFEAR SPLASH — v0.1 performance-gesture module.
// Big button + ENERGY/CHAOS/COLOR knobs. Emits a trigger + two CVs
// intended to plug into MOTIVE's SPL / MUT-CV / VAR-CV inputs.

struct Splash : Module {
	enum ParamId {
		ENERGY_PARAM,
		CHAOS_PARAM,
		COLOR_PARAM,
		SPLASH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIG_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		TRIG_OUTPUT,
		MUT_CV_OUTPUT,
		VAR_CV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		SPLASH_LIGHT,
		LIGHTS_LEN
	};

	dsp::SchmittTrigger buttonTrig;
	dsp::SchmittTrigger extTrig;
	dsp::PulseGenerator outPulse;
	float lightBrightness = 0.f;

	Splash() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(ENERGY_PARAM, 0.f, 1.f, 0.6f, "Energy", "%", 0.f, 100.f);
		configParam(CHAOS_PARAM,  0.f, 1.f, 0.5f, "Chaos",  "%", 0.f, 100.f);
		configParam(COLOR_PARAM,  0.f, 1.f, 0.5f, "Color",  "%", 0.f, 100.f);
		configButton(SPLASH_PARAM, "Splash");

		configInput(TRIG_INPUT,       "Splash trigger");
		configOutput(TRIG_OUTPUT,     "Splash pulse");
		configOutput(MUT_CV_OUTPUT,   "Mutation CV");
		configOutput(VAR_CV_OUTPUT,   "Variation CV");
	}

	void process(const ProcessArgs& args) override {
		bool btnEdge = buttonTrig.process(params[SPLASH_PARAM].getValue());
		bool extEdge = extTrig.process(inputs[TRIG_INPUT].getVoltage(), 0.1f, 1.f);

		if (btnEdge || extEdge) {
			outPulse.trigger(1e-2f);
			lightBrightness = 1.f;
		}

		float energy = params[ENERGY_PARAM].getValue();
		float chaos  = params[CHAOS_PARAM].getValue();

		outputs[TRIG_OUTPUT].setVoltage(outPulse.process(args.sampleTime) ? 10.f : 0.f);
		outputs[MUT_CV_OUTPUT].setVoltage(energy * 10.f);
		outputs[VAR_CV_OUTPUT].setVoltage(chaos  * 10.f);

		lightBrightness -= args.sampleTime * 3.f;
		if (lightBrightness < 0.f) lightBrightness = 0.f;
		lights[SPLASH_LIGHT].setBrightness(lightBrightness);
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

		const int centerBase = NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE;

		labels.push_back({25.40f, 14.0f, 6.5f, 0.60f, centerBase, cTitle,  boldFont.c_str(), "SPLASH"});

		labels.push_back({10.50f, 36.5f, 2.3f, 0.55f, centerBase, cLabel,  boldFont.c_str(), "ENERGY"});
		labels.push_back({40.30f, 36.5f, 2.3f, 0.55f, centerBase, cLabel,  boldFont.c_str(), "CHAOS"});
		labels.push_back({25.40f, 59.5f, 2.3f, 0.55f, centerBase, cLabel,  boldFont.c_str(), "COLOR"});

		labels.push_back({25.40f, 68.5f, 2.4f, 1.20f, centerBase, cBrand,  boldFont.c_str(), "SPLASH"});

		labels.push_back({40.64f, 91.0f, 2.6f, 3.20f, centerBase, cInHdr,  boldFont.c_str(), "IN"});
		labels.push_back({10.50f, 103.0f, 2.0f, 0.35f, centerBase, cIn,    monoFont.c_str(), "TRIG"});

		labels.push_back({40.64f, 111.5f, 2.6f, 3.20f, centerBase, cOutHdr, boldFont.c_str(), "OUT"});
		labels.push_back({10.50f, 123.5f, 2.0f, 0.35f, centerBase, cOut,   monoFont.c_str(), "TRIG"});
		labels.push_back({25.40f, 123.5f, 2.0f, 0.35f, centerBase, cOut,   monoFont.c_str(), "MUT"});
		labels.push_back({40.30f, 123.5f, 2.0f, 0.35f, centerBase, cOut,   monoFont.c_str(), "VAR"});
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
		addChild(new PngImage(Vec(14.4f, 4.6f), Vec(22.0f, 1.86f),
			asset::plugin(pluginInstance, "res/wordmark.png")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.50, 25.0)), module, Splash::ENERGY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40.30, 25.0)), module, Splash::CHAOS_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.40, 48.0)), module, Splash::COLOR_PARAM));

		addParam(createParamCentered<BefacoPush>(mm2px(Vec(25.40, 78.0)), module, Splash::SPLASH_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.40, 85.0)), module, Splash::SPLASH_LIGHT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.50, 97.0)), module, Splash::TRIG_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.50, 117.0)), module, Splash::TRIG_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.40, 117.0)), module, Splash::MUT_CV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(40.30, 117.0)), module, Splash::VAR_CV_OUTPUT));
	}
};

Model* modelSplash = createModel<Splash, SplashWidget>("Splash");
