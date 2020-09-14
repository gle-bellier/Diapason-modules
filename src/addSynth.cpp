#include "plugin.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <math.h>

template <typename T>
T sin2pi_pade_05_5_4(T x) {
	x -= 0.5f;
	return (T(-6.283185307) * x + T(33.19863968) * simd::pow(x, 3) - T(32.44191367) * simd::pow(x, 5))
	       / (1 + T(1.296008659) * simd::pow(x, 2) + T(0.7028072946) * simd::pow(x, 4));
}

class Vco {
public :
    float process(float,float,float);
    void set_frequencies(float,float);
    void set_amount(float);
    void set_pulsewave(float,float);
    void set_filter(float,float,float);
    float frequencies[32] = {};
    float amount[32] = {};
    double sawtooth_coeff[32] = {};
    double square_coeff[32] = {};
    float out_sawtooth=0;
    float out_square=0;
    Vco();

private:
    float filter_emulation(float,float,float,float);
    float filter_lowpass(float,float,float);
    float filter_highpass(float,float,float);
    float filter_bandpass(float,float,float);
};
Vco::Vco(){
    for(int i=0;i<32;i++){
        sawtooth_coeff[i]=(2.f/((i+1.f)*M_PI))*std::pow(-1.f,(i+2.f));
    }
}
float Vco::process (float phase,float shape, float partials){
    shape*=1.95f;
    if (shape>1.f){
        set_pulsewave(1.f - 0.5f*shape, partials);
    } else {
        set_pulsewave(0.5f,partials);
    }
    out_sawtooth=0;
    out_square=0;
    for(int i = 0;i<32;i++){
        float p = (phase-0.25f)*frequencies[i]+0.25f;
        p -= simd::floor(p);
        out_square += amount[i] * square_coeff[i] * sin2pi_pade_05_5_4(p);
    };
    for(int i = 0;i<32;i++){
        float p = phase*frequencies[i];
        p -= simd::floor(p);
        out_sawtooth += amount[i] * sawtooth_coeff[i] * sin2pi_pade_05_5_4(p);
    };

    // if shape ~ 1.95 and partials low we need to add first harmonic. 

    if (shape<1.f) {
        return shape * (2.f * out_square) + (1.f - shape) * out_sawtooth;
    } 
    else /*(shape>1.70f)*/{
        return (2.f*out_square-(shape-1.f))*(1.f - std::exp(-10.f*std::pow(partials+ (1.95f-shape),2.f)));
    }
     
    /*
    else {
        return 2.f*out_square-(shape-1.f);
    }
    */
}


void Vco::set_frequencies(float spread,float detune){
    frequencies[0] = 1;
    for(int i = 1;i<32;i++){
        frequencies[i] = std::pow(i+1.f,1.f+spread) + std::pow(-1.f,i)*std::pow(i+1.f,2.f+detune)*detune ;
    };
}


void Vco::set_amount(float k){
    for(int i = 0;i<32;i++){
        amount[i] = std::exp(-std::pow((float)(i+1) - 1.f,2)/std::pow(k,4.f)); //
    };
}
void Vco::set_pulsewave(float pulsewidth, float partials){
    for(int i=0;i<32;i++){

        float p = (i+1.f)*(pulsewidth)/2.f;
        p -= simd::floor(p);
        square_coeff[i]=(2.f/((i+1.f)*M_PI))*sin2pi_pade_05_5_4(p);
    }
}
float Vco::filter_bandpass(float freq, float freq_cut, float q){
    float b0 = 1.f - 0.5f*std::exp(-1.f+q/15);
    float b1 = std::log(1.f + std::exp(1.f)*q);
    float b2 = std::pow(std::exp(-std::log(1.f+q)*0.125f*q*std::pow(freq-freq_cut,2.f)),2.f);
    float b3 = std::log(1+freq_cut);
    float b4 = 1.f - std::log(1.f + 0.025*q);
    return b0*b1*b2*b3+b4;

}

float Vco::filter_highpass(float freq, float freq_cut, float q){
    if (freq<freq_cut){
        return std::exp(-(1.f+q)*std::pow(freq-freq_cut,2.f));
    } else {
        return 1.f;
    }
}
float Vco::filter_lowpass(float freq, float freq_cut, float q){
     if (freq>freq_cut){
        return std::exp(-(1.f+q)*std::pow(freq-freq_cut,2.f));
    } else {
        return 1.f;
    }
}

float Vco::filter_emulation(float freq, float freq_cut, float q,float shape){
    if (shape<0){
        return filter_bandpass(freq,freq_cut,q)*(1.f-shape*(-1.f+filter_lowpass(freq,freq_cut,q)));
    } else {
        return filter_bandpass(freq,freq_cut,q)*(1.f+shape*(-1.f+filter_highpass(freq,freq_cut,q)));
    }
}



