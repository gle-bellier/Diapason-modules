#include "plugin.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>

using simd::float_4;

template <int OVERSAMPLE, int QUALITY, typename T>
struct VCO {
			// Compute the sine output and shape parameter
			int channels = 0;
			T freq;
			T phase = 0.f;
			T anlValue = 0.f;
			T dgtValue = 0.f;
			T outSignal;
			T shape;
			T shapeCV;
			T distAmount;
			T thold;

			void setPitch(T pitch) {
				freq = dsp::FREQ_C4 * simd::pow(2.f, pitch);
			}
			void process(float deltaTime){
					for (int i=0; i<4;i+=1){
						phase[i] += freq[i] * deltaTime;
						if (phase[i] >= 0.5f)
							phase[i] -= 1.f;
						outSignal[i] = 5*std::sin(2.f * M_PI * phase[i]);
						shape[i] = 0.5f*clamp(shape[i] + shapeCV[i] - 5.f, 0.f,10.f); // -5 because of uni mode LFO delivering 0-10
						outSignal[i]+= shape[i]*std::sin(1.f * M_PI * phase[i]);
						outSignal[i] = outSignal[i] * (1.f/(1.f + shape[i]/5.f)); // in order to normalize the output to -5V/5V

				dgtValue[i]=outSignal[i];
				anlValue[i]=outSignal[i];


					float d = abs(outSignal[i]);
					if (d >= thold[i]) 	{
						dgtValue[i] = 5.f * outSignal[i]/d;
						anlValue[i] = (outSignal[i]/d) * (4.4f  + 0.20f * (std::sin(5.f * M_PI * phase[i])+std::sin(7.f * M_PI * phase[i])+std::sin(7.f * M_PI * phase[i])));
					}
					if (thold[i]<0){
						float psPhase = phase[i]+0.5f;
						if (psPhase >= 0.5f)
							psPhase -= 1.f;
						float pulseWidth = 0.6f-0.20f*distAmount[i];
						if (psPhase>=pulseWidth and outSignal[i]>=0){			//modification of output signal when above pulsewidth
							dgtValue[i] = -dgtValue[i];
							anlValue[i] = anlValue[i]-9.f;
						}
					}
				}
			 }

			// T dgt(T phase, T distAmount){
			// 	T v = outSignal;
			// 	return v;
			// }
			//
			// T anl(T phase, T distAmount){
			// 	T v = outSignal;
			//
			// 	return v;
			// }


			T dgt() {
				return dgtValue;
			}
			T anl() {
				return anlValue;
			}
};





struct Wave : Module {
	enum ParamId {
		PITCH_PARAM,
		DIST_PARAM,
		SHAPE_PARAM,
		FINE_PARAM,
		FM_PARAM,
		AN_PARAM,
		NUM_PARAMS

	};
	enum InputId {
		PITCH_INPUT,
		CVDIST_INPUT,
		CVSHAPE_INPUT,
		FM_INPUT,
		NUM_INPUTS
	};
	enum OutputId {
		ANL_OUTPUT,
		DGT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightId {
		//BLINK_LIGHT,
		NUM_LIGHTS
	};
	float phase = 0.f;
	float blinkPhase = 0.f;
	float bValue = 0;



	VCO<16, 16, float_4>oscillators[4];

