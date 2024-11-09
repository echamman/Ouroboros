#include "daisysp.h"
#include "daisy_seed.h"
#include "src/OLED/OLED.h"
#include "src/Presets/presets.h"
#include "src/tempoManager/tempoManager.h"

// Interleaved audio definitions
#define LEFT (i)
#define RIGHT (i + 1)

// For DEBUGGING
CpuLoadMeter loadMeter;

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 0.75f)

enum AdcChannel {
    rateKnob = 0,
    fdbkKnob,
    spaceKnob,
    wowKnob,
    blendKnob,
    NUM_ADC_CHANNELS
};

enum Switches {
    programSw = 0,
    divisionSw,
    tempoSw,
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
int divisions = 1;
float wow = 0.0f;
// Officially Space
float verbBlend;
// Blends delay and wow
float wetBlend;
// Declare a variable to store the state we want to set for the LED.
bool led_state = false;

// Declare a DelayLine of MAX_DELAY number of floats.
static DelayLine<float, MAX_DELAY> del;

// Create ReverbSc, load it to SDRAM
static ReverbSc DSY_SDRAM_BSS verb;

// Create chorus for a flutter effect
static Chorus flutter;

// Metro 
static Metro tick;

// OLED Screen 
static Oled display;

// Tempo manager
static tempoManager tempo;

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

    float feedback, del_out, sig_outL, sig_outR;
    float verb_outL, verb_outR, wow_outL, wow_outR;
    float wow_del_outL, wow_del_outR;

    for(size_t i = 0; i < size; i += 2)
    {
        // Read from delay line
        del_out = del.Read();
        // Calculate output and feedback
        feedback = (del_out * delayFDBK) + in[LEFT];

        // Write to the delay
        del.Write(feedback);
        flutter.Process(del_out + in[LEFT]);

        wow_outL = flutter.GetLeft();
        wow_outR = flutter.GetRight();

        //Form signal output with delay and wow
        wow_del_outL = (wow * wow_outL) + ((1-wow) * del_out);
        wow_del_outR = (wow * wow_outR) + ((1-wow) * del_out);

        sig_outL = (wow_del_outL * wetBlend) + ((1 - wow) * in[LEFT]);
        sig_outR = (wow_del_outR * wetBlend) + ((1 - wow) * in[RIGHT]);

        // Reverb writes to output
        verb.Process(sig_outL, sig_outR, &verb_outL, &verb_outR);

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
        buttons[programSw].Debounce();
        buttons[tempoSw].Debounce();
        buttons[divisionSw].Debounce();

        // Read inputs and update delay and verb variables
        updateDelay();
        updateReverb();
        updateFlutter();

        if(buttons[programSw].FallingEdge()){
            presetManager.nextPreset();
        }

        // Cycle through division count
        if(buttons[divisionSw].FallingEdge()){
            divisions = ((divisions) % 4) + 1;
        }

        // Tap tempo
        if(buttons[tempoSw].FallingEdge()){
            tempo.pressEvent();
        }

        //Print to display
        dLines[0] = "Preset: " + presetManager.getPresetName();
        dLines[1] = "TIME: " + std::to_string((int)(delaytime * 1000.0f)) + "ms / " + std::to_string(divisions);
        dLines[2] = "FDBK: " + std::to_string((int)(delayFDBK*100.00f));
        dLines[3] = "SPACE: " + std::to_string((int)floor(hw.adc.GetFloat(spaceKnob)*100.00f));
        dLines[4] = std::to_string(tempo.getNow());
        //dLines[4] = "WOW: " + std::to_string((int)floor(hw.adc.GetFloat(wowKnob)*100.00f));
        //(buttons[programSw].Pressed() ? dLines[4] = "Button: true" : dLines[4] = "Button: false");
        //dLines[4] = "Loads: " + std::to_string((int)(avgLoad*100.0f)) + " " + std::to_string((int)(maxLoad * 100.0f));// + " " + std::to_string((int)maxLoad);
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
    tempo.Init(sample_rate, &hw, rateKnob, &presetManager);

    // Initialize knobs
    adcConfig[rateKnob].InitSingle(hw.GetPin(15));
    adcConfig[fdbkKnob].InitSingle(hw.GetPin(16));
    adcConfig[spaceKnob].InitSingle(hw.GetPin(17));
    adcConfig[wowKnob].InitSingle(hw.GetPin(18));
    adcConfig[blendKnob].InitSingle(hw.GetPin(19));

    // Initialize Buttons
    buttons[programSw].Init(hw.GetPin(26), 1000);
    buttons[divisionSw].Init(hw.GetPin(27), 1000);
    buttons[tempoSw].Init(hw.GetPin(25), 1000);

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
    tick.Init(4.0f, sample_rate);

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

    // Reads tempo from the tempo manager
    // TODO standardize tempo as either float in seconds or int64_t in ms
    delaytime = ((float)tempo.getTempo()) / 1000.0f;
    
    wetBlend = std::trunc(hw.adc.GetFloat(blendKnob)*100.0f)/100.0f;

    // Set delay to value between 1 and 103ms, set metro to freq
    del.SetDelay(sample_rate * (delaytime / (float)divisions));
    
    // Set tick (1/delay time) * 2 * 4 (2 is so LED has rising edge every tick, unsure why 4 is needed)
    tick.SetFreq(8.0f/delaytime); 
}

void updateReverb(){

    float min = presetManager.getReverbFDBKMin();
    float range = presetManager.getReverbFDBKRange();

    // Read and scale appropriate knobs
    // Truncates the pot reading to 2 decimal points
    float rFDBK = std::trunc(hw.adc.GetFloat(fdbkKnob)*100.0f)/100.0f;
    rFDBK = min + (rFDBK * range); 
    delayFDBK = rFDBK;

    float rBlend = std::trunc(hw.adc.GetFloat(spaceKnob)*100.0f)/100.0f;

    verb.SetFeedback(rFDBK);
    verbBlend = rBlend;
}

void updateFlutter(){

    // Read and scale appropriate knobs
    // Truncates the pot reading to 2 decimal points
    float flutterAm = std::trunc(hw.adc.GetFloat(wowKnob)*100.0f)/100.0f;
    wow = flutterAm;

    flutter.SetLfoFreq(2.0f * flutterAm);
    flutter.SetLfoDepth(0.6f + flutterAm * 0.4f);
    flutter.SetDelay(0.5f + flutterAm * 0.5f);
    flutter.SetFeedback(0.3f + flutterAm * 0.4f);
}
