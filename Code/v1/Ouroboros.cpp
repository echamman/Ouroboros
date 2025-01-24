#include "daisysp.h"
#include "daisy_seed.h"
#include "src/OLED/OLED.h"
#include "src/Presets/presets.h"
#include "src/tempoManager/tempoManager.h"
#include "src/buttonManager/buttonManager.h"
#include "src/knobManager/knobManager.h"

// Interleaved audio definitions
#define RIGHT (i + 1)
#define LEFT (i)

// For DEBUGGING
CpuLoadMeter loadMeter;

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 0.75f)

enum LEDs {
    D0 = 0,
    D1,
    D2,
    D3,
    D4,
    D5,
    NUM_LEDS
};

// Output states
enum outStates {
    mono = 0,
    sendRet,
    stereo,
    NUM_STATES
};

// Send/Stereo out toggle
#define SEND_SW 10

using namespace daisysp;
using namespace daisy;

// Function declarations
int OuroborosInit();
void updateDelay();
void updateReverb();
void updateFlutter();
void updateFilter();

// Declare a DaisySeed object called hardware
DaisySeed hw;

AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
GPIO LEDS[NUM_LEDS];
GPIO sendSw;

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
// Filter Frequency and state
float filtFreq;
bool filtEnable;
// Declare a variable to store the state we want to set for the LED.
volatile bool led_state = true;

volatile bool effectOn = true;

int outStatus = mono;
string displayStatus[NUM_STATES] = { "MON", "S/R", "STR"};

// Declare a DelayLine of MAX_DELAY number of floats.
static DelayLine<float, MAX_DELAY> del;

// Create ReverbSc, load it to SDRAM
static ReverbSc DSY_SDRAM_BSS verb;

// Create chorus for a flutter effect
static Chorus flutter;

// Create filter object
static OnePole filter;

// Metro 
static Metro tick;

// OLED Screen 
static Oled display;

// Tempo manager
static tempoManager tempo;

// Button Manager
static buttonManager buttons;

// Knob Manager
static knobManager knobs;

// Preset Manager
static presets presetManager;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{

    //loadMeter.OnBlockStart();

    float feedback, del_out, sig_outL, sig_outR;
    float verb_outL, verb_outR, wow_outL, wow_outR;
    float wow_del_outL, wow_del_outR;

    for(size_t i = 0; i < size; i += 2)
    {
        if(tick.Process()){
            led_state = !led_state;
            // Set the rate LED
            LEDS[D2].Write(led_state);
        }

        tempo.Process();

        // Chorus Processing

        flutter.Process(in[LEFT]);
        wow_outL = flutter.GetLeft();
        if(outStatus == stereo){
            flutter.Process(in[RIGHT]);
            wow_outR = flutter.GetRight();
        }


        sig_outL = ((1-wow) * in[LEFT]) + (1.5 * wow * wow_outL);
        sig_outR = ((1-wow) * in[RIGHT]) + (1.5 * wow * wow_outR);

        // Delay Processing 

        // Read from delay line
        del_out = del.Read();

        // Calculate output and feedback
        // Process external send/return feed if enabled
        if((outStatus == sendRet) && effectOn){
            out[RIGHT] = del_out;
        }
        
        feedback = ( del_out * delayFDBK) + (sig_outL);

        // Write to the delay
        del.Write(feedback);

        //Add delay to output chain
        if((outStatus == sendRet) && effectOn){
            sig_outL = (sig_outL * (1-wetBlend)) + (in[RIGHT] * wetBlend);
        }else{
            sig_outL = (sig_outL * (1-wetBlend)) + (del_out * wetBlend);
            sig_outR = (sig_outR * (1-wetBlend)) + (del_out * wetBlend);
        }

        if(outStatus != stereo){
            sig_outR = sig_outL;
        }

        // Reverb writes to output
        verb.Process(sig_outL, sig_outR, &verb_outL, &verb_outR);

        sig_outL = (1-verbBlend)*sig_outL + (2.0 * verbBlend * verb_outL);
        sig_outR = (1-verbBlend)*sig_outR + (2.0 * verbBlend * verb_outR);

        // Apply LP filter to everything
        if(filtEnable){
            sig_outL = filter.Process(sig_outL);
            sig_outR = filter.Process(sig_outR);
        }

        // Final Output
        if(effectOn){

            out[LEFT]  = sig_outL;
            if(outStatus == stereo){
                out[RIGHT]  = sig_outR;
            }

        }else{
            out[LEFT]  = in[LEFT];
            out[RIGHT] = in[RIGHT];
        }
    }

    //loadMeter.OnBlockEnd();
}

