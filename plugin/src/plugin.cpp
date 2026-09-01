#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	p->addModel(modelMotive);
	p->addModel(modelSplash);
}
