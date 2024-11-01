#include "daisysp.h"
#include "daisy_seed.h"

// Interleaved audio definitions
#define LEFT (i)
#define RIGHT (i + 1)

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 0.75f)

// Use the daisy namespace to prevent having to type
// daisy:: before all libdaisy functions
using namespace daisysp;
using namespace daisy;

// Declare a DaisySeed object called hardware
DaisySeed hw;

AdcChannelConfig adcConfig[1];

// Helper Modules
static AdEnv      env;
static Oscillator osc;
static Metro      tick;  

// Declare a DelayLine of MAX_DELAY number of floats.
static DelayLine<float, MAX_DELAY> del;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float feedback, del_out, sig_out;
    for(size_t i = 0; i < size; i += 2)
    {
        // Read from delay line
        del_out = del.Read();
        // Calculate output and feedback
        sig_out  = del_out + in[i];
        feedback = (del_out * 0.75f) + in[i];

        // Write to the delay
        del.Write(feedback);

        // Output
        out[LEFT]  = sig_out;
        out[RIGHT] = sig_out;
    }
}

int main(void)
{
    // Declare a variable to store the state we want to set for the LED.
    bool led_state;
    led_state = true;

    // initialize seed hardware and daisysp modules
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();
    del.Init();

    adcConfig[0].InitSingle(hw.GetPin(21));

    //Initialize the adc with the config we just made
    hw.adc.Init(adcConfig, 1);

    // Set up Metro to pulse every second
    tick.Init(1.0f, sample_rate);

    // Set Delay time to 0.75 seconds
    del.SetDelay(sample_rate * 0.2f);

    // start callback
    hw.adc.Start();
    hw.StartAudio(AudioCallback);

    uint32_t potVal;

    // Loop forever
    for(;;)
    {
        // Set the onboard LED
        hw.SetLed(led_state);

        // Toggle the LED state for the next time around.
        led_state = !led_state;

        potVal = (int)floor(hw.adc.GetFloat(0)*100.00f);

        del.SetDelay(sample_rate * (hw.adc.GetFloat(0)+0.02f));

        // Wait 500ms
        // System::Delay((uint32_t)floor(hw.adc.GetFloat(0)*500.0f));
        System::Delay(potVal);
        System::Delay(50);
    }
}
