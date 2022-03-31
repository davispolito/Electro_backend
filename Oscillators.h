/*
  ==============================================================================

    Oscillators.h
    Created: 17 Mar 2021 2:49:31pm
    Author:  Matthew Wang

  ==============================================================================
*/

#pragma once

#include "../Constants.h"
#include "Utilities.h"

class ElectroAudioProcessor;

//==============================================================================

class Oscillator : public AudioComponent,
                   public MappingSourceModel
{
public:
    //==============================================================================
    Oscillator(const String&, ElectroAudioProcessor&, AudioProcessorValueTreeState&);
    ~Oscillator();
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void frame();
    void tick(float output[][MAX_NUM_VOICES]);
    
    //==============================================================================
    void setWaveTables(File file);
    File& getWaveTableFile() { return waveTableFile; }
    void setMtoF(float mn);
    OscShapeSet getCurrentShapeSet() { return currentShapeSet; }
    
private:
    void (Oscillator::*shapeTick)(float& sample, int v, float freq, float shape);
    void sawSquareTick(float& sample, int v, float freq, float shape);
    void sineTriTick(float& sample, int v, float freq, float shape);
    void sawTick(float& sample, int v, float freq, float shape);
    void pulseTick(float& sample, int v, float freq, float shape);
    void sineTick(float& sample, int v, float freq, float shape);
    void triTick(float& sample, int v, float freq, float shape);
    void userTick(float& sample, int v, float freq, float shape);
    
    tMBSaw saw[MAX_NUM_VOICES];
    tMBPulse pulse[MAX_NUM_VOICES];
    tCycle sine[MAX_NUM_VOICES];
    tMBTriangle tri[MAX_NUM_VOICES];
    
    // Using seperate objects for pairs to easily maintain phase relation
    tMBSaw sawPaired[MAX_NUM_VOICES];
    tMBPulse pulsePaired[MAX_NUM_VOICES];
    tCycle sinePaired[MAX_NUM_VOICES];
    tMBTriangle triPaired[MAX_NUM_VOICES];
    
    tWaveOscS wave[MAX_NUM_VOICES];
    
    float* sourceValues[MAX_NUM_UNIQUE_SKEWS];

    std::unique_ptr<SmoothedParameter> filterSend;
    std::atomic<float>* isHarmonic_raw;
    std::atomic<float>* isStepped_raw;

    std::atomic<float>* afpShapeSet;
    OscShapeSet currentShapeSet = OscShapeSetNil;
    
    float outSamples[2][MAX_NUM_VOICES];
    
    File waveTableFile;
    bool loadingTables = false;
};

//==============================================================================

class LowFreqOscillator : public AudioComponent,
                          public MappingSourceModel
{
public:
    //==============================================================================
    LowFreqOscillator(const String&, ElectroAudioProcessor&, AudioProcessorValueTreeState&);
    ~LowFreqOscillator();
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void frame();
    void tick();
    
    //==============================================================================
    void noteOn(int voice, float velocity);
    void noteOff(int voice, float velocity);
    
    //==============================================================================
    LFOShapeSet getCurrentShapeSet() { return currentShapeSet; }
    
private:
    
    void (LowFreqOscillator::*shapeTick)(float& sample, int v, float freq, float shape);
    void sawSquareTick(float& sample, int v, float freq, float shape);
    void sineTriTick(float& sample, int v, float freq, float shape);
    void userTick(float& sample, int v, float freq, float shape);
    void sawTick(float& sample, int v, float freq, float shape);
    void pulseTick(float& sample, int v, float freq, float shape);
    void sineTick(float& sample, int v, float freq, float shape);
    void triTick(float& sample, int v, float freq, float shape);
    
    RangedAudioParameter* sync;
    
    tMBSaw saw[MAX_NUM_VOICES];
    tMBPulse pulse[MAX_NUM_VOICES];
    tCycle sine[MAX_NUM_VOICES];
    tMBTriangle tri[MAX_NUM_VOICES];
    
    // Using seperate objects for pairs to easily maintain phase relation
    tMBSaw sawPaired[MAX_NUM_VOICES];
    tMBPulse pulsePaired[MAX_NUM_VOICES];
    tCycle sinePaired[MAX_NUM_VOICES];
    tMBTriangle triPaired[MAX_NUM_VOICES];
    
    tWaveOscS wave[MAX_NUM_VOICES];
    
    float* sourceValues[MAX_NUM_UNIQUE_SKEWS];
    float phaseReset;
    
    std::atomic<float>* afpShapeSet;
    LFOShapeSet currentShapeSet = LFOShapeSetNil;
};

//==============================================================================

class NoiseGenerator : public AudioComponent,
public MappingSourceModel
{
public:
    //==============================================================================
    NoiseGenerator(const String&, ElectroAudioProcessor&, AudioProcessorValueTreeState&);
    ~NoiseGenerator();
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void frame();
    void tick(float output[][MAX_NUM_VOICES]);
    
private:
    
    tNoise noise[MAX_NUM_VOICES];
    tSVF bandpass[MAX_NUM_VOICES];
    
    std::unique_ptr<SmoothedParameter> filterSend;
    
    float* sourceValues[MAX_NUM_UNIQUE_SKEWS];
};
