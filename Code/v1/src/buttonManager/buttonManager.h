/*
Button Manager class

*/
#ifndef BUTTONMANAGER_H
#define BUTTONMANAGER_H

#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

enum Switches {
    programSw = 0,
    divisionSw,
    tempoSw,
    toggleSw,
    NUM_SWITCHES
};

class buttonManager {

    private:

        DaisySeed hw;
        Switch buttons[NUM_SWITCHES];

    public:

     void Init(DaisySeed *seed);
     int Process();

};

#endif