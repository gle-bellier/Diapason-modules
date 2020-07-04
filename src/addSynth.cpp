#include "plugin.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <math.h>

class Vco {
public :
    float process(float,float);
    void set_frequencies(float,float);
    void set_amount(float);
    void set_pulsewave(float);
    void set_filter(float,float);
    float frequencies[32] = {};
    float amount[32] = {};
    double sawtooth_coeff[32] = {};
    double square_coeff[32] = {};
    float out_sawtooth=0;
    float out_square=0;
    Vco();

private:
    float filter_emulation(int,float,float);
};
Vco::Vco(){
    for(int i=0;i<32;i++){
        sawtooth_coeff[i]=(2.f/((i+1.f)*M_PI))*std::pow(-1.f,(i+2.f));
    }
    set_pulsewave(0.5);
}
float Vco::process (float phase,float shape){
    shape*=1.95f;
    if (shape>1.f){
        set_pulsewave(1.f - 0.5f*shape);
    } else {
        set_pulsewave(0.5f);
    }
    out_sawtooth=0;
    out_square=0;
    for(int i = 0;i<32;i++){
        out_square += amount[i] * square_coeff[i] * std::cos(2.f* frequencies[i] * M_PI * (phase-0.25f));
    };
    for(int i = 0;i<32;i++){
        out_sawtooth += amount[i] * sawtooth_coeff[i] * std::sin(2.f* frequencies[i] * M_PI * phase);
    };
    if (shape<1.f) {
        return shape * (2.f * out_square) + (1.f - shape) * out_sawtooth;
    } else {
        return 2.f*out_square-(shape-1.f);
    }
}
void Vco::set_frequencies(float spread,float detune){
    frequencies[0] = 1;
    for(int i = 1;i<32;i++){
        frequencies[i] = i+1.f+spread + std::pow(-1.05f,i)*detune;
    };
}
void Vco::set_amount(float k){
    for(int i = 0;i<32;i++){
        amount[i] = std::exp(-std::pow((float)(i+1) - 1.f,2)/std::pow(k,4.f)); //
    };
}
void Vco::set_pulsewave(float pulsewidth){
    for(int i=0;i<32;i++){
        square_coeff[i]=(2.f/((i+1.f)*M_PI))*std::sin((i+1.f)*M_PI*(pulsewidth));
        //TODO : formula not working in case of spread/detune : use frequency array 
    }
}
float Vco::filter_emulation(int freq, float freq_cut, float q){
    //return 35.f*std::pow(q + 0.1f, 2.f)*std::exp(-10.f*std::pow(q,0.5f)*std::pow(freq-freq_cut,2.f))-65.f*std::pow(q,2.f)+0.65f;
    float b0 = 1.f - 0.5f*std::exp(-1.f+q/15);
    float b1 = std::log(1.f + std::exp(1.f)*q);
    float b2 = std::pow(std::exp(-std::log(1.f+q)*0.125f*q*std::pow(freq-freq_cut,2.f)),2.f);
    float b3 = std::log(1+freq_cut);
    float b4 = 1.f - std::log(1.f + 0.025*q);
    return b0*b1*b2*b3+b4;

}

