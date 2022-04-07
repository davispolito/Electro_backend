/*
  ==============================================================================

    Output.h
    Created: 17 Jun 2021 10:57:13pm
    Author:  Matthew Wang

  ==============================================================================
*/

#pragma once

#include "../Constants.h"
#include "Utilities.h"
#define MASTER_OVERSAMPLE 4
//==============================================================================

class Output : public AudioComponent
{
public:
    //==============================================================================
    Output(const String&, ElectroAudioProcessor&, AudioProcessorValueTreeState&);
    ~Output();
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void frame();
    void tick(float input[MAX_NUM_VOICES], float output[2], int numChannels);
    
private:
    
//    std::atomic<float>* afpDistortionType;
//    DistortionType currentDistortionType = distnil;
    std::unique_ptr<SmoothedParameter> master;
    tOversampler os[2];
    float oversamplerArray[MASTER_OVERSAMPLE];
};

