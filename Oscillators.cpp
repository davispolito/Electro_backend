/*
  ==============================================================================

    Oscillators.cpp
    Created: 17 Mar 2021 2:49:31pm
    Author:  Matthew Wang

  ==============================================================================
*/

#include "Oscillators.h"
#include "../PluginProcessor.h"

//==============================================================================

Oscillator::Oscillator(const String& n, ElectroAudioProcessor& p,
                       AudioProcessorValueTreeState& vts) :
AudioComponent(n, p, vts, cOscParams, true),
MappingSourceModel(p, n, true, true, Colours::darkorange),
syncSource(nullptr)
{
    for (int i = 0; i < processor.numInvParameterSkews; ++i)
    {
        sourceValues[i] = (float*) leaf_alloc(&p.leaf, sizeof(float) * MAX_NUM_VOICES);
        sources[i] = &sourceValues[i];
    }
    
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tMBSaw_init(&saw[i], &processor.leaf);
        tMBPulse_init(&pulse[i], &processor.leaf);
        tCycle_init(&sine[i], &processor.leaf);
        tMBTriangle_init(&tri[i], &processor.leaf);
        
        tMBSawPulse_init(&sawPaired[i], &processor.leaf);
        
        tMBSineTri_init(&sinePaired[i], &processor.leaf);
       
    }
    
    filterSend = std::make_unique<SmoothedParameter>(p, vts, n + " FilterSend");
    isHarmonic_raw = vts.getRawParameterValue(n + " isHarmonic");
    isHarmonic_raw = vts.getRawParameterValue(n + " isEnabled");
    isStepped_raw = vts.getRawParameterValue(n + " isStepped");
    isSync_raw = vts.getRawParameterValue(n + " isSync");
    syncType_raw = vts.getRawParameterValue(n + " syncType");
    afpShapeSet = vts.getRawParameterValue(n + " ShapeSet");
}

Oscillator::~Oscillator()
{
    for (int i = 0; i < processor.numInvParameterSkews; ++i)
    {
        leaf_free(&processor.leaf, (char*)sourceValues[i]);
    }
    
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tMBSaw_free(&saw[i]);
        tMBPulse_free(&pulse[i]);
        tCycle_free(&sine[i]);
        tMBTriangle_free(&tri[i]);
        tMBSawPulse_free(&sawPaired[i]);
        tMBSineTri_free(&sinePaired[i]);

        
        if (waveTableFile.exists())
        {
            tWaveOscS_free(&wave[i]);
        }
    }
    DBG("Post exit: " + String(processor.leaf.allocCount) + " " + String(processor.leaf.freeCount));
}

void Oscillator::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    AudioComponent::prepareToPlay(sampleRate, samplesPerBlock);
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tMBSaw_setSampleRate(&saw[i], sampleRate);
        tMBPulse_setSampleRate(&pulse[i], sampleRate);
        tCycle_setSampleRate(&sine[i], sampleRate);
        tMBTriangle_setSampleRate(&tri[i], sampleRate);
        
        tMBSawPulse_setSampleRate(&sawPaired[i], sampleRate);
        
        tMBSineTri_setSampleRate(&sinePaired[i], sampleRate);
       
        
        if (waveTableFile.exists())
        {
            tWaveOscS_setSampleRate(&wave[i], sampleRate);
        }
    }
}

void Oscillator::frame()
{
    sampleInBlock = 0;
    enabled = afpEnabled == nullptr || *afpEnabled > 0 ||
    processor.sourceMappingCounts[getName()] > 0;
    
    currentShapeSet = OscShapeSet(int(*afpShapeSet));
    switch (currentShapeSet) {
        case SawPulseOscShapeSet:
            shapeTick = &Oscillator::sawSquareTick;
            break;
            
        case SineTriOscShapeSet:
            shapeTick = &Oscillator::sineTriTick;
            break;
            
        case SawOscShapeSet:
            shapeTick = &Oscillator::sawTick;
            break;
            
        case PulseOscShapeSet:
            shapeTick = &Oscillator::pulseTick;
            break;
            
        case SineOscShapeSet:
            shapeTick = &Oscillator::sineTick;
            break;
            
        case TriOscShapeSet:
            shapeTick = &Oscillator::triTick;
            break;
            
        case UserOscShapeSet:
            shapeTick = &Oscillator::userTick;
            break;
            
        default:
            shapeTick = &Oscillator::sawSquareTick;
            break;
    }
}


