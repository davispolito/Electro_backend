/*
  ==============================================================================

    Output.cpp
    Created: 17 Jun 2021 10:57:13pm
    Author:  Matthew Wang

  ==============================================================================
*/

#include "Output.h"
#include "../PluginProcessor.h"

//==============================================================================
Output::Output(const String& n, ElectroAudioProcessor& p,
               AudioProcessorValueTreeState& vts) :
AudioComponent(n, p, vts, cOutputParams, false)
{
    master = std::make_unique<SmoothedParameter>(processor, vts, "Master");

}

Output::~Output()
{
   
}

void Output::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    AudioComponent::prepareToPlay(sampleRate, samplesPerBlock);
}

void Output::frame()
{
    sampleInBlock = 0;
}

void Output::tick(float input[MAX_NUM_VOICES])
{
//    float a = sampleInBlock * invBlockSize;
    
    for (int v = 0; v < processor.numVoicesActive; ++v)
    {
        float amp = quickParams[OutputAmp][v]->tick();
        amp = amp < 0.f ? 0.f : amp;
        input[v] = input[v] * amp;
        
        // Porting over some code from
        // https://github.com/juce-framework/JUCE/blob/master/modules/juce_dsp/processors/juce_Panner.cpp
       
    }
    
    float pedGain = 1.f;
    if (processor.pedalControlsMaster)
    {
        // this is to clip the gain settings so all the way down on the pedal isn't actually
        // off, it let's a little signal through. Would be more efficient to fix the table to
        // span a better range.
        float volumeSmoothed = processor.ccParams.getLast()->get();
        float volIdx = LEAF_clip(47.0f, ((volumeSmoothed * 80.0f) + 47.0f), 127.0f);
        
        //then interpolate the value
        int volIdxInt = (int) volIdx;
        float alpha = volIdx-volIdxInt;
        int volIdxIntPlus = (volIdxInt + 1) & 127;
        float omAlpha = 1.0f - alpha;
        pedGain = volumeAmps128[volIdxInt] * omAlpha;
        pedGain += volumeAmps128[volIdxIntPlus] * alpha;
    }
    
    sampleInBlock++;
}
