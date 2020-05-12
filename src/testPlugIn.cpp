#include "plugin.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <math.h>



class filter {
	protected :
	float buffers[4]; // Order : X1,X2,Y1,Y2
	float coeffs[5];// order : a0, a1, a2, b1, b2
	float thetaC;
	void bufferize(float, float);
	void setCutoff(float,float);
};
void filter::bufferize(float in, float out){
	buffers[1]=buffers[0];
	buffers[0]=in;
	buffers[3]=buffers[2];
	buffers[2]=out;
}
void filter::setCutoff(float frequency, float tSampling){
		thetaC = 2*M_PI*frequency*tSampling;
}

class lpFilter2 : public filter{
	public :
	float process(float, float, float, float);
};
float lpFilter2::process (float X0, float freq, float qFactor,float Tn){
			setCutoff(freq,Tn);
			float d = 1.f/qFactor;
			float fBetaNumerator = 1.f - 0.5f*d*std::sin(thetaC);
			float fBetaDenominator = 1.f + 0.5f*d*std::sin(thetaC);
			float fBeta = 0.5f*(fBetaNumerator/fBetaDenominator);
			float fGamma = (0.5f + fBeta)*std::cos(thetaC);
			float fAlpha = (0.5f + fBeta - fGamma)/2.0f;
			coeffs[0] = fAlpha;
			coeffs[1] = 2.f*fAlpha;
			coeffs[2] = fAlpha;
			coeffs[3] = -2.f*fGamma;
			coeffs[4] = 2.f*fBeta;
			float output = coeffs[0]*X0 + coeffs[1]*buffers[0] + coeffs[2]*buffers[1] - coeffs[3]*buffers[2] - coeffs[4]*buffers[3];
			bufferize(X0,output);
			return output;
		}

class hpFilter2 : public filter{
	public :
	float process(float, float, float, float);
};
float hpFilter2::process (float X0, float freq, float qFactor,float Tn){
			setCutoff(freq,Tn);
			float d = 1.f/qFactor;
			float fBetaNumerator = 1.f - 0.5f*d*std::sin(thetaC);
			float fBetaDenominator = 1.f + 0.5f*d*std::sin(thetaC);
			float fBeta = 0.5f*(fBetaNumerator/fBetaDenominator);
			float fGamma = (0.5f + fBeta)*std::cos(thetaC);
			float fAlpha = (0.5f + fBeta + fGamma)/2.0f;
			coeffs[0] = fAlpha;
			coeffs[1] = -2.f*fAlpha;
			coeffs[2] = fAlpha;
			coeffs[3] = -2.f*fGamma;
			coeffs[4] = 2.f*fBeta;
			float output = coeffs[0]*X0 + coeffs[1]*buffers[0] + coeffs[2]*buffers[1] - coeffs[3]*buffers[2] - coeffs[4]*buffers[3];
			bufferize(X0,output);
			return output;
		}

class bpFilter2 : public filter{
	public :
	float process(float, float, float, float);
};
float bpFilter2::process (float X0, float freq, float qFactor,float Tn){
			setCutoff(freq,Tn);
			float fBetaNumerator = 1.f - std::tan(simd::clamp(thetaC/(2.f*qFactor),0.f,3.14f/2.f));
			float fBetaDenominator = 1.f + std::tan(simd::clamp(thetaC/(2.f*qFactor),0.f,3.14f/2.f));
			float fBeta = 0.5f*(fBetaNumerator/fBetaDenominator);
			float fGamma = (0.5f + fBeta)*std::cos(thetaC);
			float fAlpha = 0.5f - fBeta;
			coeffs[0] = fAlpha;
			coeffs[1] = 0.f;
			coeffs[2] = -fAlpha;
			coeffs[3] = -2.f*fGamma;
			coeffs[4] = 2.f*fBeta;
			float output = coeffs[0]*X0 + coeffs[1]*buffers[0] + coeffs[2]*buffers[1] - coeffs[3]*buffers[2] - coeffs[4]*buffers[3];
			bufferize(X0,output);
			return output;
		}

