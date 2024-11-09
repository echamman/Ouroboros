#include "tempoManager.h"

void tempoManager::Init(float sample_rate, DaisySeed *seed, int knob, presets *presetManager){

    rateKnob = knob;
    preset = *presetManager;
    hw = *seed;

    time_point<steady_clock> now = steady_clock::now();

    //Fill buffer
    for(int i = 0; i < 3; i++){
        press[i] = now;
        
    }
    p0 = now;
}

// Records a button press
void tempoManager::pressEvent(){

    time_point<steady_clock> now = steady_clock::now();

    for(int i = 2; i > 0; i--){
        press[i] = press[i-1];
    }
    p1 = p0;
    p0 = now;

    press[0] = now;
}

int64_t tempoManager::getNow(){

    auto dur = duration_cast<milliseconds>(p0 - p1);

    return dur.count();
}


//Returns a tempo if 3 pushes were registered, else return knob time. Time in ms
int64_t tempoManager::getTempo(){

    // Read and scale appropriate knobs
    // Read knob as float (0-0.99), truncate to two decimal points
    float delayKnob = std::trunc(hw.adc.GetFloat(rateKnob)*100.0f)/100.0f;

    float min = preset.getDelayMin();
    float range = preset.getDelayRange();
    float delaytime;

    milliseconds dur12 = duration_cast<milliseconds>(press[2] - press[1]);
    milliseconds dur01 = duration_cast<milliseconds>(press[1] - press[0]);

    // Return last duration if the two durations are within 200ms of each other and not 10ms of eachother
    if((dur12.count() - dur01.count()) < 200){
        if((dur12.count() - dur01.count()) > 10){
            tap = true;
            tapTempo = dur01.count();
        }
    }

    for(int i = 3; i >= 0; i--){
        delaybuf[i+1] = delaybuf[i];
    }

    delaybuf[0] = delayKnob;

    // Only update delay if knob has been turned sufficiently
    if( delaybuf[0] == delaybuf[1] && delaybuf[0] == delaybuf[2]){
        delaytime = (min + (delayKnob * range));

        if(abs(delaytime - ((float)(knobTempo))/1000.0f) > 0.01f){
            knobTempo = (int64_t)(delaytime*1000.0f);
            tap = false;
        }
    }

    if(tap){
        return tapTempo;
    }else{
        return knobTempo;
    }
}
