/*
  ==============================================================================

    Effect.cpp
    Created: 25 May 2022 2:45:37pm
    Author:  Davis Polito

  ==============================================================================
*/

#include "Effect.h"
#include "../PluginProcessor.h"
Effect::Effect(const String& n, ElectroAudioProcessor& p,
               AudioProcessorValueTreeState& vts) : AudioComponent(n, p, vts, cFXParams, false)
{
    _tick = typeToTick(Softclip);
    int temp = processor.leaf.clearOnAllocation;
    getProcessor()->leaf.clearOnAllocation = 1;

    
    getProcessor()->leaf.clearOnAllocation = temp;
    afpFXType = vts.getRawParameterValue(n + " FXType");
    inv_oversample = 1.0f / OVERSAMPLE;
    for (int i = 0; i < MAX_NUM_VOICES; i++){
        tCrusher_init(&bc[i],&getProcessor()->leaf);
        tHighpass_init(&dcBlock1[i], 5.0f,&getProcessor()->leaf);
        tHighpass_init(&dcBlock2[i], 5.0f,&getProcessor()->leaf);
        tVZFilter_init(&shelf1[i], Lowshelf, 80.0f, 32.0f,  &getProcessor()->leaf);
        tVZFilter_init(&shelf2[i], Highshelf, 12000.0f, 32.0f, &getProcessor()->leaf);
        tVZFilter_init(&bell1[i], Bell, 1000.0f, 1.9f,  &getProcessor()->leaf);
        tVZFilter_setSampleRate(&shelf1[i], getProcessor()->leaf.sampleRate * OVERSAMPLE);
        tVZFilter_setSampleRate(&shelf2[i], getProcessor()->leaf.sampleRate * OVERSAMPLE);
        tVZFilter_setSampleRate(&bell1[i], getProcessor()->leaf.sampleRate * OVERSAMPLE);
        tCompressor_init(&comp[i], &getProcessor()->leaf);
        tLockhartWavefolder_init(&wf[i], &getProcessor()->leaf);
        tHermiteDelay_init(&delay1[i], 6000.0f, 10000, &getProcessor()->leaf);
        tHermiteDelay_init(&delay2[i], 6000.0f, 10000, &getProcessor()->leaf);
        tCycle_init(&mod1[i], &getProcessor()->leaf);
        tCycle_init(&mod2[i], &getProcessor()->leaf);
        tCycle_setFreq(&mod1[i], 0.2f);
        tCycle_setFreq(&mod2[i], 0.22222222222f);
    }
}

Effect::~Effect()
{
    
}

//void Effect::setTick(FXType t)
//{
//    _tick = typeToTick(t);
//}





void Effect::frame()
{
    sampleInBlock = 0;
    
    _tick = typeToTick(FXType(int(*afpFXType)));
}

Effect::EffectTick Effect::typeToTick(FXType type)
{
    switch(type)
    {
        case None:
            return &Effect::tick;
            break;
        case Softclip:
            return &Effect::softClipTick;
        case ABSaturator:
            return &Effect::satTick;
        break;
        case Tanh:
            return &Effect::tanhTick;
        break;
        case Shaper:
            return &Effect::shaperTick;
        break;
        case Compressor:
            return &Effect::compressorTick;
        break;
        case Chorus:
            return &Effect::chorusTick;
        break;
        case Bitcrush:
            return &Effect::bcTick;
        break;
        case TiltFilter:
            return &Effect::tiltFilterTick;
        break;
        case Wavefolder:
            return &Effect::wavefolderTick;
        break;
        case FXTypeNil:
            return &Effect::tick;
        break;
    }
}

float Effect::wavefolderTick(float sample, float param1, float param2, float param3, float param4, float param5, int v)
{
    float gain = dbtoa(param1 * 12.0f);
    sample = sample * gain;
    
    float temp = tLockhartWavefolder_tick(&wf[v],(sample + (param2 * gain)));
    
    temp = tHighpass_tick(&dcBlock1[v], temp);
    return temp;
}

float Effect::chorusTick(float sample, float param1, float param2, float param3, float param4, float param5, int v)
{
    float baseLength = param1 * 7000.0f + 10.0f;
    float modDepth = param2 * 0.1f;
    float modSpeed1 = param3 * 0.4f + 0.01f;
    float modSpeed2 = param4 * 0.4444444f + 0.01f;
    tCycle_setFreq(&mod1[v], modSpeed1);
    tCycle_setFreq(&mod2[v], modSpeed2);
    
    tHermiteDelay_setDelay(&delay1[v], baseLength * .707f * (1.0f + modDepth * tCycle_tick(&mod1[v])));
    tHermiteDelay_setDelay(&delay2[v], baseLength * .5f * (1.0f - modDepth * tCycle_tick(&mod2[v])));
    float temp = tHermiteDelay_tick(&delay1[v], sample) - sample;
    temp += tHermiteDelay_tick(&delay2[v], sample) - sample;
    
    //temp = tHighpass_tick(&dcBlock1[v], temp);
    return temp;
}

