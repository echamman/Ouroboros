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
    float osc_out, env_out, feedback, del_out, sig_out;
    for(size_t i = 0; i < size; i += 2)
    {
        // When the Metro ticks:
        // trigger the envelope to start, and change freq of oscillator.
        if(tick.Process())
        {
            float freq = rand() % 200;
            osc.SetFreq(freq + 100.0f);
            env.Trigger();
        }

        // Use envelope to control the amplitude of the oscillator.
        env_out = env.Process();
        osc.SetAmp(env_out);
        osc_out = osc.Process();

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
    env.Init(sample_rate);
    osc.Init(sample_rate);
    del.Init();

    // Set up Metro to pulse every second
    tick.Init(1.0f, sample_rate);

    // set adenv parameters
    env.SetTime(ADENV_SEG_ATTACK, 0.001);
    env.SetTime(ADENV_SEG_DECAY, 0.50);
    env.SetMin(0.0);
    env.SetMax(0.25);
    env.SetCurve(0); // linear

    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_TRI);
    osc.SetFreq(220);
    osc.SetAmp(0.25);

    // Set Delay time to 0.75 seconds
    del.SetDelay(sample_rate * 0.2f);


    // start callback
    hw.StartAudio(AudioCallback);

    // Loop forever
    for(;;)
    {
        // Set the onboard LED
        hw.SetLed(led_state);

        // Toggle the LED state for the next time around.
        led_state = !led_state;

        // Wait 500ms
        System::Delay(500);
    }
}