int main(void)
{

    OuroborosInit();

    string dLines[6]; //Create an Array of strings for the OLED display

    bool doUpdate = true;

    // Loop forever
    for(;;)
    {

        // get the current load (smoothed value and peak values)
        const float avgLoad = loadMeter.GetAvgCpuLoad();
        const float maxLoad = loadMeter.GetMaxCpuLoad();
        const float minLoad = loadMeter.GetMinCpuLoad();

        // Read inputs and update delay and verb variables
        if(doUpdate){
            updateDelay();
            updateReverb();
            updateFlutter();
            updateFilter();
        }

        switch(buttons.Process()){
            case 0:
                doUpdate = knobs.checkKnobs();
                break;

            // Preset Button
            case 1:
                presetManager.nextPreset();
                doUpdate = true;
                break;

            // Div button
            case 2: 
                divisions = ((divisions) % 4) + 1;
                doUpdate = true;
                break;

            //tempo button
            case 3: 
                tempo.pressEvent();
                doUpdate = true;
                break;

            // Effect On/Off Button
            case 4: 
                effectOn = !effectOn;
                LEDS[D0].Write(effectOn);
                doUpdate = true;
                break;

            // Mode Change 
            case 5:
                outStatus = ((outStatus + 1) % NUM_STATES);

                if(outStatus == mono){
                    LEDS[D3].Write(true);
                    LEDS[D4].Write(false);
                    sendSw.Write(false);
                }else if(outStatus == sendRet){
                    LEDS[D3].Write(false);
                    LEDS[D4].Write(true);
                    sendSw.Write(false);
                }else{
                    LEDS[D3].Write(false);
                    LEDS[D4].Write(false);
                    sendSw.Write(true);
                }

                doUpdate = true;
                break;
            default:
                doUpdate = knobs.checkKnobs();
                break;
        }

        
        //Print to display
        if(doUpdate){
            dLines[0] = "Preset: " + presetManager.getPresetName();
            dLines[1] = "TIME: " + std::to_string((int)(delaytime * 1000.0f)) + "ms / " + std::to_string(divisions);
            dLines[2] = "FDBK: " + std::to_string((int)(delayFDBK*100.00f)) +\
            " | SPACE: " + knobs.readKnobString(spaceKnob);
            dLines[3] = "WOW: " + knobs.readKnobString(wowKnob) +\
            " | BLEND: " + knobs.readKnobString(blendKnob);
            dLines[4] = "FILT: " + knobs.readKnobString(filtKnob) + " | STATE: " + displayStatus[outStatus];
            dLines[5] = "";
            display.print(dLines, 6);
        }
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

    filter.Init();
    filter.SetFrequency(0.497f);

    verb.Init(sample_rate);
    verb.SetLpFreq(18000.0f);

    tempo.Init(sample_rate, &hw, rateKnob, &presetManager, &knobs);

    buttons.Init(&hw);

    // Initialize LEDs - Mapped top to bottom from schematic
    LEDS[D0].Init(daisy::seed::D6, daisy::GPIO::Mode::OUTPUT, daisy::GPIO::Pull::NOPULL, daisy::GPIO::Speed::LOW);
    LEDS[D1].Init(daisy::seed::D5, daisy::GPIO::Mode::OUTPUT, daisy::GPIO::Pull::NOPULL, daisy::GPIO::Speed::LOW);
    LEDS[D2].Init(daisy::seed::D4, daisy::GPIO::Mode::OUTPUT, daisy::GPIO::Pull::NOPULL, daisy::GPIO::Speed::HIGH);
    LEDS[D3].Init(daisy::seed::D3, daisy::GPIO::Mode::OUTPUT, daisy::GPIO::Pull::NOPULL, daisy::GPIO::Speed::LOW);
    LEDS[D4].Init(daisy::seed::D2, daisy::GPIO::Mode::OUTPUT, daisy::GPIO::Pull::NOPULL, daisy::GPIO::Speed::LOW);
    LEDS[D5].Init(daisy::seed::D1, daisy::GPIO::Mode::OUTPUT, daisy::GPIO::Pull::NOPULL, daisy::GPIO::Speed::LOW);

    LEDS[D0].Write(effectOn);

    knobs.Init(&hw);
    // Initialize Send/StereoprogramSw

    // HIGH = Stereo, Low = Send (Low disables volume knob on output R)
    sendSw.Init(daisy::seed::D10, daisy::GPIO::Mode::OUTPUT, daisy::GPIO::Pull::NOPULL, daisy::GPIO::Speed::LOW);
    
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
    
    wetBlend = knobs.readKnob(blendKnob);

    // Set delay to value between 1 and 103ms, set metro to freq
    del.SetDelay(sample_rate * (delaytime / (float)divisions));
    
    // Set tick (1/delay time) * 2 * 4 (2 is so LED has rising edge every tick)
    if(abs(tick.GetFreq() - (2.0f/delaytime)) > 1.0f){
        tick.SetFreq(2.0f/delaytime); 
        tick.Reset();
    }
}

void updateReverb(){

    float min = presetManager.getReverbFDBKMin();
    float range = presetManager.getReverbFDBKRange();

    // Read and scale appropriate knobs
    // Truncates the pot reading to 2 decimal points
    float rFDBK = knobs.readKnob(fdbkKnob);
    rFDBK = min + (rFDBK * range); 
    delayFDBK = rFDBK;

    float rBlend = knobs.readKnob(spaceKnob);

    verb.SetFeedback(rFDBK);
    verbBlend = rBlend;
}

void updateFlutter(){

    // Read and scale appropriate knobs
    // Truncates the pot reading to 2 decimal points
    float flutterAm = knobs.readKnob(wowKnob);
    wow = flutterAm;

    flutter.SetLfoFreq(0.3f + flutterAm * 0.7f);
    flutter.SetLfoDepth(0.6f + flutterAm * 0.4f);
    flutter.SetDelay(0.5f + flutterAm * 0.5f);
    flutter.SetFeedback(0.3f + flutterAm * 0.4f);
}

void updateFilter(){
    
    // Read and scale appropriate knobs
    // Truncates the pot reading to 2 decimal points - 0.01 to 1.00
    float filterKnob = knobs.readKnob(filtKnob, 0.01f);

    // Filter knob at noon is no filter
    // Filter knob to left is Low Pass
    // Filter knob to right is High Pass

    if(filterKnob < 0.48){
        filter.SetFilterMode(daisysp::OnePole::FILTER_MODE_LOW_PASS);
        filtFreq = filterKnob * 0.3;    // Magic number that sounds good
        filtEnable = true;
    }else if (filterKnob > 0.52){
        filter.SetFilterMode(daisysp::OnePole::FILTER_MODE_HIGH_PASS);
        filtFreq = (filterKnob - 0.52) * 0.1;    // Magic number that sounds good
        filtEnable = true;
    }else{
        filtEnable = false;
    }

    filter.SetFrequency(filtFreq);
}