float Effect::shaperTick(float sample, float param1, float param2, float param3, float param4, float param5, int v)
{
    float gain = dbtoa(param1 * 24.0f);
    sample = sample * gain;
    float temp = LEAF_shaper(sample + (param2 * gain),param3);
    temp = tHighpass_tick(&dcBlock1[v], temp);
    return temp;
}

float Effect::tick(float sample, float param1, float param2, float param3, float param4, float param5, int v)
{
    return sample;
}

float Effect::tiltFilterTick(float sample, float param1, float param2, float param3, float param4, float param5, int v)
{
    tVZFilter_setGain(&shelf1[v], fastdbtoa(-1.0f * ((param1 * 30.0f) - 15.0f)));
    tVZFilter_setGain(&shelf2[v], fastdbtoa((param1 * 30.0f) - 15.0f));
    tVZFilter_setFrequencyAndBandwidthAndGain(&bell1[v], faster_mtof(param2 * 77.0f + 42.0f), (param3 +1.0f)*6.0f, fastdbtoa((param4* 34.0f) - 17.0f));
    sample = tVZFilter_tickEfficient(&shelf1[v], sample);
    sample = tVZFilter_tickEfficient(&shelf2[v], sample);
    sample = tVZFilter_tickEfficient(&bell1[v], sample);
    return sample;
}

void Effect::oversample_tick(float* samples, int v)
{
    float param1 = quickParams[Param1][v]->tick();
    float param2 = quickParams[Param2][v]->tick();
    float param3 = quickParams[Param3][v]->tick();
    float param4 = quickParams[Param4][v]->tick();
    float param5 = quickParams[Param5][v]->tick();
    float mix = quickParams[Mix][v]->tick();
    for(int i = 0; i < OVERSAMPLE; i++)
    {
        float output = (this->*_tick)((samples[i]), param1, param2, param3, param4, param5, v);
        samples[i] = ((1.0f - mix) * (samples[i])) + (mix * output);
    }
    sampleInBlock++;
}

float Effect::tanhTick(float sample, float param1, float param2, float param3, float param4, float param5, int v)
{
    float gain = dbtoa(param1 * 24.0f);
    sample = sample * gain;
    float temp = tanhf(sample + (param2 * gain));
    temp = tHighpass_tick(&dcBlock1[v], temp);
    temp = tanhf(temp);
    //temp = tHighpass_tick(&dcBlock2, temp);
    return temp;
}

float Effect::softClipTick(float sample, float param1, float param2, float param3, float param4, float param5, int v)
{
    float gain = dbtoa(param1 * 24.0f);
    sample = sample * gain;
    sample = sample + (param2);
    if (sample <= -1.0f)
    {
        sample = -0.666666f;
    } else if (sample >= 1.0f)
    {
        sample = 0.6666666f;
    } else
    {
        sample = sample - ((sample * sample * sample)* 0.3333333f);
    }
    sample = tHighpass_tick(&dcBlock1[v], sample);
    return sample;
}


float Effect::satTick(float sample, float param1, float param2, float param3, float param4, float param5, int v)
{
    float gain = dbtoa(param1 * 24.0f);
    sample = sample * gain;
    float temp = (sample + (param2 * gain)) / (1.0f + fabs(sample + param2));
    temp = tHighpass_tick(&dcBlock1[v], temp);
    temp = tHighpass_tick(&dcBlock2[v], temp);
    temp = tanhf(temp);
    return temp;
}



float Effect::bcTick(float sample, float param1, float param2, float param3, float param4, float param5, int v)
{
    float gain = dbtoa(param1 * 24.0f);
    sample = sample * gain;
    tCrusher_setQuality (&bc[v],param2);
    tCrusher_setSamplingRatio (&bc[v],(param3* inv_oversample) + 0.01f);
    tCrusher_setRound(&bc[v], param4);
    tCrusher_setOperation(&bc[v], param5);
    float temp = tCrusher_tick(&bc[v], sample);
    return temp;
}


float Effect::compressorTick(float sample, float param1, float param2, float param3, float param4, float param5, int v)
{
    tCompressor_setParams(&comp[v], param1*-24.0f,((param2*11.0f)+1.0f), 3.0f, param3 * 15.0f, param4 * 1000.0f +  1.0f, param5 * 1000.0f + 1.0f);
    return tCompressor_tick (&comp[v], sample);
}

//TanhClipper::TanhClipper(const String& n, ElectroAudioProcessor& p, AudioProcessorValueTreeState& vts) : Effect(n, p, vts)
//{
//    int temp = processor.leaf.clearOnAllocation;
//    getProcessor()->leaf.clearOnAllocation = 1;
//    tOversampler_init(&os, oversample, 1, &getProcessor()->leaf);
//
//    getProcessor()->leaf.clearOnAllocation = temp;
//}
//
//TanhClipper::~TanhClipper()
//{
//    tOversampler_free(&os);
//}
//
//void TanhClipper::tick(float* samples)
//{
//
//
//    for (int v = 0; v < processor.numVoicesActive; ++v)
//    {
//        float drive = quickParams[Param1][v]->tickNoSmoothing();
//        float mix = quickParams[Mix][v]->tickNoSmoothing();
//        samples[v] = (1.0f - mix) * samples[v] + mix*tOversampler_tick(&os, samples[v], oversamplerArray, &tanhf);
//    }
//}
