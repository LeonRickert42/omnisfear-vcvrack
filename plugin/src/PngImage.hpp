#pragma once
#include "plugin.hpp"

// Renders a PNG (with alpha) at an mm-positioned box via NanoVG.
struct PngImage : Widget {
	std::string path;

	PngImage(Vec posMm, Vec sizeMm, const std::string& p) {
		box.pos  = mm2px(posMm);
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
