#include "tempoManager.h"

void tempoManager::Init(float samprate, DaisySeed *seed, int knob, presets *presetManager, knobManager *knobs){

    sample_rate = samprate;
    rateKnob = knob;
    preset = *presetManager;
    hw = *seed;
    knobMan = *knobs;

    //Fill buffer
    for(int i = 0; i < 3; i++){
        press[i] = 0;
    }
}

void tempoManager::Process(){
    samples = (samples + 1) % ((int)(sample_rate) * MAX_DELAY_S);
    
    if(samples == 0){
        overflow = true;
    }

}

void tempoManager::resetCount(){
    samples = 0;
    overflow = false;
    bufCount = 1;
}

// Records a button press
void tempoManager::pressEvent(){

    // Move sample count through buffer
    for(int i = 2; i > 0; i--){
        press[i] = press[i-1];
    }
    press[0] = samples;

    // Check if overflowed between samples
    if(overflow){
        resetCount();
    }else if (bufCount < 3){
        bufCount++;
    }else{
        tap = true;
        tapTempo = (press[0] - press[1]);
        // Translate samples into time in ms
        tapTempo = ((tapTempo / sample_rate) * 1000.0f);
    }
}

//Returns a tempo if 3 pushes were registered, else return knob time. Time in ms
float tempoManager::getTempo(){

    // Read and scale appropriate knobs
    // Read knob as float (0-0.99), truncate to two decimal points
    float delayKnob = knobMan.readKnob(rateKnob);

    float min = preset.getDelayMin();
    float range = preset.getDelayRange();
    float delaytime;

    delaytime = (min + (delayKnob * range));

    if(abs(delaytime - ((knobTempo))/1000.0f) > 0.01f){
        knobTempo = (delaytime*1000.0f);
        tap = false;
    }

    // Return tap tempo if tapping has occured
    // Return knob tempo if knob has been turned
    if(tap){
        fonepole(smoothed_tempo, tapTempo, 0.0001f);
        return smoothed_tempo;
    }else{
        fonepole(smoothed_tempo, knobTempo, 0.0001f);
        return smoothed_tempo;
    }
}