void Oscillator::setShape(int v, float shape)
{
    currentShapeSet = OscShapeSet(int(*afpShapeSet));
    switch (currentShapeSet) {
        case SawPulseOscShapeSet:
            tMBSawPulse_setShape(&sawPaired[v], shape);
            break;
            
        case SineTriOscShapeSet:
            tMBSineTri_setShape(&sinePaired[v], shape);
            break;
            
        case SawOscShapeSet:
            //shapeTick = &Oscillator::sawTick;
            break;
            
        case PulseOscShapeSet:
            tMBPulse_setWidth(&pulse[v], shape);
            break;
            
        case SineOscShapeSet:
            //shapeTick = &Oscillator::sineTick;
            break;
            
        case TriOscShapeSet:
            tMBTriangle_setWidth(&tri[v], shape);
            break;
            
        case UserOscShapeSet:
            tWaveOscS_setIndex(&wave[v], shape);
            break;
            
        default:
            tMBSawPulse_setShape(&sawPaired[v], shape);
            break;
    }
}

void Oscillator::tick(float output[][MAX_NUM_VOICES])
{
    //if (loadingTables || !enabled) return;

    for (int v = 0; v < processor.numVoicesActive; ++v)
    {
        if (!processor.voiceIsSounding[v]) continue;
        
        float pitch = quickParams[OscPitch][v]->read();
        float harm = quickParams[OscHarm][v]->read();
        float fine = quickParams[OscFine][v]->read();
        float freq = quickParams[OscFreq][v]->read();
        float shape = quickParams[OscShape][v]->read();
        if(processor.knobsToSmooth.contains(quickParams[OscShape][v]))
        {
            setShape(v, shape);
        }
        float amp = quickParams[OscAmp][v]->read();
        float harm_pitch = harm + pitch;
        amp = amp < 0.f ? 0.f : amp;
        float note = processor.voiceNote[v];
        if (isStepped_raw == nullptr || *isStepped_raw > 0)
        {
            harm_pitch = round(harm_pitch);
        }
        if (isHarmonic_raw == nullptr || *isHarmonic_raw > 0)
        {
            note = harm_pitch >= 0 ? ftom(processor.tuner.mtof(note) * (harm_pitch + 1)) : ftom(processor.tuner.mtof(note) / abs((harm_pitch - 1)));
            harm_pitch = 0;
        }
        
        //DBG(ftom(processor.tuner.mtof(note) / (harm - 1)));
        //DBG(processor.tuner.mtof(note) / (harm - 1));
        float finalFreq = processor.tuner.mtof(LEAF_clip(0.0f, note + harm_pitch + (fine * 0.01f), 127.0f)) + freq;
        //DBG(note);
        //freq = freq < 10.f ? 0.f : freq
        
        float sample = 0.0f;
        
        shape = LEAF_clip(0.f, shape, 1.f);
        (this->*shapeTick)(sample, v, finalFreq, shape);
    
        sample *= amp;
        
        float normSample = (sample + 1.f) * 0.5f;
        sourceValues[0][v] = normSample;
        for (int i = 1; i < processor.numInvParameterSkews; ++i)
        {
            float invSkew = processor.quickInvParameterSkews[i];
            sourceValues[i][v] = powf(normSample, invSkew);
        }
        
        syncOut[v] = sample;
        
        sample *= processor.oscAmpMult;
        
        float f = filterSend->tickNoHooks();
        
        output[0][v] += sample*f * *afpEnabled;
        output[1][v] += sample*(1.f-f) * *afpEnabled ;
    }
    
    sampleInBlock++;
}

