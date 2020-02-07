#include "plugin.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>



struct HPFilter2 {
	float bX1;
	float bX2;
	float bY1;
	float bY2;
	float q;
	double thetaC;



	void setCutoff(float freq, float Tn){
		thetaC = 2*M_PI*freq*Tn;
	}
	void setQ(float qFactor){
		q = qFactor;
	}

	float process (float X0, float freq, float qFactor,float Tn){
		setCutoff(freq,Tn);
		setQ(qFactor);
		double d2 = 0.5f/q;
		double B = 0.5f*(1-d2*std::sin(thetaC))/(1+d2*std::sin(thetaC));
		double A = (0.5f + B)*std::cos(thetaC);

		float a0 = (0.5 + B + A)/2.0f;
		float a1 = -2.f*a0;
		float a2 = a0;
		float b1 = -2.f*A;
		float b2 = 2.f*B;
		// double a0 = (0.5f + B - A)/2.0f;
		// double a1 = 2.f*a0;
		// double a2 = a0;
		// double b1 = -2.f*A;
		// double b2 = 2.f*B;

		double output = a0*X0 + a1*bX1 + a2*bX2 - b1*bY1 - b2*bY2;
		bX1 = X0;
		bX2 = bX1;
		bY1 = output;
		bY2 = bY1;
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
		OUTPUT,
		NUM_OUTPUTS
	};
	enum LightId {
		//BLINK_LIGHT,
		NUM_LIGHTS
	};
	float phase = 0.f;
	// float bX1 = 0;
	// float bX2 = 0;
	// float bY1 = 0;
	// float bY2 = 0;
	HPFilter2 filter;

	Test() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FREQUENCY, 0.f, 10000.f,2000.f, "Amount", "");
		configParam(QFACTOR, 0.1f, 10.f, 5.f, "Amount", "");
	}
	void process(const ProcessArgs &args) override {
		// get value of parameter
		float q = params[QFACTOR].getValue();
		float freq = params[FREQUENCY].getValue();
		std::cout<< freq;

		// double thetaC = 2*M_PI*freq*args.sampleTime;
		// double d2 = 0.5f/q;
		//
		// // float gamma = std::cos(thetaC)/(1.f+std::sin(thetaC));
		// //
		// // float a0 = (1.f - gamma)/2.0f;
		// // float a1 = a0;
		// // float a2 = 0.f;
		// // float b1 = -gamma;
		// // float b2 = 0.f;
		//
		// double B = 0.5f*(1-d2*std::sin(thetaC))/(1+d2*std::sin(thetaC));
		// double A = (0.5f + B)*std::cos(thetaC);
		//
		//
		// double a0 = (0.5f + B - A)/2.0f;
		// double a1 = 2.f*a0;
		// double a2 = a0;
		// double b1 = -2.f*A;
		// double b2 = 2.f*B;
		//
		//
		// /* HPF
		// float a0 = (0.5 + B + A)/2.0f;
		// float a1 = -2.f*a0;
		// float a2 = a0;
		// float b1 = -2.f*A;
		// float b2 = 2.f*B;
		// */
		double X0 = inputs[INPUT].getVoltage();
		// double output = a0*X0 + a1*bX1 + a2*bX2 - b1*bY1 - b2*bY2;
		//
		// bX1 = X0;
		// bX2 = bX1;
		// bY1 = output;
		// bY2 = bY1;
		float output = filter.process(X0,freq,q,args.sampleTime);
		outputs[OUTPUT].setVoltage(simd::clamp(output,-5.f,5.f));
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
		addOutput(createOutput<PJ301MPort>(Vec(98, 320), module, Test::OUTPUT));

		//addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, Test::BLINK_LIGHT));
	}
};


// Define the Model with the Module type, ModuleWidget type, and module slug
Model *modelTest = createModel<Test, TestWidget>("Test");
