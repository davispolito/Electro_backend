/*
 ==============================================================================
 
 Filters.cpp
 Created: 17 Mar 2021 3:10:49pm
 Author:  Matthew Wang
 
 ==============================================================================
 */

#include "Filters.h"
#include "../PluginProcessor.h"

//==============================================================================
Filter::Filter(const String& n, ElectroAudioProcessor& p,
                             AudioProcessorValueTreeState& vts) :
AudioComponent(n, p, vts, cFilterParams, true)
{    
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tSVF_init(&lowpass[i], SVFTypeLowpass, 2000.f, 0.7f, &processor.leaf);
        tSVF_init(&highpass[i], SVFTypeHighpass, 2000.f, 0.7f, &processor.leaf);
        tSVF_init(&bandpass[i], SVFTypeBandpass, 2000.f, 0.7f, &processor.leaf);
        tDiodeFilter_init(&diodeFilters[i], 2000.f, 1.0f, &processor.leaf);
        tVZFilter_init(&VZfilterPeak[i], Bell, 2000.f, 1.0f, &processor.leaf);
        tVZFilter_init(&VZfilterLS[i], Lowshelf, 2000.f, 1.0f, &processor.leaf);
        tVZFilter_init(&VZfilterHS[i], Highshelf, 2000.f, 1.0f, &processor.leaf);
         tVZFilter_init(&VZfilterBR[i], BandReject, 2000.f, 1.0f, &processor.leaf);
        tLadderFilter_init(&Ladderfilter[i], 2000.f, 1.0f, &processor.leaf);
    }
    
    afpFilterType = vts.getRawParameterValue(n + " Type");
}

Filter::~Filter()
{
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tSVF_free(&lowpass[i]);
        tSVF_free(&highpass[i]);
        tSVF_free(&bandpass[i]);
        tDiodeFilter_free(&diodeFilters[i]);
        tVZFilter_free(&VZfilterPeak[i]);
        tVZFilter_free(&VZfilterLS[i]);
        tVZFilter_free(&VZfilterHS[i]);
        tVZFilter_free(&VZfilterBR[i]);
        tLadderFilter_free(&Ladderfilter[i]);
    }
}

void Filter::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    AudioComponent::prepareToPlay(sampleRate, samplesPerBlock);
}
//refactor to change function pointer when the type is selected rather than checking at every frame

void Filter::frame()
{
    sampleInBlock = 0;
    enabled = afpEnabled == nullptr || *afpEnabled > 0;
    
    currentFilterType = FilterType(int(*afpFilterType));
    switch (currentFilterType) {
        case LowpassFilter:
            filterTick = &Filter::lowpassTick;
            break;
            
        case HighpassFilter:
            filterTick = &Filter::highpassTick;
            break;
            
        case BandpassFilter:
            filterTick = &Filter::bandpassTick;
            break;
            
        case DiodeLowpassFilter:
            filterTick = &Filter::diodeLowpassTick;
            break;
            
        case VZPeakFilter:
            filterTick = &Filter::VZpeakTick;
            break;
            
        case VZLowshelfFilter:
            filterTick = &Filter::VZlowshelfTick;
            break;
            
        case VZHighshelfFilter:
            filterTick = &Filter::VZhighshelfTick;
            break;
            
        case VZBandrejectFilter:
            filterTick = &Filter::VZbandrejectTick;
            break;
            
        case LadderLowpassFilter:
            filterTick = &Filter::LadderLowpassTick;
            break;
            
        default:
            filterTick = &Filter::lowpassTick;
            break;
    }
}

void Filter::tick(float* samples)
{
    if (!enabled) return;
    
//    float a = sampleInBlock * invBlockSize;
    
    for (int v = 0; v < processor.numVoicesActive; ++v)
    {
        float midiCutoff = quickParams[FilterCutoff][v]->tickNoSmoothing();
        float keyFollow = quickParams[FilterKeyFollow][v]->tickNoSmoothing();
        float q = quickParams[FilterResonance][v]->tickNoSmoothing();
        float gain = quickParams[FilterGain][v]->tickNoSmoothing();
        
        LEAF_clip(0.f, keyFollow, 1.f);
        
        float follow = processor.voiceNote[v];
        if (isnan(follow))
        {
            follow = 0.0f;
        }

        float cutoff = midiCutoff + (follow * keyFollow);
        cutoff = fabsf(mtof(cutoff));
        
        q = q < 0.1f ? 0.1f : q;
        
        (this->*filterTick)(samples[v], v, cutoff, q, keyFollow, gain);
    }
    
    sampleInBlock++;
}