class bsFilter2 : public filter{
	public :
	float process(float, float, float, float);
};
float bsFilter2::process (float X0, float freq, float qFactor,float Tn){
			setCutoff(freq,Tn);
			float fBetaNumerator = 1.f - std::tan(simd::clamp(thetaC/(2.f*qFactor),0.f,3.14f/2.f));
			float fBetaDenominator = 1.f + std::tan(simd::clamp(thetaC/(2.f*qFactor),0.f,3.14f/2.f));
			float fBeta = 0.5f*(fBetaNumerator/fBetaDenominator);
			float fGamma = (0.5f + fBeta)*std::cos(thetaC);
			float fAlpha = 0.5f + fBeta;
			coeffs[0] = fAlpha;
			coeffs[1] = -2.f*fGamma;
			coeffs[2] = fAlpha;
			coeffs[3] = -2.f*fGamma;
			coeffs[4] = 2.f*fBeta;
			float output = coeffs[0]*X0 + coeffs[1]*buffers[0] + coeffs[2]*buffers[1] - coeffs[3]*buffers[2] - coeffs[4]*buffers[3];
			bufferize(X0,output);
			return output;
		}

class lpFilter2Butterworth : public filter{
	public :
	float process(float, float, float);
};
float lpFilter2Butterworth::process (float X0,float freq,float Tn){
			setCutoff(freq,Tn);
			float C = 1.f / std::tan(simd::clamp(thetaC/2.f,0.f,3.14f/2.f));
			float fAlpha = 1.f / (1.f + simd::pow(2.f, 0.5f)*C + simd::pow(C,2.f));
			coeffs[0] = fAlpha;
			coeffs[1] = 2.f*fAlpha;
			coeffs[2] = fAlpha;
			coeffs[3] = 2.f*fAlpha*(1.f - simd::pow(C,2.f));
			coeffs[4] = fAlpha*(1.f - simd::pow(2.f, 0.5f)*C + simd::pow(C,2.f));
			float output = coeffs[0]*X0 + coeffs[1]*buffers[0] + coeffs[2]*buffers[1] - coeffs[3]*buffers[2] - coeffs[4]*buffers[3];
			bufferize(X0,output);
			return output;
		}

class hpFilter2Butterworth : public filter{
	public :
	float process(float, float, float);
};
float hpFilter2Butterworth::process (float X0,float freq,float Tn){
			setCutoff(freq,Tn);
			float C = std::tan(simd::clamp(thetaC/2.f,0.f,3.14f/2.f));
			float fAlpha = 1.f / (1.f + simd::pow(2.f, 0.5f)*C + simd::pow(C,2.f));
			coeffs[0] = fAlpha;
			coeffs[1] = -2.f*fAlpha;
			coeffs[2] = fAlpha;
			coeffs[3] = -2.f*fAlpha*(1.f - simd::pow(C,2.f));
			coeffs[4] = fAlpha*(1.f - simd::pow(2.f, 0.5f)*C + simd::pow(C,2.f));
			float output = coeffs[0]*X0 + coeffs[1]*buffers[0] + coeffs[2]*buffers[1] - coeffs[3]*buffers[2] - coeffs[4]*buffers[3];
			bufferize(X0,output);
			return output;
		}

