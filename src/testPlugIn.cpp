#include "plugin.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>


struct Wave : Module {
	enum ParamId {
		AMOUNT
	};
	enum InputId {
		INPUT
	};
	enum OutputId {
		OUTPUT
	};
	enum LightId {
		//BLINK_LIGHT,
		NUM_LIGHTS
	};
	float phase = 0.f;
	float blinkPhase = 0.f;
	float bValue = 0;


	Wave() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(AMOUNT, -1.f, 1.f, 0.f, "Amount", "");
	}
	void process(const ProcessArgs &args) override {
		// get value of parameter
		float amount = params[AMOUNT].getValue();
		float s = inputs[INPUT].getVoltage();
		outputs[OUTPUT].setVoltage(s);


	}
};




struct WaveWidget : ModuleWidget {
	WaveWidget(Wave *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Wave.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<RoundHugeBlackKnob>(Vec(49, 55), module, Wave::AMOUNT));
		addInput(createInput<PJ301MPort>(Vec(28, 320), module, Wave::INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(98, 320), module, Wave::OUTPUT));

		//addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, Wave::BLINK_LIGHT));
	}
};


// Define the Model with the Module type, ModuleWidget type, and module slug
Model *modelWave = createModel<Wave, WaveWidget>("Wave");