void Filter::lowpassTick(float& sample, int v, float cutoff, float q, float morph, float gain)
{
    tSVF_setFreqAndQ(&lowpass[v], cutoff, q);
    sample = tSVF_tick(&lowpass[v], sample);
    sample *= dbtoa(gain);
}

void Filter::highpassTick(float& sample, int v, float cutoff, float q, float morph, float gain)
{
    //tVZFilter_setMorphOnly(&filters[v], morph);
    //tVZFilter_setFreqAndBandwidth(&filters[v], cutoff + 200.0f, q + 0.01f);
    //sample = tVZFilter_tick(&filters[v], sample);
    tSVF_setFreqAndQ(&highpass[v], cutoff, q);
    sample = tSVF_tick(&highpass[v], sample);
    sample *= dbtoa(gain);
}

void Filter::bandpassTick(float& sample, int v, float cutoff, float q, float morph, float gain)
{
    //tVZFilter_setMorphOnly(&filters[v], morph);
    //tVZFilter_setFreqAndBandwidth(&filters[v], cutoff + 200.0f, q + 0.01f);
    //sample = tVZFilter_tick(&filters[v], sample);
    tSVF_setFreqAndQ(&bandpass[v], cutoff, q);
    sample = tSVF_tick(&bandpass[v], sample);
    sample *= dbtoa(gain);
}

void Filter::diodeLowpassTick(float& sample, int v, float cutoff, float q, float morph, float gain)
{
    tDiodeFilter_setFreq(&diodeFilters[v], cutoff);
    tDiodeFilter_setQ(&diodeFilters[v], q);
    sample = tDiodeFilter_tick(&diodeFilters[v], sample);
    sample *= dbtoa(gain);
}

void Filter::VZpeakTick(float& sample, int v, float cutoff, float q, float morph, float gain)
{
    //tVZFilter_setMorphOnly(&filters[v], morph);
    tVZFilter_setFrequencyAndBandwidthAndGain(&VZfilterPeak[v], cutoff, q, dbtoa((gain * 50.f) - 25.f));
    sample = tVZFilter_tickEfficient(&VZfilterPeak[v], sample);
}

void Filter::VZlowshelfTick(float& sample, int v, float cutoff, float q, float morph, float gain)
{
    //tVZFilter_setMorphOnly(&filters[v], morph);
    //tVZFilter_setFreq(&VZfilterLS[v], cutoff);
    tVZFilter_setFrequencyAndResonanceAndGain(&VZfilterLS[v], cutoff, q, fasterdbtoa((gain * 50.f) - 25.0f));
    sample = tVZFilter_tickEfficient(&VZfilterLS[v], sample);
}
void Filter::VZhighshelfTick(float& sample, int v, float cutoff, float q, float morph, float gain)
{
    //tVZFilter_setMorphOnly(&filters[v], morph);
    
    tVZFilter_setFrequencyAndResonanceAndGain(&VZfilterHS[v], cutoff, q, fasterdbtoa((gain * 50.0f) - 25.0f));
    sample = tVZFilter_tickEfficient(&VZfilterHS[v], sample);
}
void Filter:: VZbandrejectTick(float& sample, int v, float cutoff, float q, float morph, float gain)
{
    //tVZFilter_setMorphOnly(&filters[v], morph);
    tVZFilter_setFrequencyAndResonanceAndGain(&VZfilterBR[v], cutoff, q, 1);
    sample = tVZFilter_tickEfficient(&VZfilterBR[v], sample);
    sample *= dbtoa(gain);
}

void Filter:: LadderLowpassTick(float& sample, int v, float cutoff, float q, float morph, float gain)
{
    //tVZFilter_setMorphOnly(&filters[v], morph);
    tLadderFilter_setFreq(&Ladderfilter[v], cutoff);
    tLadderFilter_setQ(&Ladderfilter[v], q);
    sample = tLadderFilter_tick(&Ladderfilter[v], sample);
    sample *= dbtoa(gain);
}
