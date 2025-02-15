/*
    Tempo  management class
    Records time of last two presses to form tempo, or reads knob
*/

#ifndef TEMPOMANAGER_H
#define TEMPOMANAGER_H

#include "daisysp.h"
#include "daisy_seed.h"
#include "../Presets/presets.h"
#include "../knobManager/knobManager.h"
#include <chrono>

#define MAX_DELAY_S 8       // 8 second recording buffer for sampling

using namespace daisysp;
using namespace daisy;
using namespace std::chrono;


class tempoManager{
    private:
        float sample_rate;
        float tapTempo = 300.0f;    // Time in milliseconds
        float knobTempo = 0.0f;     // Time in milliseconds
        float smoothed_tempo = 0.0f;
        uint64_t samples = 0;       // ticks passed 

        float delaybuf[5] = { };    // Knob debouncing buffer
        float press[3];          // Tap tempo buffer
        int bufCount = 0;           // Valid spots in tap tempo buffer

        bool overflow = true;       // overflowed maximum tap tempo delay time
        bool tap = false;           // Flag for whether tap tempo or knob tempo will be used

        int rateKnob;               // ADC Value to read knob
        DaisySeed hw;
        presets preset;
        knobManager knobMan;

        // Resets tempo counter 
        void resetCount();

    public:
        // Initializes tap tempo module
        void Init(float sample_rate, DaisySeed *seed, int knob, presets *presetManager, knobManager *knobs);

        // Processed at sample rate frequency
        void Process();

        // Records a button press
        void pressEvent();

        //Returns a tempo if 3 pushes were registered, else -1
        //Needs preset value because of bug
        float getTempo(int currPreset);

};

#endif