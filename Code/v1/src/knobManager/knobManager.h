/*
Knob Manager class

*/
#ifndef KNOBMANAGER_H
#define KNOBMANAGER_H

#include "daisysp.h"
#include "daisy_seed.h"
#include <stdio.h>
#include <string>

using namespace daisysp;
using namespace daisy;
using namespace std;

// Enum for readability
enum AdcChannel {
    rateKnob = 0,
    fdbkKnob,
    spaceKnob,
    wowKnob,
    blendKnob,
    filtKnob,
    NUM_ADC_CHANNELS
};

class knobManager{

    private:
        DaisySeed hw;
        AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
        int K[6] = { 0 };

    public:
        void Init(DaisySeed *seed);
        float readKnob(int knob);
        float readKnob(int knob, float min, float max);
        float readKnob(int knob, float offset);
        string readKnobString(int knob);
        bool checkKnobs();
};

#endif