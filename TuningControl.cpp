//
//  TuningControl.cpp
//  Electrobass
//
//  Created by Davis Polito on 1/20/22.
//  Copyright © 2022 Snyderphonics. All rights reserved.
//

#include "TuningControl.hpp"
#include "tuning-library/include/Tunings.h"
TuningControl::MidiToFreq TuningControl::setMtoFFunction(bool isMTSOn)
{
    if (isMTSOn)
    {
        return &TuningControl::_MTS_mtof;
    }
    else
    {
        return &TuningControl::_mtof;
    }
}

float TuningControl::mtof(float mn)
{
    return (this->*mtofptr)(mn);
}

float TuningControl::_mtof(float f)
{
    if (f <= -1500.0f) return(0);
    else if (f > 1499.0f) return(_mtof(1499.0f));
    else return (8.17579891564f * expf(0.0577622650f * f));
}

float TuningControl::_MTS_mtof(float mn)
{
    return MTS_NoteToFrequency(client, (char)mn, -1);
}
void TuningControl::MTSOnOff()
{
    if (isMTS && (!MTS_HasMaster(client) || client == nullptr))
    {
        client = MTS_RegisterClient();
    } else if (!isMTS && MTS_HasMaster(client))
    {
        //Deregister just deletes the client so we must set it to null so that we know it is deleted;
        MTS_DeregisterClient(client);
        client = nullptr;
    }
}
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ SCALA READING ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
void TuningControl::loadScala(std::string fname, float* arr)
{
    Tunings::Scale s;
    try {
        s = Tunings::readSCLFile(fname);
    } catch (Tunings::TuningError t) {
        AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, TRANS("Scala Loading Error"),TRANS(t.what()));
        return;
    }

    auto offsets = Array<float>(12);

    if (s.count != 12)
    {
        AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, TRANS("Scala Loading Error"), TRANS("Only 12 note scales supported"));
    }
    if(s.count == 12)
    {
        //subtract from equal temperament to get fractional midi representation
        Tunings::Scale et = Tunings::evenTemperament12NoteScale();
        for (int i = 0; i < 12; i++)
        {
            float micro = s.tones[i].cents;
            float equal = et.tones[i].cents;
            float offset = micro - equal;
            arr[(i+1)%12] = offset / 100.f; //.scl format puts first interval as the first line so we shift the representation over
            DBG("Cents Deviation " + String(arr[(i+1)%12]));
        }
    }
}