void Oscillator::loadAll(int v)
{
    quickParams[OscShape][v]->setValueToRaw();
    quickParams[OscAmp][v]->setValueToRaw();
    quickParams[OscFine][v]->setValueToRaw();
    quickParams[OscHarm][v]->setValueToRaw();
    quickParams[OscFreq][v]->setValueToRaw();
    quickParams[OscPitch][v]->setValueToRaw();
    setShape(v, quickParams[OscShape][v]->read());
}

void Oscillator::sawSquareTick(float& sample, int v, float freq, float shape)
{
    tMBSawPulse_setFreq(&sawPaired[v], freq);
    
    if (isSync_raw == nullptr || *isSync_raw > 0)
    {
        if(syncType_raw != nullptr)
        {
            tMBSawPulse_setSyncMode(&sawPaired[v], !(*syncType_raw>0.5f));
        }
        tMBSawPulse_sync(&sawPaired[v], syncSource->syncOut[v]);
    }
    sample += tMBSawPulse_tick(&sawPaired[v]) * 2.f;

}

void Oscillator::sineTriTick(float& sample, int v, float freq, float shape)
{
    tMBSineTri_setFreq(&sinePaired[v], freq);
    
    if (isSync_raw == nullptr || *isSync_raw > 0)
    {
        if(syncType_raw != nullptr)
        {
            tMBSineTri_setSyncMode(&sinePaired[v], !(*syncType_raw>0.5f));
        }
        tMBSineTri_sync(&sinePaired[v], syncSource->syncOut[v]);
    }
    sample += tMBSineTri_tick(&sinePaired[v]) * 2.0f;

}

void Oscillator::sawTick(float& sample, int v, float freq, float shape)
{
    tMBSaw_setFreq(&saw[v], freq);
    if (isSync_raw == nullptr || *isSync_raw > 0)
    {
        if(syncType_raw != nullptr)
        {
            tMBSaw_setSyncMode(&saw[v], !(*syncType_raw>0.5f));
        }
        tMBSaw_sync(&saw[v], syncSource->syncOut[v]);
    }
    sample += tMBSaw_tick(&saw[v]) * 2.f;;
}

void Oscillator::pulseTick(float& sample, int v, float freq, float shape)
{
    tMBPulse_setFreq(&pulse[v], freq);
    
    if (isSync_raw == nullptr || *isSync_raw > 0)
    {
        if(syncType_raw != nullptr)
        {
            tMBPulse_setSyncMode(&pulse[v], !(*syncType_raw>0.5f));
        }
        tMBPulse_sync(&pulse[v], syncSource->syncOut[v]);
    }
    sample += tMBPulse_tick(&pulse[v]) * 2.f;;
}

void Oscillator::sineTick(float& sample, int v, float freq, float shape)
{
    tCycle_setFreq(&sine[v], freq);
    sample += tCycle_tick(&sine[v]);
}

void Oscillator::triTick(float& sample, int v, float freq, float shape)
{
    tMBTriangle_setFreq(&tri[v], freq);
    
    if (isSync_raw == nullptr || *isSync_raw > 0)
    {
        if(syncType_raw != nullptr)
        {
            tMBTriangle_setSyncMode(&tri[v], !(*syncType_raw>0.5f));
        }
        tMBTriangle_sync(&tri[v], syncSource->syncOut[v]);
    }
    sample += tMBTriangle_tick(&tri[v]) * 2.f;;
}

void Oscillator::userTick(float& sample, int v, float freq, float shape)
{
    tWaveOscS_setFreq(&wave[v], freq);
    
    sample += tWaveOscS_tick(&wave[v]);
}