void Vco::set_filter(float freq_cut, float q, float shape) {
    for(int i = 0;i<32;i++){
        amount[i] *= filter_emulation(i+1.f,freq_cut,q*10.f,shape);
        //TODO : formula not working in case of spread/detune : use frequency array 


    }
}

struct Additive : Module {
    enum ParamId {
        SPREAD_PARAM,
        PARTIALS_PARAM,
        PITCH_PARAM,
        FINE_PARAM,
        SHAPE_PARAM,
        DETUNE_PARAM,
        FILTER_FREQ_PARAM,
        FILTER_Q_PARAM,
        FILTER_SHAPE_PARAM,
        MOD_PARTIALS,
        MOD_SHAPE,
        MOD_FILTER_SHAPE,
        MOD_SPREAD,
        MOD_FILTER_Q,
        NUM_PARAMS
    };
    enum InputId {
        PITCH,
        CV_SHAPE,
        CV_SPREAD,
        CV_PARTIALS,
        CV_FILTER_FREQ,
        CV_FILTER_Q,
        CV_FILTER_SHAPE,
        CV_DETUNE,
        SYNC,
        NUM_INPUTS
    };
    enum OutputId {
        OUTPUT1,
        OUTPUT2,
        OUTPUT,
        NUM_OUTPUTS
    };
    enum LightId {
        //BLINK_LIGHT,
        NUM_LIGHTS
    };
    float phase = 0.f;
    float bufferSync = 0.f;
    Vco osc;
    Additive() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PITCH_PARAM, -24.f, 24.f, 0.f, "Pitch", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine");
        configParam(PARTIALS_PARAM, 0.f, 1.f, 0.f, "Partials", "");
        
        configParam(SHAPE_PARAM, 0.f, 1.f, 0.f, "Shape", "");
        configParam(SPREAD_PARAM, 0.f, 1.f, 0.f, "Spread", "");
        configParam(DETUNE_PARAM, 0.f, 1.f, 0.f, "Detune", "");
        
        configParam(FILTER_FREQ_PARAM, -2.f, 5.f, -2.f, "Filter Freq", " Hz", 2.f, dsp::FREQ_C4);
        configParam(FILTER_Q_PARAM, 0.f, 1.f, 0.f, "Resonance", "");
        configParam(FILTER_SHAPE_PARAM, -1.f, 1.f, 0.f, "Filter shape ", "");