class lpFilter2Massberg : public filter{
	public :
	float process(float, float, float, float);
};
float lpFilter2Massberg::process (float X0, float freq, float qFactor,float Tn){
			setCutoff(freq,Tn);
			qFactor /= 10.f;
			std::cout <<qFactor <<'\n';
			float Qp; float Qz;float Ws;
			float g1 = 2.f/simd::pow(simd::pow(2.f - (2.f*M_PI*M_PI)/(thetaC*thetaC),2.f)+simd::pow((2*M_PI)/(qFactor*thetaC),2.f), 0.5f);
			if (qFactor>simd::pow(0.5f,0.5f)){
				float gr = (2*simd::pow(qFactor,2.f))/simd::pow(4.f*qFactor*qFactor-1.f,0.5f);
				float wr = thetaC*simd::pow(1.f-1.f/(4.f*simd::pow(qFactor,2.f)),0.5f);
				float Wr = tan(wr/2.f);
				Ws = Wr*simd::pow((gr*gr-g1*g1)/(gr*gr-1.f),0.25f);

				float wp = 2.f * atan(Ws);
				float wz = 2.f * atan(Ws/simd::pow(g1,0.5f));
				float gp = 1.f/(simd::pow(1.f - simd::pow(wp/thetaC,2.f)+simd::pow(wp/(qFactor*thetaC),2.f), 0.5f));
				float gz = 1.f/(simd::pow(1.f - simd::pow(wz/thetaC,2.f)+simd::pow(wz/(qFactor*thetaC),2.f), 0.5f));

				Qp = simd::pow(g1*(gp*gp-gz*gz)/((g1+gz*gz)*simd::pow(g1-1.f,2.f)),0.5f);
				Qz = simd::pow(g1*g1*(gp*gp-gz*gz)/(gz*gz*(g1+gp*gp)*simd::pow(g1-1.f,2.f)),0.5f);


			} else {
				float wm = thetaC*simd::pow(((2.f-(1.f/(qFactor*qFactor)) + simd::pow(((1.f-4.f*qFactor*qFactor)/simd::pow(qFactor,4.f))+4.f/g1,0.5f))/2.f),0.5f);
				float Wm = tan(wm/2.f);
				Ws = thetaC*simd::pow(1.f-g1*g1,0.25f)/2.f;
				Ws = 	std::min(Ws,Wm);

				float wp = 2.f * atan(Ws);
				float wz = 2.f * atan(Ws/simd::pow(g1,0.5f));
				float gp = 1.f/(simd::pow(1.f - simd::pow(wp/thetaC,2.f)+simd::pow(wp/(qFactor*thetaC),2.f), 0.5f));
				float gz = 1.f/(simd::pow(1.f - simd::pow(wz/thetaC,2.f)+simd::pow(wz/(qFactor*thetaC),2.f), 0.5f));

				Qp = simd::pow(g1*(gp*gp-gz*gz)/((g1+gz*gz)*simd::pow(g1-1.f,2.f)),0.5f);
				Qz = simd::pow(g1*g1*(gp*gp-gz*gz)/(gz*gz*(g1+gp*gp)*simd::pow(g1-1.f,2.f)),0.5f);
			}

			float fGamma = Ws*Ws + (1/Qp)*Ws + 1.f;


			coeffs[0] = (Ws*Ws + (simd::pow(g1,0.5f)*Ws)/Qz + g1)/fGamma;

			coeffs[1] = 2.f*(Ws*Ws-g1)/fGamma;
			coeffs[2] = (Ws*Ws - simd::pow(g1,0.5f)*Ws/Qz + g1)/fGamma;
			coeffs[3] = 2.f*(Ws*Ws-1.f)/fGamma;
			coeffs[4] = (Ws*Ws - Ws/Qp +1.f)/fGamma;

			float output = coeffs[0]*X0 + coeffs[1]*buffers[0] + coeffs[2]*buffers[1] - coeffs[3]*buffers[2] - coeffs[4]*buffers[3];
			bufferize(X0,output);
			return output;
		}











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
	lpFilter2Massberg filterlp;
	hpFilter2Butterworth filterhp;
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
		float outputhp = filterhp.process(X0,freq,args.sampleTime);
		outputs[OUTPUTHP].setVoltage(simd::clamp(outputhp,-5.f,5.f));
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
		addOutput(createOutput<PJ301MPort>(Vec(98, 320), module, Test::OUTPUTHP));
		//addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, Test::BLINK_LIGHT));
	}
};


// Define the Model with the Module type, ModuleWidget type, and module slug
Model *modelTest = createModel<Test, TestWidget>("Test");