	Wave() {
		// Configure the module
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		// Configure parameters
		// See engine/Param.hpp for config() arguments
		configParam(PITCH_PARAM, -1.f, 1.f, 0.f, "Pitch", " Hz", 2.f, dsp::FREQ_C4);
		configParam(FINE_PARAM, -100.f, 0.f, 100.f, "Fine","cts");
		configParam(SHAPE_PARAM, 0.f, 20.f, 0.f, "Shape", "Type");
		configParam(DIST_PARAM, 0.f, 10.f, 0.f, "Dist", " Amp");
		configParam(FM_PARAM, 0.f, 1.f, 0.f, "FM modulation");
	}
	void process(const ProcessArgs &args) override {
		// get value of parameter
		float frequency = params[PITCH_PARAM].getValue();
		float fine = params[FINE_PARAM].getValue();
		float fmAmount = params[FM_PARAM].getValue();
		float shape = params[SHAPE_PARAM].getValue();
		float distAmount = params[DIST_PARAM].getValue();




		int channels = std::max(inputs[PITCH_INPUT].getChannels(), 1);

		for (int c = 0; c < channels;c+=4){
			float_4 shapeCV = inputs[CVSHAPE_INPUT].getPolyVoltageSimd<float_4>(c);
			float_4 pitch = inputs[PITCH_INPUT].getPolyVoltageSimd<float_4>(c);
			float_4 fmIn = inputs[FM_INPUT].getPolyVoltageSimd<float_4>(c);
			float_4 cvDist = inputs[CVDIST_INPUT].getPolyVoltageSimd<float_4>(c);

			// float pitch = pitch4[0];
			// float shapeCV = shapeCV4[0];
			// float distCV = cvDist4[0];
			// float fmIn = fmIn4[0];


			auto *oscillator = &oscillators[c / 4];
			oscillator->channels = std::min(channels - c, 4);

			pitch += frequency;
			pitch += fine/1000;
			if (inputs[FM_INPUT].isConnected()) {
				pitch += fmAmount * fmIn;
			}

			// float a = f4shape[0];
			// if (a!=bValue){
			// 		std::cout<<"shape -> "<< a << std::endl;
			// 		bValue = a;
			// }
			float_4 min = {0.f,0.f,0.f,0.f};
			float_4 max = {10.f,10.f,10.f,10.f};
			float_4 distAmount4 = 0.5f*simd::clamp(distAmount+cvDist,min,max); // distAmount included between 0 and 5
			float_4 thold = 5.f-(5.f/3.f)*distAmount; // times (5/3) because we want to use it to make pulse width parameter

			oscillator->shape = shape;
			oscillator->shapeCV = shapeCV;
			oscillator->distAmount = distAmount4;
			oscillator->thold = thold;

			oscillator->setPitch(pitch);
			oscillator->process(args.sampleTime);

			// Set output
			if (outputs[ANL_OUTPUT].isConnected())
				outputs[ANL_OUTPUT].setVoltageSimd(oscillator->anl(), c);
			if (outputs[DGT_OUTPUT].isConnected())
				outputs[DGT_OUTPUT].setVoltageSimd(oscillator->dgt(), c);

		}
		outputs[ANL_OUTPUT].setChannels(channels);
		outputs[DGT_OUTPUT].setChannels(channels);




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

		addParam(createParam<RoundHugeBlackKnob>(Vec(49, 55), module, Wave::PITCH_PARAM));
		addParam(createParam<Rogan1PWhite>(Vec(25, 152), module, Wave::FINE_PARAM));
		addParam(createParam<Rogan1PWhite>(Vec(93, 152), module, Wave::FM_PARAM));

		addParam(createParam<Rogan1PWhite>(Vec(93, 202), module, Wave::SHAPE_PARAM));
		addParam(createParam<Rogan1PWhite>(Vec(25, 202), module, Wave::DIST_PARAM));


		addInput(createInput<PJ301MPort>(Vec(28, 270), module, Wave::CVDIST_INPUT));
		addInput(createInput<PJ301MPort>(Vec(63, 270), module, Wave::FM_INPUT));
		addInput(createInput<PJ301MPort>(Vec(98, 270), module, Wave::CVSHAPE_INPUT));
		addInput(createInput<PJ301MPort>(Vec(28, 320), module, Wave::PITCH_INPUT));

		addOutput(createOutput<PJ301MPort>(Vec(63, 320), module, Wave::DGT_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(98, 320), module, Wave::ANL_OUTPUT));

		//addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, Wave::BLINK_LIGHT));
	}
};


// Define the Model with the Module type, ModuleWidget type, and module slug
Model *modelWave = createModel<Wave, WaveWidget>("Wave");
