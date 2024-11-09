/*
    Tempo  management class
    Records time of last two presses to form tempo, or reads knob
*/

#ifndef TEMPOMANAGER_H
#define TEMPOMANAGER_H

#include "daisysp.h"
#include "daisy_seed.h"
#include "../Presets/presets.h"
#include <chrono>

using namespace daisysp;
using namespace daisy;
using namespace std::chrono;


class tempoManager{
    private:
        int64_t tapTempo = 300;   // Time in milliseconds
        int64_t knobTempo = 0;  // Time in milliseconds
        int rateKnob;
        float delaybuf[5] = { };
        bool tap = false;

        DaisySeed hw;
        presets preset;
        time_point<steady_clock> press[3];
        time_point<steady_clock> p1;
        time_point<steady_clock> p0;

    public:
        // Initializes tap tempo module
        void Init(float sample_rate, DaisySeed *seed, int knob, presets *presetManager);

        // Records a button press
        void pressEvent();

        //Returns a tempo if 3 pushes were registered, else -1
        int64_t getTempo();

        int64_t getNow();

};

#endif