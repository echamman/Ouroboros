#include "daisysp.h"
#include "daisy_seed.h"
#include "src/OLED/OLED.h"
#include "src/Presets/presets.h"

// Interleaved audio definitions
#define LEFT (i)
#define RIGHT (i + 1)

// For DEBUGGING
CpuLoadMeter loadMeter;

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 0.75f)

enum AdcChannel {
    Knob0 = 0,
    Knob1,
    Knob2,
    Knob3,
    NUM_ADC_CHANNELS
};

enum Switches {
    Switch0 = 0,
    NUM_SWITCHES
};

using namespace daisysp;
using namespace daisy;

// Function declarations
int OuroborosInit();
void updateDelay();
void updateReverb();
void updateFlutter();

// Declare a DaisySeed object called hardware
DaisySeed hw;

AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
Switch buttons[NUM_SWITCHES];

// Global variable holds sample rate
float sample_rate;

// Global variables TO BE MOVED
float delaytime = 0.0f;
float delayFDBK = 0.75f;
float delaybuf[5] = { };
float reverbtime = 0.0f;
// Declare a variable to store the state we want to set for the LED.
bool led_state = false;

// Declare a DelayLine of MAX_DELAY number of floats.
static DelayLine<float, MAX_DELAY> del;

// Create ReverbSc, load it to SDRAM
static ReverbSc DSY_SDRAM_BSS verb;
float verbBlend;

// Create chorus for a flutter effect
static Chorus flutter;

// Metro 
static Metro tick;

// OLED Screen 
static Oled display;

// Preset Manager
static presets presetManager;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{

    loadMeter.OnBlockStart();

    if(tick.Process()){
        led_state = !led_state;
    }

    float feedback, del_out, sig_outL, sig_outR, verb_outL, verb_outR;

    for(size_t i = 0; i < size; i += 2)
    {
        // Read from delay line
        del_out = del.Read();
        // Calculate output and feedback
        sig_outL  = del_out + in[i];
        feedback = (del_out * delayFDBK) + in[i];

        // Write to the delay
        del.Write(feedback);
        flutter.Process(sig_outL);

        //sig_outL = flutter.GetLeft();
        //sig_outR = flutter.GetRight();

        // Reverb writes to output
        verb.Process(sig_outL, sig_outL, &verb_outL, &verb_outR);

        // Output
        out[LEFT]  = (1-verbBlend)*sig_outL + verbBlend*verb_outL;
        out[RIGHT] = (1-verbBlend)*sig_outL + verbBlend*verb_outR;
    }

    loadMeter.OnBlockEnd();
}

int main(void)
{

    OuroborosInit();

    string dLines[6]; //Create an Array of strings for the OLED display

    // Loop forever
    for(;;)
    {

        // get the current load (smoothed value and peak values)
        const float avgLoad = loadMeter.GetAvgCpuLoad();
        const float maxLoad = loadMeter.GetMaxCpuLoad();
        const float minLoad = loadMeter.GetMinCpuLoad();

        // Set the onboard LED
        hw.SetLed(led_state);

        //Debounce the button
        buttons[Switch0].Debounce();

        // Read inputs and update delay and verb variables
        updateDelay();
        updateReverb();

        if(buttons[Switch0].FallingEdge()){
            presetManager.nextPreset();
        }

        //Print to display
        dLines[0] = "TIME: " + std::to_string((int)(delaytime * 1000.0f)) + "ms";
        dLines[1] = "FDBK: " + std::to_string((int)(delayFDBK*100.00f));
        dLines[2] = "SPACE: " + std::to_string((int)floor(hw.adc.GetFloat(Knob2)*100.00f));
        dLines[3] = "WOW: " + std::to_string((int)floor(hw.adc.GetFloat(Knob3)*100.00f));
        //(buttons[Switch0].Pressed() ? dLines[4] = "Button: true" : dLines[4] = "Button: false");
        //dLines[4] = "Loads: " + std::to_string((int)(avgLoad*100.0f)) + " " + std::to_string((int)(maxLoad * 100.0f));// + " " + std::to_string((int)maxLoad);
        dLines[4] = "Preset: " + presetManager.getPresetName();
        dLines[5] = "";
        display.print(dLines, 6);

    }
}

// Initializes everything in the system
int OuroborosInit(){

    // initialize seed hardware and daisysp modules
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();
    del.Init();
    verb.Init(sample_rate);
    verb.SetLpFreq(18000.0f);

    // Initialize knobs
    adcConfig[Knob0].InitSingle(hw.GetPin(15));
    adcConfig[Knob1].InitSingle(hw.GetPin(16));
    adcConfig[Knob2].InitSingle(hw.GetPin(17));
    adcConfig[Knob3].InitSingle(hw.GetPin(18));

    // Initialize Buttons
    buttons[Switch0].Init(hw.GetPin(25), 1000);

    //Initialize the adc with the config we just made
    hw.adc.Init(adcConfig, NUM_ADC_CHANNELS);

    // Set Delay time to 0.75 seconds
    del.SetDelay(sample_rate * 0.2f);

    // Initialize Flutter
    flutter.Init(sample_rate);
    flutter.SetLfoFreq(.33f, .2f);
    flutter.SetLfoDepth(1.f, 1.f);
    flutter.SetDelay(.75f, .9f);

    // Initialize Metro at 100Hz
    tick.Init(10.0f, sample_rate);

    // initialize the load meter so that it knows what time is available for the processing:
    loadMeter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());

    // start callback
    hw.adc.Start();
    hw.StartAudio(AudioCallback);

    //Allow the OLED to start up
    System::Delay(100);
    /** And Initialize */
    display.Init(&hw);
    System::Delay(2000);

    return 1;
}

void updateDelay(){

    float min = presetManager.getDelayMin();
    float range = presetManager.getDelayRange();

    // Read and scale appropriate knobs
    // Read knob as float (0-0.99), truncate to two decimal points
    float delayKnob = std::trunc(hw.adc.GetFloat(Knob0)*100.0f)/100.0f;
    
    for(int i = 3; i >= 0; i--){
        delaybuf[i+1] = delaybuf[i];
    }

    delaybuf[0] = delayKnob;

    // Only update delay if knob has been turned sufficiently
    if( delaybuf[0] == delaybuf[1] && delaybuf[0] == delaybuf[2]){
        delaytime = min + (delayKnob * range);
        
        // Set delay to value between 1 and 103ms, set metro to freq
        del.SetDelay(sample_rate * (delaytime));
        
        // Set tick 1/delay time * 2 * 4 (2 is so LED has rising edge every tick, unsure why 4 is needed)
        tick.SetFreq(8.0f/delaytime); 
    }
}

void updateReverb(){

    float min = presetManager.getReverbFDBKMin();
    float range = presetManager.getReverbFDBKRange();

    // Read and scale appropriate knobs
    // Truncates the pot reading to 2 decimal points
    float rFDBK = std::trunc(hw.adc.GetFloat(Knob1)*100.0f)/100.0f;
    rFDBK = min + (rFDBK * range); 
    delayFDBK = rFDBK;

    float rBlend = std::trunc(hw.adc.GetFloat(Knob2)*100.0f)/100.0f;

    verb.SetFeedback(rFDBK);
    verbBlend = rBlend;
}

void updateFlutter(){
    return;
}
