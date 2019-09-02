#include "plugin.hpp"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

struct Wave : Module {
	enum ParamId {
		PITCH_PARAM,
		DIST_PARAM,
		SHAPE_PARAM,
		FM_PARAM,
		AN_PARAM,
		NUM_PARAMS

	};
	enum InputId {
		PITCH_INPUT,
		CVDIST_INPUT,
		CVSHAPE_INPUT,
		FM_INPUT,
		CVFM_INPUT,
		NUM_INPUTS
	};
	enum OutputId {
		SINE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightId {
		BLINK_LIGHT,
		NUM_LIGHTS
	};

	float phase = 0.f;
	float blinkPhase = 0.f;




	Wave() {
		// Configure the module
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// Configure parameters
		// See engine/Param.hpp for config() arguments
		//configParam(PITCH_PARAM, -3.f, 3.f, 0.f, "Pitch", " Hz", 2.f, dsp::FREQ_C4);
		configParam(SHAPE_PARAM, 0.f, 1.f, 0.f, "Shape", "Type");
		configParam(DIST_PARAM, 0.f, 1.f, 0.f, "Dist", " Amp");
		configParam(AN_PARAM, 0.f, 1.f, 0.f, "Analog mode");


	}

	void process(const ProcessArgs &args) override {
		// Implement a simple sine oscillator

		// Compute the frequency from the pitch parameter and input
		float pitch = params[PITCH_PARAM].getValue();
		pitch += inputs[PITCH_INPUT].getVoltage();
		pitch = clamp(pitch, -4.f, 4.f);
		// The default pitch is C4 = 261.6256f
		float freq = dsp::FREQ_C4/4.f * std::pow(2.f, pitch);

		// Accumulate the phase
		phase += freq * args.sampleTime;
		if (phase >= 0.5f)
			phase -= 1.f;

		float octAmp = params[SHAPE_PARAM].getValue();
		float cvShape = inputs[CVSHAPE_INPUT].getVoltage();
		float dist = 5.f - params[DIST_PARAM].getValue();
		float cvDist = inputs[CVDIST_INPUT].getVoltage();
		float analogique = params[AN_PARAM].getValue();

		// Compute the sine output
		octAmp = clamp(octAmp+cvShape,0.f,1.f);
		dist = clamp(dist+cvDist,0.f,5.f);
		float sine = std::sin(2.f * M_PI * phase) +  octAmp * std::sin(1.f * M_PI * phase);
		// Audio signals are typically +/-5V
		// https://vcvrack.com/manual/VoltageStandards.html
		sine = 5.f * sine * (1.f/(1.f + octAmp));
		float d = (abs(sine)+2.f)*2.f;
		if (d >=dist){
			if (analogique>=1.f){
				sine = 5.f * sine/d;
			}
			else{
				sine = (sine/d) * (5.f  + 0.5f * std::sin(5.f * M_PI * phase) * (1-analogique));
			}

		}

		outputs[SINE_OUTPUT].setVoltage(sine);

		// Blink light at 1Hz
		blinkPhase += args.sampleTime;
		if (blinkPhase >= 1.f)
			blinkPhase -= 1.f;
		lights[BLINK_LIGHT].setBrightness(blinkPhase < 0.5f ? 1.f : 0.f);
	}

	// For more advanced Module features, see engine/Module.hpp in the Rack API.
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize: implements custom behavior requested by the user
};


struct WaveWidget : ModuleWidget {
	WaveWidget(Wave *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Wave.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<Rogan2PWhite>(Vec(85, 310), module, Wave::PITCH_PARAM));
		addParam(createParam<Rogan2PWhite>(Vec(30, 85), module, Wave::SHAPE_PARAM));
		addParam(createParam<Rogan2PWhite>(Vec(30, 190), module, Wave::DIST_PARAM));
		addParam(createParam<Rogan2PWhite>(Vec(85, 250), module, Wave::FM_PARAM));
		addParam(createParam<CKSS>(Vec(43.5f, 271), module, Wave::AN_PARAM));


		addInput(createInput<PJ301MPort>(Vec(135, 100), module, Wave::CVSHAPE_INPUT));
		addInput(createInput<PJ301MPort>(Vec(135, 180), module, Wave::CVDIST_INPUT));
		addInput(createInput<PJ301MPort>(Vec(32, 335), module, Wave::PITCH_INPUT));
		addInput(createInput<PJ301MPort>(Vec(134, 255), module, Wave::FM_INPUT));
		addInput(createInput<PJ301MPort>(Vec(134, 295), module, Wave::CVFM_INPUT));

		addOutput(createOutput<PJ301MPort>(Vec(134, 335), module, Wave::SINE_OUTPUT));

		addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, Wave::BLINK_LIGHT));
	}
};


// Define the Model with the Module type, ModuleWidget type, and module slug
Model *modelWave = createModel<Wave, WaveWidget>("Wave");