void Oscillator::setWaveTables(File file)
{
    loadingTables = true;
    
    File loaded = processor.loadWaveTables(file.getFullPathName(), file);
    
    if (loaded.exists() && loaded != waveTableFile)
    {
        if (waveTableFile.exists())
        {
            for (int i = 0; i < MAX_NUM_VOICES; ++i)
            {
                tWaveOscS_free(&wave[i]);
            }
        }
        waveTableFile = loaded;
        
        for (int i = 0; i < MAX_NUM_VOICES; ++i)
        {
            Array<tWaveTableS>& tables = processor.waveTables.getReference(waveTableFile.getFullPathName());
            tWaveOscS_init(&wave[i], tables.getRawDataPointer(), tables.size(), &processor.leaf);
        }
    }
    
    loadingTables = false;
}

//==============================================================================

LowFreqOscillator::LowFreqOscillator(const String& n, ElectroAudioProcessor& p, AudioProcessorValueTreeState& vts) :
AudioComponent(n, p, vts, cLowFreqParams, false),
MappingSourceModel(p, n, true, true, Colours::chartreuse)
{
    for (int i = 0; i < p.numInvParameterSkews; ++i)
    {
        sourceValues[i] = (float*) leaf_alloc(&p.leaf, sizeof(float) * MAX_NUM_VOICES);
        sources[i] = &sourceValues[i];
    }
    
    sync = vts.getParameter(n + " Sync");
    
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tIntPhasor_init(&saw[i], &processor.leaf);
        tSquareLFO_init(&pulse[i], &processor.leaf);
        tCycle_init(&sine[i], &processor.leaf);
        tTriLFO_init(&tri[i], &processor.leaf);
        
        tSineTriLFO_init(&sineTri[i], &processor.leaf);
        tSawSquareLFO_init(&sawSquare[i], &processor.leaf);
    }
    
    phaseReset = 0.0f;
    
    afpShapeSet = vts.getRawParameterValue(n + " ShapeSet");
}

LowFreqOscillator::~LowFreqOscillator()
{
    for (int i = 0; i < processor.numInvParameterSkews; ++i)
    {
        leaf_free(&processor.leaf, (char*)sourceValues[i]);
    }
    
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tIntPhasor_free(&saw[i]);
        tSquareLFO_free(&pulse[i]);
        tCycle_free(&sine[i]);
        tTriLFO_free(&tri[i]);
        
        tSineTriLFO_free(&sineTri[i]);
        tSawSquareLFO_free(&sawSquare[i]);
    }
}

void LowFreqOscillator::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    AudioComponent::prepareToPlay(sampleRate, samplesPerBlock);
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tIntPhasor_setSampleRate(&saw[i], sampleRate);
        tSquareLFO_setSampleRate(&pulse[i], sampleRate);
        tCycle_setSampleRate(&sine[i], sampleRate);
        tTriLFO_setSampleRate(&tri[i], sampleRate);
        
        tSawSquareLFO_setSampleRate(&sawSquare[i], sampleRate);
        tSineTriLFO_setSampleRate(&sineTri[i], sampleRate);
    }
}

void LowFreqOscillator::frame()
{
    sampleInBlock = 0;
    // only enabled if it's actually being used as a source
    enabled = processor.sourceMappingCounts[getName()] > 0;
    currentShapeSet = LFOShapeSet(int(*afpShapeSet));
    switch (currentShapeSet) {
        case SineTriLFOShapeSet:
            shapeTick = &LowFreqOscillator::sineTriTick;
            break;
            
        case SawPulseLFOShapeSet:
            shapeTick = &LowFreqOscillator::sawSquareTick;
            break;
            
        case SineLFOShapeSet:
            shapeTick = &LowFreqOscillator::sineTick;
            break;
            
        case TriLFOShapeSet:
            shapeTick = &LowFreqOscillator::triTick;
            break;
            
        case SawLFOShapeSet:
            shapeTick = &LowFreqOscillator::sawTick;
            break;
            
        case PulseLFOShapeSet:
            shapeTick = &LowFreqOscillator::pulseTick;
            break;
            
        default:
            shapeTick = &LowFreqOscillator::sineTriTick;
            break;
    }
}

