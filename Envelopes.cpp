/*
  ==============================================================================

    Envelopes.cpp
    Created: 17 Mar 2021 4:17:24pm
    Author:  Matthew Wang

  ==============================================================================
*/

#include "Envelopes.h"
#include "../PluginProcessor.h"

//==============================================================================
Envelope::Envelope(const String& n, ElectroAudioProcessor& p,
                   AudioProcessorValueTreeState& vts) :
AudioComponent(n, p, vts, cEnvelopeParams, false),
MappingSourceModel(p, n, true, false, Colours::deepskyblue)
{
    for (int i = 0; i < processor.numInvParameterSkews; ++i)
    {
        sourceValues[i] = (float*) leaf_alloc(&processor.leaf, sizeof(float) * MAX_NUM_VOICES);
        sources[i] = &sourceValues[i];
    }
    
    useVelocity = vts.getParameter(n + " Velocity");
    
    //exponential buffer rising from 0 to 1
    LEAF_generate_exp(expBuffer, 1000.0f, -1.0f, 0.0f, -0.0008f, EXP_BUFFER_SIZE);
    
    // exponential decay buffer falling from 1 to
    LEAF_generate_exp(decayExpBuffer, 0.001f, 0.0f, 1.0f, -0.0008f, DECAY_EXP_BUFFER_SIZE);
    
    expBufferSizeMinusOne = EXP_BUFFER_SIZE - 1;
    decayExpBufferSizeMinusOne = DECAY_EXP_BUFFER_SIZE - 1;
    
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tADSRT_init(&envs[i], expBuffer[0] * 8192.0f,
                    expBuffer[(int)(0.06f * expBufferSizeMinusOne)] * 8192.0f,
                    expBuffer[(int)(0.9f * expBufferSizeMinusOne)] * 8192.0f,
                    expBuffer[(int)(0.1f * expBufferSizeMinusOne)] * 8192.0f,
                    decayExpBuffer, DECAY_EXP_BUFFER_SIZE, &processor.leaf);
        tADSRT_setLeakFactor(&envs[i], ((1.0f - 0.1f) * 0.00005f) + 0.99995f);
    }
}

Envelope::~Envelope()
{
    for (int i = 0; i < processor.numInvParameterSkews; ++i)
    {
        leaf_free(&processor.leaf, (char*)sourceValues[i]);
    }
    
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tADSRT_free(&envs[i]);
    }
}

void Envelope::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    AudioComponent::prepareToPlay(sampleRate, samplesPerBlock);
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tADSRT_setSampleRate(&envs[i], sampleRate);
    }
}

void Envelope::frame()
{
    sampleInBlock = 0;
    // only enabled if it's actually being used as a source
    enabled = processor.sourceMappingCounts[getName()] > 0;
}


void Envelope::loadAll(int v)
{
    float attack = quickParams[EnvelopeAttack][v]->read();
    attack = attack < 0.f ? 0.f : attack;
    tADSRT_setAttack(&envs[v], attack);
    float decay = quickParams[EnvelopeDecay][v]->read();
    decay = decay < 0.f ? 0.f : decay;
    tADSRT_setDecay(&envs[v], decay);
    float sustain = quickParams[EnvelopeSustain][v]->read();
    sustain = sustain < 0.f ? 0.f : sustain;
    tADSRT_setSustain(&envs[v], sustain);
    float release = quickParams[EnvelopeRelease][v]->read();
    release = release < 0.f ? 0.f : release;
    tADSRT_setRelease(&envs[v], release);
    float leak = quickParams[EnvelopeLeak][v]->read();
    leak = leak < 0.f ? 0.f : leak;
    tADSRT_setLeakFactor(&envs[v], 0.99995f + 0.00005f*(1.f-leak));
}

void Envelope::tick()
{
    if (!enabled) return;
//    float a = sampleInBlock * invBlockSize;
    
    for (int v = 0; v < processor.numVoicesActive; v++)
    {
        
        if(!quickParams[EnvelopeAttack][v]->getRemoveMe())
        {
            float attack = quickParams[EnvelopeAttack][v]->read();
            attack = attack < 0.f ? 0.f : attack;
            tADSRT_setAttack(&envs[v], attack);
        }
        if(!quickParams[EnvelopeDecay][v]->getRemoveMe())
        {
            float decay = quickParams[EnvelopeDecay][v]->read();
            decay = decay < 0.f ? 0.f : decay;
            tADSRT_setDecay(&envs[v], decay);
        }
        if(!quickParams[EnvelopeSustain][v]->getRemoveMe())
        {
            float sustain = quickParams[EnvelopeSustain][v]->read();
            sustain = sustain < 0.f ? 0.f : sustain;
            tADSRT_setSustain(&envs[v], sustain);
        }
        if(!quickParams[EnvelopeRelease][v]->getRemoveMe())
        {
            float release = quickParams[EnvelopeRelease][v]->read();
            release = release < 0.f ? 0.f : release;
            tADSRT_setRelease(&envs[v], release);
        }
        if(!quickParams[EnvelopeLeak][v]->getRemoveMe())
        {
            float leak = quickParams[EnvelopeLeak][v]->read();
            leak = leak < 0.f ? 0.f : leak;
            tADSRT_setLeakFactor(&envs[v], 0.99995f + 0.00005f*(1.f-leak));
        }
        
       
        
        
        

        float value = tADSRT_tick(&envs[v]);
        
        sourceValues[0][v] = value;
        for (int i = 1; i < processor.numInvParameterSkews; ++i)
        {
            float invSkew = processor.quickInvParameterSkews[i];
            sourceValues[i][v] = powf(value, invSkew);
        }
        if (isAmpEnv)
        {
//            if (processor.strings[0]->numVoices > 1)
//            {
                if (processor.strings[0]->voices[v][0] == -2)
                {
                    if (envs[v]->whichStage == env_idle)
                    {
                        tSimplePoly_deactivateVoice(&processor.strings[0], v);
                        processor.voiceIsSounding[v] = false;
                        //DBG("Voice Ended");
                    }
                }
//            }
        }
    }
//    sampleInBlock++;
}

void Envelope::noteOn(int voice, float velocity)
{
    if (useVelocity->getValue() == 0) velocity = 1.f;
    tADSRT_on(&envs[voice], velocity);
    processor.voiceIsSounding[voice] = true;
}

void Envelope::noteOff(int voice, float velocity)
{
    
    tADSRT_off(&envs[voice]);
}
