/*
  ==============================================================================

    Filters.h
    Created: 17 Mar 2021 3:10:49pm
    Author:  Matthew Wang

  ==============================================================================
*/

#pragma once

#include "../Constants.h"
#include "Utilities.h"

class ElectroAudioProcessor;

class Filter : public AudioComponent
{
public:
    //==============================================================================
    Filter(const String&, ElectroAudioProcessor&, AudioProcessorValueTreeState&);
    ~Filter();
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void frame();
    void tick(float* samples);
    
private:
    
    void (Filter::*filterTick)(float& sample, int v, float cutoff, float q, float morph);
    void lowpassTick(float& sample, int v, float cutoff, float q, float morph);
    void highpassTick(float& sample, int v, float cutoff, float q, float morph);
    void bandpassTick(float& sample, int v, float cutoff, float q, float morph);
    void diodeLowpassTick(float& sample, int v, float cutoff, float q, float morph);
    void LadderLowpassTick(float& sample, int v, float cutoff, float q, float morph);
    void VZlowshelfTick(float& sample, int v, float cutoff, float q, float morph);
    void VZhighshelfTick(float& sample, int v, float cutoff, float q, float morph);
    void VZpeakTick(float& sample, int v, float cutoff, float q, float morph);
    void VZbandrejectTick(float& sample, int v, float cutoff, float q, float morph);

    
    tDiodeFilter diodeFilters[NUM_STRINGS];
    tVZFilter VZfilterPeak[NUM_STRINGS];
    tVZFilter VZfilterLS[NUM_STRINGS];
    tVZFilter VZfilterHS[NUM_STRINGS];
    tVZFilter VZfilterBR[NUM_STRINGS];
    tSVF lowpass[NUM_STRINGS];
    tSVF highpass[NUM_STRINGS];
    tSVF bandpass[NUM_STRINGS];
    tLadderFilter Ladderfilter[NUM_STRINGS];
    
    
    std::atomic<float>* afpFilterType;
    FilterType currentFilterType = FilterTypeNil;
};
