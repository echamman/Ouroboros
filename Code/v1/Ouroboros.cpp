#include "daisysp.h"
#include "daisy_seed.h"
#include "src/OLED.h"

// Interleaved audio definitions
#define LEFT (i)
#define RIGHT (i + 1)

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

// Declare a DaisySeed object called hardware
DaisySeed hw;

AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
Switch buttons[NUM_SWITCHES];

// Declare a DelayLine of MAX_DELAY number of floats.
static DelayLine<float, MAX_DELAY> del;

// Create ReverbSc, load it to SDRAM
static ReverbSc DSY_SDRAM_BSS verb;
float verbBlend;

// Metro 
static Metro tick;

// OLED Screen 
static Oled display;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float feedback, del_out, sig_out, verb_outL, verb_outR;

    for(size_t i = 0; i < size; i += 2)
    {
        // Read from delay line
        del_out = del.Read();
        // Calculate output and feedback
        sig_out  = del_out + in[i];
        feedback = (del_out * 0.75f) + in[i];

        // Write to the delay
        del.Write(feedback);

        // Reverb writes to output
        verb.Process(sig_out, sig_out, &verb_outL, &verb_outR);

        // Output
        out[LEFT]  = (1-verbBlend)*sig_out + verbBlend*verb_outL;
        out[RIGHT] = (1-verbBlend)*sig_out + verbBlend*verb_outR;
    }
}

int main(void)
{
    // Declare a variable to store the state we want to set for the LED.
    bool led_state;
    led_state = true;

    string dLines[6]; //Create an Array of strings for the OLED display

    // initialize seed hardware and daisysp modules
    float sample_rate;
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

    // Initialize Metro at 100Hz
    tick.Init(100, sample_rate);

    // start callback
    hw.adc.Start();
    hw.StartAudio(AudioCallback);

    //Allow the OLED to start up
    System::Delay(100);
    /** And Initialize */
    display.Init(&hw);
    System::Delay(2000);

    // Loop forever
    for(;;)
    {
        // Set the onboard LED
        hw.SetLed(led_state);

        //Debounce the button
        buttons[Switch0].Debounce();

        // Toggle the LED state for the next time around.
        led_state = !led_state;

        // Set delay to value between 1 and 103ms, set metro to freq
        del.SetDelay(sample_rate * (hw.adc.GetFloat(Knob0)+0.03f));
        tick.SetFreq(sample_rate * (hw.adc.GetFloat(Knob0)+0.03f));
        
        verb.SetFeedback(hw.adc.GetFloat(Knob1));
        verbBlend = hw.adc.GetFloat(Knob2);

        //Print to display
        dLines[0] = "Knob 0: " + std::to_string((int)floor(hw.adc.GetFloat(Knob0)*100.00f));
        dLines[1] = "Knob 1: " + std::to_string((int)floor(hw.adc.GetFloat(Knob1)*100.00f));
        dLines[2] = "Knob 2: " + std::to_string((int)floor(hw.adc.GetFloat(Knob2)*100.00f));
        dLines[3] = "Knob 3: " + std::to_string((int)floor(hw.adc.GetFloat(Knob3)*100.00f));
        (buttons[Switch0].Pressed() ? dLines[4] = "Button: true" : dLines[4] = "Button: false");
        dLines[5] = "";
        display.print(dLines, 6);

        if(tick.Process()){
            led_state = !led_state;
        }
    }
}
