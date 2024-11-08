/*
OLED controller class

*/
#ifndef OLED_H
#define OLED_H

#include <stdio.h>
#include <string.h>
#include "daisysp.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"

using namespace daisysp;
using namespace daisy;
using MyOledDisplay = OledDisplay<SSD130xI2c128x64Driver>;
using namespace std;

class Oled{

    private:
        MyOledDisplay display;
        DaisySeed  hw;
        char strbuff[128];

    public: 

        //Initialize OLED Screen
        void Init(DaisySeed *seed);

        //Print to Display
        void print(string dLines[], int max);

};

#endif