void LowFreqOscillator::setShape(int v, float shape)
{
    currentShapeSet = LFOShapeSet(int(*afpShapeSet));
    switch (currentShapeSet) {
        case SineTriLFOShapeSet:
            tSineTriLFO_setShape(&sineTri[v], shape);
            break;
            
        case SawPulseLFOShapeSet:
            tSawSquareLFO_setShape(&sawSquare[v], shape);
            break;
            
        case SineLFOShapeSet:
            //shapeTick = &LowFreqOscillator::sineTick;
            break;
            
        case TriLFOShapeSet:
            //tTriLFO_setWidth(&tri[v], shape);
            break;
            
        case SawLFOShapeSet:
            //shapeTick = &LowFreqOscillator::sawTick;
            break;
            
        case PulseLFOShapeSet:
            tSquareLFO_setPulseWidth(&pulse[v], shape);
            break;
            
        default:
            tSineTriLFO_setShape(&sineTri[v], shape);
            break;
    }
}

void LowFreqOscillator::setRate(int v, float rate)
{
    currentShapeSet = LFOShapeSet(int(*afpShapeSet));
    switch (currentShapeSet) {
        case SineTriLFOShapeSet:
            tSineTriLFO_setFreq(&sineTri[v], rate);
            
            break;
            
        case SawPulseLFOShapeSet:
            tSawSquareLFO_setFreq(&sawSquare[v], rate);
            break;
            
        case SineLFOShapeSet:
            tCycle_setFreq(&sine[v], rate);
            break;
            
        case TriLFOShapeSet:
            tTriLFO_setFreq(&tri[v], rate);
            break;
            
        case SawLFOShapeSet:
            tIntPhasor_setFreq(&saw[v], rate);
            break;
            
        case PulseLFOShapeSet:
            tSquareLFO_setFreq(&pulse[v], rate);
            break;
            
        default:
            tSineTriLFO_setFreq(&sineTri[v], rate);
            break;
    }
}

float LowFreqOscillator::tick()
{
    if (!enabled) return 0.0f;
//    float a = sampleInBlock * invBlockSize;
    float r = 0.0f;
    for (int v = 0; v < processor.numVoicesActive; v++)
    {
        float rate = quickParams[LowFreqRate][v]->read();
        float shape = quickParams[LowFreqShape][v]->read();
        if (processor.knobsToSmooth.contains(quickParams[LowFreqShape][v]))
        {
            setShape(v,shape);
        }
        
        if (processor.knobsToSmooth.contains(quickParams[LowFreqRate][v]))
        {
            setRate(v,rate);
        }
        phaseReset = quickParams[LowFreqPhase][v]->read();
        // Even though our oscs can handle negative frequency I think allowing the rate to
        // go negative would be confusing behavior
        rate = rate < 0.f ? 0.f : rate;
        shape = LEAF_clip(0.f, shape, 1.f);
        
        float sample = 0;
        (this->*shapeTick)(sample, v);
        float normSample = (sample + 1.f) * 0.5f;
        r = sample;
        sourceValues[0][v] = normSample;
        for (int i = 1; i < processor.numInvParameterSkews; ++i)
        {
            float invSkew = processor.quickInvParameterSkews[i];
            sourceValues[i][v] = powf(normSample, invSkew);
        }
    }
    sampleInBlock++;
    return r;
}

void LowFreqOscillator::loadAll(int v)
{
    quickParams[LowFreqShape][v]->setValueToRaw();
    quickParams[LowFreqRate][v]->setValueToRaw();
    quickParams[LowFreqPhase][v]->setValueToRaw();
    setShape(v, quickParams[LowFreqShape][v]->read());
    setRate(v, quickParams[LowFreqRate][v]->read());
}

void LowFreqOscillator::sawSquareTick(float& sample, int v)
{

    sample += tSawSquareLFO_tick(&sawSquare[v]);
}

