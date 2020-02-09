#include "plugin.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>



struct lpFilter2 {

	float bX1=0;
	float bX2=0;
	float bY1=0;
	float bY2=0;

	float process (float X0, float freq, float qFactor,float Tn){
			//setCutoff(freq,Tn);
			float thetaC = 2*M_PI*freq*Tn;
			std::cout << "Freq = " << freq << '\n';
			float q = qFactor;
			float d = 1.f/q;
			float fBetaNumerator = 1.f - 0.5f*d*std::sin(thetaC);
			float fBetaDenominator = 1.f + 0.5f*d*std::sin(thetaC);
			float fBeta = 0.5f*(fBetaNumerator/fBetaDenominator);
			float fGamma = (0.5f + fBeta)*std::cos(thetaC);
			float fAlpha = (0.5f + fBeta - fGamma)/2.0f;

			float coeff0 = fAlpha;
			float coeff1 = 2.f*fAlpha;
			float coeff2 = fAlpha;
			float coeff3 = -2.f*fGamma;
			float coeff4 = 2.f*fBeta;

			std::cout << "Input = "<< X0 << '\n';
			float output = coeff0*X0 + coeff1*bX1 + coeff2*bX2 - coeff3*bY1 - coeff4*bY2;
			//float output = coeff0*X0 + coeff1*buffers[0] + coeff2*buffers[1] - coeff3*buffers[2] - coeff4*buffers[3];
			std::cout << "Output = " << output << '\n';
			// bufferize(X0,output);
			bX2=bX1;
			bX1=X0;
			bY2=bY1;
			bY1=output;

			return output;
		}
};


struct Test : Module {
	enum ParamId {
		QFACTOR,
		FREQUENCY,
		NUM_PARAMS
	};
	enum InputId {
		INPUT,
		NUM_INPUTS
	};
	enum OutputId {
		OUTPUTLP,
		OUTPUTHP,
		NUM_OUTPUTS
	};
	enum LightId {
		//BLINK_LIGHT,
		NUM_LIGHTS
	};
	float phase = 0.f;
	lpFilter2 filterlp;

	Test() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FREQUENCY, 0.f, 1.f, 0.5f, "Frequency", " Hz", std::pow(2, 10.f), dsp::FREQ_C4 / std::pow(2, 5.f));
		configParam(QFACTOR, 0.01f, 1.f, 0.5f, "Amount", "");
	}
	void process(const ProcessArgs &args) override {
		// get value of parameter
		float resonance = params[QFACTOR].getValue();
		float q = simd::pow(resonance, 2) * 10.f;
		float pitch = params[FREQUENCY].getValue();
		pitch = pitch * 10.f - 5.f;
		float freq = dsp::FREQ_C4 * simd::pow(2.f, pitch);
		float X0 = inputs[INPUT].getVoltage() + 1e-6f * (2.f * random::uniform() - 1.f);;
		float outputlp = filterlp.process(X0,freq,q,args.sampleTime);
		outputs[OUTPUTLP].setVoltage(simd::clamp(outputlp,-5.f,5.f));
	}
};




struct TestWidget : ModuleWidget {
	TestWidget(Test *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/testPanel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<RoundHugeBlackKnob>(Vec(49, 55), module, Test::FREQUENCY));
		addParam(createParam<RoundHugeBlackKnob>(Vec(49, 155), module, Test::QFACTOR));

		addInput(createInput<PJ301MPort>(Vec(28, 320), module, Test::INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(68, 320), module, Test::OUTPUTLP));

		//addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, Test::BLINK_LIGHT));
	}
};


// Define the Model with the Module type, ModuleWidget type, and module slug
Model *modelTest = createModel<Test, TestWidget>("Test");
