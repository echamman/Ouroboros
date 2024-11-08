/*
    Defines presets available for delay and reverb
*/

#include <stdio.h>
#include <string>

enum presetNames {
    def = 0,
    slapback,
    NUM_PRESETS
};

struct preset{
    std::string name;
    float delaytimeMin;     // in s
    float delaytimeMax;     // in s
    float reverbFDBKMin;    // 0 - 0.99
    float reverbFDBKMax;    // 0-0.99
    float HP;               // freq
    float LP;               // freq
    float flutter;          // Chorus effect
};

class presets{
    private:
        preset presetArr[NUM_PRESETS] = {
            {
                "default",
                0.03f,
                1.0f,
                0.0f,
                0.99f,
                0.0f,
                20000.0f,
                1.0f
            },
            {
                "slapback",
                0.03f,
                0.3f,
                0.0f,
                0.4f,
                0.0f,
                20000.0f,
                1.0f
            },
        };

        int currentPreset = 0;

    public:

        void setPreset(int presetNum){
            currentPreset = presetNum;
        }

        void nextPreset(){
            currentPreset = (currentPreset + 1) % NUM_PRESETS;
        }

        void prevPreset(){
            currentPreset = (currentPreset - 1) % NUM_PRESETS;
        }

        std::string getPresetName(){
            return presetArr[currentPreset].name;
        }

        float getDelayMin(){
            return presetArr[currentPreset].delaytimeMin;
        }

        float getDelayMax(){
            return presetArr[currentPreset].delaytimeMax;
        }

        float getDelayRange(){
            return presetArr[currentPreset].delaytimeMax - presetArr[currentPreset].delaytimeMin;
        }

        float getReverbFDBKMin(){
            return presetArr[currentPreset].reverbFDBKMin;
        }

        float getReverbFDBKMax(){
            return presetArr[currentPreset].reverbFDBKMax;
        }

        float getReverbFDBKRange(){
            return presetArr[currentPreset].reverbFDBKMax - presetArr[currentPreset].reverbFDBKMin;
        }

        float getLP(){
            return presetArr[currentPreset].LP;
        }

        float getHP(){
            return presetArr[currentPreset].HP;
        }

        float getFlutter(){
            return presetArr[currentPreset].flutter;
        }
};