void LowFreqOscillator::sineTriTick(float& sample, int v)
{
    sample += tSineTriLFO_tick(&sineTri[v]);
}

void LowFreqOscillator::sineTick(float& sample, int v)
{
    sample += tCycle_tick(&sine[v]);
}

void LowFreqOscillator::triTick(float& sample, int v)
{
    sample += tTriLFO_tick(&tri[v]);
}

void LowFreqOscillator::sawTick(float& sample, int v)
{
    sample += tIntPhasor_tick(&saw[v]);
}

void LowFreqOscillator::pulseTick(float& sample, int v)
{
    sample += tSquareLFO_tick(&pulse[v]);
}

void LowFreqOscillator::noteOn(int voice, float velocity)
{
    if (sync->getValue() > 0)
    {
        tIntPhasor_setPhase(&saw[voice], phaseReset);
        tSquareLFO_setPulseWidth(&pulse[voice], phaseReset);
        tCycle_setPhase(&sine[voice], phaseReset);
        tTriLFO_setPhase(&tri[voice], phaseReset);
        
        tSawSquareLFO_setPhase(&sawSquare[voice], phaseReset);
        tSineTriLFO_setPhase(&sineTri[voice], phaseReset);
    }
}


//==============================================================================

NoiseGenerator::NoiseGenerator(const String& n, ElectroAudioProcessor& p, AudioProcessorValueTreeState& vts) :
AudioComponent(n, p, vts, cNoiseParams, true),
MappingSourceModel(p, n, true, true, Colours::darkorange)
{
    for (int i = 0; i < p.numInvParameterSkews; ++i)
    {
        sourceValues[i] = (float*) leaf_alloc(&p.leaf, sizeof(float) * MAX_NUM_VOICES);
        sources[i] = &sourceValues[i];
    }
    
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tVZFilter_init(&shelf1[i], Lowshelf, 80.0f, 6.0f,   &processor.leaf);
        tVZFilter_init(&shelf2[i], Highshelf, 12000.0f, 6.0f,  &processor.leaf);
        tVZFilter_init(&bell1[i], Bell, 1000.0f, 1.9f,   &processor.leaf);
        tNoise_init(&noise[i], WhiteNoise, &processor.leaf);
    }
    
    filterSend = std::make_unique<SmoothedParameter>(p, vts, n + " FilterSend");

//    afpShapeSet = vts.getRawParameterValue(n + " ShapeSet");
}

NoiseGenerator::~NoiseGenerator()
{
    for (int i = 0; i < processor.numInvParameterSkews; ++i)
    {
        leaf_free(&processor.leaf, (char*)sourceValues[i]);
    }
    
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tNoise_free(&noise[i]);
        tVZFilter_free(&bell1[i]);
        tVZFilter_free(&shelf1[i]);
        tVZFilter_free(&shelf2[i]);
    }
}

void NoiseGenerator::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    AudioComponent::prepareToPlay(sampleRate, samplesPerBlock);
    for (int i = 0; i < MAX_NUM_VOICES; i++)
    {
        tVZFilter_setSampleRate(&bell1[i], sampleRate);
        tVZFilter_setSampleRate(&shelf1[i], sampleRate);
        tVZFilter_setSampleRate(&shelf2[i], sampleRate);
    }
}

