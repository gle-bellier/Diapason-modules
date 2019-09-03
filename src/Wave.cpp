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
		configParam(PITCH_PARAM, -1.f, 1.f, 0.f, "Pitch", " Hz", 2.f, dsp::FREQ_C4);
		configParam(SHAPE_PARAM, 0.f, 10.f, 0.f, "Shape", "Type");
		configParam(DIST_PARAM, 0.f, 10.f, 0.f, "Dist", " Amp");
		configParam(AN_PARAM, 0.f, 1.f, 0.f, "Analog mode");
		configParam(FM_PARAM, 0.f, 1.f, 0.f, "FM modulation");
	}
	void process(const ProcessArgs &args) override {
		// get value of parameter
		float frequency = params[PITCH_PARAM].getValue();
		float shape = params[SHAPE_PARAM].getValue();
		float distType = params[AN_PARAM].getValue();
		float distAmount = params[DIST_PARAM].getValue(); //
		float fmAmount = params[FM_PARAM].getValue();

		float pitch = inputs[PITCH_INPUT].getVoltage();
		float shapeCV = inputs[CVSHAPE_INPUT].getVoltage();
		float distCV = inputs[CVDIST_INPUT].getVoltage();
		float fmCV = inputs[CVFM_INPUT].getVoltage();
		float fmIn = inputs[CVFM_INPUT].getVoltage();


		// Compute the frequency from the pitch parameter and input


		// The default pitch is C4 = 261.6256f
		pitch +=frequency;
		float freq = dsp::FREQ_C4 * std::pow(2.f, pitch);

		// Accumulate the phase
		phase += freq * args.sampleTime;
		if (phase >= 0.5f)
			phase -= 1.f;

		// Compute the sine output and shape parameter
		float outSignal = 5*std::sin(2.f * M_PI * phase);
		shape = 0.5f*clamp(shape + shapeCV - 5.f, 0.f,10.f); // -5 because of uni mode LFO delivering 0-10
		outSignal+= shape*std::sin(1.f * M_PI * phase);
		outSignal = outSignal * (1.f/(1.f + shape/5.f)); // in order to normalize the output to -5V/5V

		//want to add some distorsion

		distAmount = 0.5f*clamp(distAmount+distCV,0.f,10.f); // distAmount included between 0 and 5
		float thold = 5.f-(5.f/3.f)*distAmount; // times (5/3) because we want to use it to make pulse width parameter

		float d = abs(outSignal);
		if (d >= thold){
			if (distType>=1.f){
				outSignal = 5.f * outSignal/d;
			}
			else{
				outSignal = (outSignal/d) * (4.5f  + 0.5f * std::sin(5.f * M_PI * phase));
			}
		}
		if (thold<0){
			float pulseWidth = 0.6f-0.20f*distAmount;
			outSignal = simd::ifelse(phase < pulseWidth, abs(outSignal), -abs(outSignal));
		}
		outputs[SINE_OUTPUT].setVoltage(outSignal);

		//Blink light at 1Hz
		blinkPhase += args.sampleTime;
		if (blinkPhase >= 1.f)
			blinkPhase -= 1.f;
		lights[BLINK_LIGHT].setBrightness(blinkPhase < 0.5f ? 1.f : 0.f);
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