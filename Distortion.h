/*
  ==============================================================================

    Distortion.h
    Created: 31 Mar 2022 11:33:06am
    Author:  Davis Polito

  ==============================================================================
*/

#pragma once
#include "../Constants.h"
#include "Utilities.h"

class ElectroAudioProcessor;

class Distortion : public AudioComponent
{
public:
    //==============================================================================
    Distortion(const String&, ElectroAudioProcessor&, AudioProcessorValueTreeState&);
    ~Distortion();
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void frame();
    void tick(float* samples);
    
private:
    
    void (Distortion::*DistortionTick)(float& sample, int v, float cutoff, float q, float morph, float gain);
    
    std::atomic<float>* afpDistortionType;
    DistortionType currentDistortionType = DistortionTypeNil;
};
