#include "buttonManager.h"

void buttonManager::Init(DaisySeed *seed){

    hw = *seed;

    // Initialize Buttons
    buttons[programSw].Init(hw.GetPin(26), 1000);
    buttons[divisionSw].Init(hw.GetPin(25), 1000);
    buttons[tempoSw].Init(hw.GetPin(27), 1000);
    buttons[toggleSw].Init(hw.GetPin(24), 1000);
}

/*  Debounces buttons and checks for presses
    Returns: 
    0 - No pushes
    1-4 - correlates to single button push: prog, div, temp, tog in that order
    5 - toggle + div pushed
*/
int buttonManager::Process(){
    
    //Debounce the button
    buttons[programSw].Debounce();
    buttons[tempoSw].Debounce();
    buttons[divisionSw].Debounce();
    buttons[toggleSw].Debounce();

    if(buttons[programSw].FallingEdge()){
        return 1;
    }

    if(buttons[tempoSw].FallingEdge()){
        return 3;
    }
    
    if(buttons[divisionSw].RisingEdge() || buttons[toggleSw].RisingEdge()){
        while(1){

            if(buttons[divisionSw].Pressed() && buttons[toggleSw].Pressed()){
                return 5;
            }
            
            if(buttons[divisionSw].FallingEdge()){
                return 2;
            }
            
            if(buttons[toggleSw].FallingEdge()){
                return 4;
            }

            buttons[divisionSw].Debounce();
            buttons[toggleSw].Debounce();

        }
    }
    
   return 0;
}