void NoiseGenerator::frame()
{
    sampleInBlock = 0;
    enabled = afpEnabled == nullptr || *afpEnabled > 0 ||
    processor.sourceMappingCounts[getName()] > 0;
    
//    currentShapeSet = LFOShapeSet(int(*afpShapeSet));
//    switch (currentShapeSet) {
//        case SineTriLFOShapeSet:
//            shapeTick = &LowFreqOscillator::sineTriTick;
//            break;
//
//        case SawPulseLFOShapeSet:
//            shapeTick = &LowFreqOscillator::sawSquareTick;
//            break;
//
//        case SineLFOShapeSet:
//            shapeTick = &LowFreqOscillator::sineTick;
//            break;
//
//        case TriLFOShapeSet:
//            shapeTick = &LowFreqOscillator::triTick;
//            break;
//
//        case SawLFOShapeSet:
//            shapeTick = &LowFreqOscillator::sawTick;
//            break;
//
//        case PulseLFOShapeSet:
//            shapeTick = &LowFreqOscillator::pulseTick;
    
//            break;
//
//        default:
//            shapeTick = &LowFreqOscillator::sineTriTick;
//            break;
//    }
}
void NoiseGenerator::loadAll(int v)
{
    quickParams[NoiseGain][v]->setValueToRaw();
    quickParams[NoiseAmp][v]->setValueToRaw();
    quickParams[NoiseFreq][v]->setValueToRaw();
    quickParams[NoiseTilt][v]->setValueToRaw();
    tVZFilter_setGain(&shelf1[v], fastdbtoa(-1.0f * ((quickParams[NoiseTilt][v]->read() * 30.0f) - 15.0f)));
    tVZFilter_setGain(&shelf2[v], fastdbtoa((quickParams[NoiseTilt][v]->read() * 30.0f) - 15.0f));
    tVZFilter_setFreqFast(&bell1[v], faster_mtof(quickParams[NoiseTilt][v]->read()  * 77.0f + 42.0f));
    tVZFilter_setGain(&bell1[v],fastdbtoa((quickParams[NoiseGain][v]->read() * 34.0f) - 17.0f));

}

void NoiseGenerator::tick(float output[][MAX_NUM_VOICES])
{
    if (!enabled) return;
    //    float a = sampleInBlock * invBlockSize;
    
    for (int v = 0; v < processor.numVoicesActive; v++)
    {
        float tilt = quickParams[NoiseTilt][v]->read();
        float gain = quickParams[NoiseGain][v]->read();
        float freq = quickParams[NoiseFreq][v]->read();
        float amp = quickParams[NoiseAmp][v]->read();
        amp = amp < 0.f ? 0.f : amp;
        if(processor.knobsToSmooth.contains(quickParams[NoiseTilt][v]))
        {
            tVZFilter_setGain(&shelf1[v], fastdbtoa(-1.0f * ((tilt * 30.0f) - 15.0f)));
            tVZFilter_setGain(&shelf2[v], fastdbtoa((tilt * 30.0f) - 15.0f));
        }
        if(processor.knobsToSmooth.contains(quickParams[NoiseFreq][v]))
        {
            tVZFilter_setFreqFast(&bell1[v], faster_mtof(freq * 77.0f + 42.0f));
        }
        
        if(processor.knobsToSmooth.contains(quickParams[NoiseGain][v]))
        {
            tVZFilter_setGain(&bell1[v],fastdbtoa((gain* 34.0f) - 17.0f));
        }
//        tVZFilter_setFrequencyAndResonanceAndGain(&bell1[v], faster_mtof(freq * 77.0f + 42.0f), 1.9f, fastdbtoa((gain* 34.0f) - 17.0f));
        
        
        //float sample = tSVF_tick(&bandpass[v], tNoise_tick(&noise[v])) * amp;
        float sample = tNoise_tick(&noise[v]) ;
        sample = tVZFilter_tickEfficient(&shelf1[v], sample);
        sample = tVZFilter_tickEfficient(&shelf2[v], sample);
        sample = tVZFilter_tickEfficient(&bell1[v], sample);
        sample = sample * amp; 
        float normSample = (sample + 1.f) * 0.5f;
        //float normSample = sample;
        sourceValues[0][v] = normSample;
        for (int i = 1; i < processor.numInvParameterSkews; ++i)
        {
            float invSkew = processor.quickInvParameterSkews[i];
            sourceValues[i][v] = powf(normSample, invSkew);
        }
        
        float f = filterSend->tickNoHooks();
        
        output[0][v] += sample*f * *afpEnabled;
        output[1][v] += sample*(1.f-f) * *afpEnabled ;
    }
    sampleInBlock++;
}