void Vco::set_filter(float freq_cut, float q) {
    for(int i = 0;i<32;i++){
        amount[i] *= filter_emulation(i+1,freq_cut,q*10.f);
        //std::cout<<amount[i] << std::endl;;

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
        MOD_PARTIALS,
        MOD_SHAPE,
        MOD_FILTER_FREQ,
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
        CV_DETUNE,
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
    Vco osc;
    Additive() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PITCH_PARAM, -2.f, 2.f, 0.f, "Pitch", " Hz", 2.f, dsp::FREQ_C4);
        configParam(FILTER_FREQ_PARAM, -2.f, 5.f, -2.f, "Filter Freq", " Hz", 2.f, dsp::FREQ_C4);
		configParam(FINE_PARAM, -100.f, 100.f, 0.f, "Fine","cts");
        configParam(FILTER_Q_PARAM, 0.f, 1.f, 0.f, "Resonance", "");

        configParam(SHAPE_PARAM, 0.f, 1.f, 0.f, "Shape", "");
        configParam(SPREAD_PARAM, 0.f, 1.f, 0.f, "Spread", "");
        configParam(DETUNE_PARAM, 0.f, 1.f, 0.f, "Detune", "");
        configParam(PARTIALS_PARAM, 0.f, 1.f, 0.f, "Partials", "");
        

        configParam(MOD_PARTIALS, -1.f, 1.f, 0.f, "Amount CV", "");
        configParam(MOD_SHAPE, -1.f, 1.f, 0.f, "Amount CV", "");
        configParam(MOD_FILTER_Q, -1.f, 1.f, 0.f, "Amount CV", "");        
        configParam(MOD_FILTER_FREQ, -1.f, 1.f, 0.f, "Amount CV", "");
    }
    void process(const ProcessArgs &args) override {
        float frequency = params[PITCH_PARAM].getValue();
		float fine = params[FINE_PARAM].getValue();

        //float partials = 10.f*params[PARTIALS_PARAM].getValue();
        //float shape = simd::clamp(params[SHAPE_PARAM].getValue() + inputs[CV_SHAPE].getVoltage()/5.f,0.f,1.9f);
        float partials = params[PARTIALS_PARAM].getValue();
        float shape = params[SHAPE_PARAM].getValue();
        float spread  = params[SPREAD_PARAM].getValue();
        float detune  = params[DETUNE_PARAM].getValue();
        float filter_freq  = params[FILTER_FREQ_PARAM].getValue();
        float filter_q = params[FILTER_Q_PARAM].getValue();


        float pitch =  inputs[PITCH].getVoltage();
        float cv_shape = inputs[CV_SHAPE].getVoltage();
        float cv_spread = inputs[CV_SPREAD].getVoltage();
        float cv_detune = inputs[CV_DETUNE].getVoltage();
        float cv_partials = inputs[CV_PARTIALS].getVoltage();
        float cv_filter_freq = inputs[CV_FILTER_FREQ].getVoltage();
        float cv_filter_q = inputs[CV_FILTER_Q].getVoltage();

        float mod_partials = params[MOD_PARTIALS].getValue();
        float mod_shape = params[MOD_SHAPE].getValue();
        float mod_filter_freq = params[MOD_FILTER_FREQ].getValue();
        float mod_filter_q = params[MOD_FILTER_Q].getValue();

        detune = simd::clamp(detune + cv_detune/5.f,0.f,0.1f);
        spread = simd::clamp(spread + cv_spread/5.f,0.f,0.1f);

        partials = simd::clamp(partials + cv_partials*mod_partials/5.f,0.01f,1.f)*10.f;
        shape = simd::clamp(shape + cv_shape*mod_shape/5.f,0.f,1.f);
        filter_freq = simd::clamp(filter_freq + cv_filter_freq*mod_filter_freq/3.5f,-2.f,5.f);
        filter_q = simd::clamp(filter_q + cv_filter_q*mod_filter_q/5.f,0.f,1.f);


        float filter_frequency = dsp::FREQ_C4 * simd::pow(2.f, filter_freq);
        pitch += frequency;
		pitch += fine/1200;
        float freq = dsp::FREQ_C4 * simd::pow(2.f, pitch);
        // index of filter cutting frequency f_c
        float filter_index_c = filter_frequency/freq;


        float deltaPhase = simd::clamp(freq * args.sampleTime, 1e-6f, 0.35f);
		phase += deltaPhase;
		phase -= simd::floor(phase);



        osc.set_frequencies(spread,detune);
        osc.set_amount(partials);
        osc.set_filter(filter_index_c,filter_q);

        float out = osc.process(phase,shape);
        outputs[OUTPUT].setVoltage(simd::clamp(out*4.5f,-5.f,5.f));

    }
};
struct AdditiveWidget : ModuleWidget {
    AdditiveWidget(Additive *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/addSynthPanel.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParam<RoundBlackKnob>(Vec(18, 55), module, Additive::PITCH_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(18, 105), module, Additive::SPREAD_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(18, 155), module, Additive::PARTIALS_PARAM));

        addParam(createParam<RoundBlackKnob>(Vec(60, 55), module, Additive::FINE_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(60, 105), module, Additive::DETUNE_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(60, 155), module, Additive::SHAPE_PARAM));

        addParam(createParam<RoundBlackKnob>(Vec(102, 105), module, Additive::FILTER_FREQ_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(102, 155), module, Additive::FILTER_Q_PARAM));


        addParam(createParam<RoundSmallBlackKnob>(Vec(11, 227), module, Additive::MOD_PARTIALS));
        addParam(createParam<RoundSmallBlackKnob>(Vec(45, 227), module, Additive::MOD_SHAPE));       
        addParam(createParam<RoundSmallBlackKnob>(Vec(80, 227), module, Additive::MOD_FILTER_FREQ));
        addParam(createParam<RoundSmallBlackKnob>(Vec(114, 227), module, Additive::MOD_FILTER_Q));



        addInput(createInput<PJ301MPort>(Vec(11, 276), module, Additive::CV_PARTIALS));
        addInput(createInput<PJ301MPort>(Vec(45, 276), module, Additive::CV_SHAPE));
        addInput(createInput<PJ301MPort>(Vec(80, 276), module, Additive::CV_FILTER_FREQ));
        addInput(createInput<PJ301MPort>(Vec(114, 276), module, Additive::CV_FILTER_Q));


        addInput(createInput<PJ301MPort>(Vec(11, 320), module, Additive::PITCH));
        addInput(createInput<PJ301MPort>(Vec(45, 320), module, Additive::CV_DETUNE));
        addInput(createInput<PJ301MPort>(Vec(80, 320), module, Additive::CV_SPREAD));
        addOutput(createOutput<PJ301MPort>(Vec(114, 320), module, Additive::OUTPUT));

    }
};


// Define the Model with the Module type, ModuleWidget type, and module slug
Model *modelAdditive = createModel<Additive, AdditiveWidget>("Additive");
