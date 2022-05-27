/*
  ==============================================================================

    FXTab.h
    Created: 25 May 2022 2:45:25pm
    Author:  Davis Polito

  ==============================================================================
*/

#pragma once
#include "ElectroModules.h"
#include "../Constants.h"
//==============================================================================
class FXTab : public Component
{
public:
    FXTab(ElectroAudioProcessorEditor& e, ElectroAudioProcessor& p, AudioProcessorValueTreeState& vts) :
    editor(e),
    processor(p)
    {
        for (int i = 0; i < NUM_FX; ++i)
        {
            fxModules.add(new FXModule(e, vts, *processor.fx[i]));
            addAndMakeVisible(fxModules[i]);
        }
    }
    
    ~FXTab() override
    {

    }
   
    void resized() override
    {
        Rectangle<int> area = getLocalBounds();
        
        int h = area.getHeight();
        int w = area.getWidth();
        int r = area.getWidth() - w - 2;

        Rectangle<int> bottomArea = area.removeFromBottom(h*0.15);
        bottomArea.removeFromTop(h*0.03);
        Rectangle<int> upperBottomArea = bottomArea.removeFromTop(h*0.06);
        upperBottomArea.removeFromRight(2);
        bottomArea.removeFromRight(2);
        
        float s = w / EDITOR_WIDTH;

        
        
        for (int i = 0; i < NUM_FX; i++)
        {
            fxModules[i]->setBounds(-1, (120*s*i)-i-1, 540*s+1, 120*s);
        }
    }
    
private:
    
    ElectroLookAndFeel laf;
    ElectroAudioProcessorEditor& editor;
    ElectroAudioProcessor& processor;
    OwnedArray<FXModule> fxModules;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FXTab)
};
