#include "knobManager.h"

void knobManager::Init(DaisySeed *seed){

    hw = *seed;

    // Initialize knobs
    adcConfig[rateKnob].InitSingle(hw.GetPin(15));
    adcConfig[fdbkKnob].InitSingle(hw.GetPin(16));
    adcConfig[spaceKnob].InitSingle(hw.GetPin(17));
    adcConfig[wowKnob].InitSingle(hw.GetPin(18));
    adcConfig[blendKnob].InitSingle(hw.GetPin(19));
    adcConfig[filtKnob].InitSingle(hw.GetPin(20));

    //Initialize the adc with the config we just made
    hw.adc.Init(adcConfig, NUM_ADC_CHANNELS);

    hw.adc.Start();
}

// Returns true if the knobs have been moved since last check
bool knobManager::checkKnobs(){

    int val = 0;

    for(int i = 0; i < 6; i++){

        val = (int)(readKnob(i) * 100.0f);
        if(K[i] != val){
            K[i] = val;
            return true;
        }
    }

    return false;
}

bool knobManager::checkKnob(int knob){

    int val = (int)(readKnob(knob) * 100.0f);
    
    if(K[knob] != val){
        K[knob] = val;
        return true;
    }
}

//Returns a float reading of the knob, 0.00 - 0.99
float knobManager::readKnob(int knob){

    return std::trunc((1.0 - hw.adc.GetFloat(knob))*100.0f)/100.0f;
}

//Returns a float reading of the knob scaled between min and max
float knobManager::readKnob(int knob, float min, float max){

    float range = max - min;

    float val = std::trunc((1.0 - hw.adc.GetFloat(knob))*100.0f)/100.0f;
    val = val * range;
    val = val + min;

    return val;
    
}

//Returns a float reading of the knob (0.00 - 0.99) plus an offset
float knobManager::readKnob(int knob, float offset){

    float val = std::trunc((1.0 - hw.adc.GetFloat(knob))*100.0f)/100.0f;
    val = val + offset;

    return val;
}

string knobManager::readKnobString(int knob){

    return to_string((int)floor((1.0 - hw.adc.GetFloat(knob)) * 100.0f));
}


