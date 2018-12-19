#include "Template.hpp"


struct MyModule : Module {
	enum ParamIds {
		PITCH_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SINE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		BLINK_LIGHT,
		NUM_LIGHTS
	};

	float phase = 0.f;
	float blinkPhase = 0.f;

	MyModule() {
		// Set the number of components
		setup(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// Set parameter settings
		params[PITCH_PARAM].setup(-3.f, 3.f, 0.f, "Pitch", " semi", 0.f, 12.f);
	}
	void step() override;

	// For more advanced Module features, see engine/Module.hpp in the Rack API.
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize: implements custom behavior requested by the user
};


void MyModule::step() {
	// Implement a simple sine oscillator
	float deltaTime = context()->engine->getSampleRate();

	// Compute the frequency from the pitch parameter and input
	float pitch = params[PITCH_PARAM].value;
	pitch += inputs[PITCH_INPUT].value;
	pitch = clamp(pitch, -4.f, 4.f);
	// The default pitch is C4
	float freq = 261.626f * std::pow(2.f, pitch);

	// Accumulate the phase
	phase += freq * deltaTime;
	if (phase >= 1.f)
		phase -= 1.f;

	// Compute the sine output
	float sine = std::sin(2.f * M_PI * phase);
	outputs[SINE_OUTPUT].value = 5.f * sine;

	// Blink light at 1Hz
	blinkPhase += deltaTime;
	if (blinkPhase >= 1.f)
		blinkPhase -= 1.f;
	lights[BLINK_LIGHT].value = (blinkPhase < 0.5f) ? 1.f : 0.f;
}


struct MyModuleWidget : ModuleWidget {
	MyModuleWidget(MyModule *module) : ModuleWidget(module) {
		setPanel(SVG::load(asset::plugin(plugin, "res/MyModule.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<Davies1900hBlackKnob>(Vec(28, 87), module, MyModule::PITCH_PARAM));

		addInput(createInput<PJ301MPort>(Vec(33, 186), module, MyModule::PITCH_INPUT));

		addOutput(createOutput<PJ301MPort>(Vec(33, 275), module, MyModule::SINE_OUTPUT));

		addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, MyModule::BLINK_LIGHT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelMyModule = createModel<MyModule, MyModuleWidget>("MyModule");
