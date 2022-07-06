/*
  ==============================================================================

    Effect.h
    Created: 25 May 2022 2:45:37pm
    Author:  Davis Polito

  ==============================================================================
*/

#pragma once
#include "../Constants.h"
#include "Utilities.h"

class Effect : public AudioComponent
{
public:
    //==============================================================================
    Effect(const String&, ElectroAudioProcessor&, AudioProcessorValueTreeState&);
    ~Effect();
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock)
    {
        AudioComponent::prepareToPlay(sampleRate, samplesPerBlock);
    }
    
    ElectroAudioProcessor* getProcessor() {return &processor;}    float tick(float sample, float param1, float param2, float param3, float param4, float param5, int v);
    void setTick(FXType);
    void frame();
    void oversample_tick(float* samples, int v);
    
private:
    typedef float (Effect::*EffectTick)(float sample, float param1, float param2, float param3, float param4, float param5, int v);
    static EffectTick typeToTick(FXType type);
    EffectTick _tick;
    tHighpass dcBlock1[MAX_NUM_VOICES];
    tHighpass dcBlock2[MAX_NUM_VOICES];
    tVZFilter shelf1[MAX_NUM_VOICES], shelf2[MAX_NUM_VOICES], bell1[MAX_NUM_VOICES];
    tCompressor comp[MAX_NUM_VOICES];
    tCrusher bc[MAX_NUM_VOICES];
    tLockhartWavefolder wf[MAX_NUM_VOICES];
    tHermiteDelay delay1[MAX_NUM_VOICES];
    tHermiteDelay delay2[MAX_NUM_VOICES];
    tCycle mod1[MAX_NUM_VOICES];
    tCycle mod2[MAX_NUM_VOICES];
    float tiltFilterTick(float sample, float param1, float param2, float param3, float param4, float param5,int v);
    float hardClipTick(float sample, float param1, float param2, float param3, float param4, float param5,int v);
    //void (Effect::*effectTick)(float& sample, int v, float param1, float param2, float param3, float mix);
    float tanhTick(float sample, float param1, float param2, float param3, float param4, float param5, int v);
    float softClipTick(float sample, float param1, float param2, float param3, float param4, float param5, int v);
    float satTick(float sample, float param1, float param2, float param3, float param4, float param5, int v);
    float bcTick(float sample, float param1, float param2, float param3, float param4, float param5, int v);
    float compressorTick(float sample, float param1, float param2, float param3, float param4, float param5, int v);
    float shaperTick(float sample, float param1, float param2, float param3, float param4, float param5, int v);
    float wavefolderTick(float sample, float param1, float param2, float param3, float param4, float param5, int v);
    float chorusTick(float sample, float param1, float param2, float param3, float param4, float param5, int v);
    float inv_oversample;
    std::atomic<float>* afpFXType;
    int sampleInBlock;
};
