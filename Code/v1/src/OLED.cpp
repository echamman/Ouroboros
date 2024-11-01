/* Class for the OLED display
*/
#define MAX_OLED_LINE 12

#include "OLED.h"

void Oled::Init(DaisySeed *seed){
    hw = *seed;
    /** Configure the Display */
    MyOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = hw.GetPin(12);
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = hw.GetPin(11);

    display.Init(disp_cfg);

    //Display startup Screen
    display.Fill(false);
    display.SetCursor(0, 18);
    sprintf(strbuff, "Eazaudio");
    display.WriteString(strbuff, Font_11x18, true);
    display.SetCursor(0, 36);
    sprintf(strbuff, "Ouroboros");
    display.WriteString(strbuff, Font_7x10, true);
    display.Update();
}

    //Print to Display
void Oled::print(string dLines[], int max){
    display.Fill(false);
    display.SetCursor(0, 0);
    sprintf(strbuff, dLines[0].c_str());
    display.WriteString(strbuff, Font_7x10, true);
    for(int d=1; d<max; d++){
        display.SetCursor(0, d*11+2);
        sprintf(strbuff, dLines[d].c_str());
        display.WriteString(strbuff, Font_6x8, true);
    }
    display.Update();
}