        configParam(MOD_SHAPE, -1.f, 1.f, 0.f, "CV Shape", "");
        configParam(MOD_PARTIALS, -1.f, 1.f, 0.f, "CV Partials", "");
        configParam(MOD_SPREAD, -1.f, 1.f, 0.f, "CV Spread", "");
        configParam(MOD_FILTER_SHAPE, -1.f, 1.f, 0.f, "CV F Shape", "");
        configParam(MOD_FILTER_Q, -1.f, 1.f, 0.f, "CV F Res", "");        
    }
    void process(const ProcessArgs &args) override {
        // Parameters configuration 
        float frequency = params[PITCH_PARAM].getValue();
		float fine = params[FINE_PARAM].getValue();
        float partials = params[PARTIALS_PARAM].getValue();

        
        float shape = params[SHAPE_PARAM].getValue();
        float spread  = params[SPREAD_PARAM].getValue();
        float detune  = params[DETUNE_PARAM].getValue();

        float filter_freq  = params[FILTER_FREQ_PARAM].getValue();
        float filter_q = params[FILTER_Q_PARAM].getValue();
        float filter_shape = params[FILTER_SHAPE_PARAM].getValue();


        // First row of inputs configuration

        float cv_shape = inputs[CV_SHAPE].getVoltage();
        float cv_partials = inputs[CV_PARTIALS].getVoltage();
        float cv_spread = inputs[CV_SPREAD].getVoltage();
        float cv_filter_shape = inputs[CV_FILTER_SHAPE].getVoltage();
        float cv_filter_q = inputs[CV_FILTER_Q].getVoltage();
        
        // Second row of inputs configuration

        float pitch =  inputs[PITCH].getVoltage();
        float sync = inputs[SYNC].getVoltage();
        float cv_detune = inputs[CV_DETUNE].getVoltage();
        float cv_filter_freq = inputs[CV_FILTER_FREQ].getVoltage();


        // Mod parameters configuration 

        float mod_shape = params[MOD_SHAPE].getValue();
        float mod_partials = params[MOD_PARTIALS].getValue();
        float mod_spread = params[MOD_SPREAD].getValue();
        float mod_filter_shape = params[MOD_FILTER_SHAPE].getValue();
        float mod_filter_q = params[MOD_FILTER_Q].getValue();


        // Main values

        shape = simd::clamp(shape + cv_shape*mod_shape/5.f,0.f,1.f);
        partials = simd::clamp(partials + cv_partials*mod_partials/5.f,0.01f,1.f)*10.f;
        spread = simd::clamp(spread + cv_spread*mod_spread/5.f,0.f,1.f);
        filter_shape = simd::clamp(filter_shape + cv_filter_shape*mod_filter_shape/5.f,-1.f,1.f);
        filter_q = simd::clamp(filter_q + cv_filter_q*mod_filter_q/5.f,0.f,1.f);
        
        
        detune = simd::clamp(detune + cv_detune/5.f,0.f,1.f);
        filter_freq = simd::clamp(filter_freq + cv_filter_freq/3.5f,-2.f,5.f);
        float filter_frequency = dsp::FREQ_C4 * simd::pow(2.f, filter_freq);
        
        
        float freqParam = frequency / 12.f;
		freqParam += dsp::quadraticBipolar(fine) * 3.f / 12.f;
        /*
        pitch += frequency;
		pitch += fine/1200;
        float freq = dsp::FREQ_C4 * simd::pow(2.f, pitch);
        */
        pitch+=freqParam;
        float freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 20) / 1048576;
        //float freq = dsp::FREQ_C4 * std::pow(2,pitch + 30) / 1073741824;

        // index of filter cutting frequency f_c
        float filter_index_c = filter_frequency/freq;


        float deltaPhase = simd::clamp(freq * args.sampleTime, 1e-6f, 0.35f);
		phase += deltaPhase;

        //  sync function
        if (sync>0.1f && bufferSync>=0 && bufferSync<=0.1f){
            phase=0.f;
        }
        bufferSync=sync;
        


		phase -= simd::floor(phase);

        osc.set_frequencies(spread,detune);
        osc.set_amount(partials);
        osc.set_filter(filter_index_c,filter_q,filter_shape);

        float out = osc.process(phase,shape,partials);
        outputs[OUTPUT].setVoltage(simd::clamp(out*4.5f,-5.f,5.f));

    }
};
struct AdditiveWidget : ModuleWidget {
    AdditiveWidget(Additive *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/wipPanel.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParam<RoundBlackKnob>(Vec(30, 35), module, Additive::PITCH_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(90, 35), module, Additive::FINE_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(130, 35), module, Additive::PARTIALS_PARAM));
        
        
        
        addParam(createParam<RoundBlackKnob>(Vec(30, 100), module, Additive::SHAPE_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(90, 100), module, Additive::SPREAD_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(130, 100), module, Additive::DETUNE_PARAM));

        addParam(createParam<RoundBlackKnob>(Vec(30, 165), module, Additive::FILTER_FREQ_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(90, 165), module, Additive::FILTER_SHAPE_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(130, 165), module, Additive::FILTER_Q_PARAM));


        addParam(createParam<RoundSmallBlackKnob>(Vec(15, 237), module, Additive::MOD_SHAPE));
        addParam(createParam<RoundSmallBlackKnob>(Vec(45, 237), module, Additive::MOD_PARTIALS));
        addParam(createParam<RoundSmallBlackKnob>(Vec(75, 237), module, Additive::MOD_SPREAD));       
        addParam(createParam<RoundSmallBlackKnob>(Vec(105, 237), module, Additive::MOD_FILTER_SHAPE));
        addParam(createParam<RoundSmallBlackKnob>(Vec(135, 237), module, Additive::MOD_FILTER_Q));



        addInput(createInput<PJ301MPort>(Vec(15, 276), module, Additive::CV_SHAPE));
        addInput(createInput<PJ301MPort>(Vec(45, 276), module, Additive::CV_PARTIALS));
        addInput(createInput<PJ301MPort>(Vec(75, 276), module, Additive::CV_SPREAD));
        addInput(createInput<PJ301MPort>(Vec(105, 276), module, Additive::CV_FILTER_SHAPE));
        addInput(createInput<PJ301MPort>(Vec(135, 276), module, Additive::CV_FILTER_Q));



        addInput(createInput<PJ301MPort>(Vec(15, 320), module, Additive::PITCH));
        addInput(createInput<PJ301MPort>(Vec(45, 320), module, Additive::SYNC));
        addInput(createInput<PJ301MPort>(Vec(75, 320), module, Additive::CV_DETUNE));
        addInput(createInput<PJ301MPort>(Vec(105, 320), module, Additive::CV_FILTER_FREQ));
        addOutput(createOutput<PJ301MPort>(Vec(135, 320), module, Additive::OUTPUT));

    }
};


// Define the Model with the Module type, ModuleWidget type, and module slug
Model *modelAdditive = createModel<Additive, AdditiveWidget>("Additive");
