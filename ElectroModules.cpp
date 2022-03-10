/*
  ==============================================================================

    ElectroModules.cpp
    Created: 2 Jul 2021 3:06:27pm
    Author:  Matthew Wang

  ==============================================================================
*/

#include "ElectroModules.h"
#include "../PluginEditor.h"

//==============================================================================
//==============================================================================
ElectroComponent::ElectroComponent() :
outlineColour(Colours::transparentBlack)
{
    setInterceptsMouseClicks(false, true);
}

ElectroComponent::~ElectroComponent()
{
    
}

void ElectroComponent::paint(Graphics &g)
{
    Rectangle<int> area = getLocalBounds();
    
    g.setColour(outlineColour);
    g.drawRect(area);
}

//==============================================================================

ElectroModule::ElectroModule(ElectroAudioProcessorEditor& editor, AudioProcessorValueTreeState& vts,
                   AudioComponent& ac, float relLeftMargin, float relDialWidth,
                   float relDialSpacing, float relTopMargin, float relDialHeight) :
editor(editor),
vts(vts),
ac(ac),
relLeftMargin(relLeftMargin),
relDialWidth(relDialWidth),
relDialSpacing(relDialSpacing),
relTopMargin(relTopMargin),
relDialHeight(relDialHeight)
{
    String& name = ac.getName();
    StringArray& paramNames = ac.getParamNames();
    for (int i = 0; i < paramNames.size(); i++)
    {
        String paramName = name + " " + paramNames[i];
        String displayName = paramNames[i];
        dials.add(new ElectroDial(editor, paramName, displayName, false, true));
        addAndMakeVisible(dials[i]);
        sliderAttachments.add(new SliderAttachment(vts, paramName, dials[i]->getSlider()));
        dials[i]->getSlider().addListener(this);
        for (auto t : dials[i]->getTargets())
        {
            t->addListener(this);
            t->addMouseListener(this, true);
            t->update(true, false);
        }
    }
    
    if (ac.isToggleable())
    {
        enabledToggle.addListener(this);
        enabledToggle.setToggleState(ac.isEnabled(), sendNotification);
        addAndMakeVisible(enabledToggle);
        buttonAttachments.add(new ButtonAttachment(vts, name, enabledToggle));
    }
}

ElectroModule::~ElectroModule()
{
    sliderAttachments.clear();
    buttonAttachments.clear();
    comboBoxAttachments.clear();
}

void ElectroModule::resized()
{
    for (int i = 0; i < ac.getParamNames().size(); ++i)
    {
        dials[i]->setBoundsRelative(relLeftMargin + (relDialWidth+relDialSpacing)*i, relTopMargin,
                                    relDialWidth, relDialHeight);
    }
    
    if (ac.isToggleable())
    {
        enabledToggle.setBounds(0, 0, 25, 25);
    }
}

void ElectroModule::sliderValueChanged(Slider* slider)
{
    if (MappingTarget* mt = dynamic_cast<MappingTarget*>(slider))
    {
        dynamic_cast<ElectroDial*>(mt->getParentComponent())->sliderValueChanged(slider);
    }
}

void ElectroModule::setBounds (float x, float y, float w, float h)
{
    Rectangle<float> newBounds (x, y, w, h);
    setBounds(newBounds);
}

void ElectroModule::setBounds (Rectangle<float> newBounds)
{
    Component::setBounds(newBounds.toNearestInt());
}

ElectroDial* ElectroModule::getDial (int index)
{
    return dials[index];
}

//==============================================================================
//==============================================================================

OscModule::OscModule(ElectroAudioProcessorEditor& editor, AudioProcessorValueTreeState& vts,
                     AudioComponent& ac) :
ElectroModule(editor, vts, ac, 0.05f, 0.132f, 0.05f, 0.18f, 0.8f),
chooser(nullptr)
{
    
    
    buttonAttachments.add(new ButtonAttachment(vts, ac.getName() + " isHarmonic", pitchDialToggle));
    buttonAttachments.add(new ButtonAttachment(vts, ac.getName() + " isStepped", steppedToggle));
    outlineColour = Colours::darkgrey;
    
    // Pitch and freq dials should snap to ints
    getDial(OscPitch)->setRange(-16., 16., 1.);
    getDial(OscFreq)->setRange(-2000., 2000., 1.);
    pitchDialToggle.setLookAndFeel(&laf);
    pitchDialToggle.addListener(this);
    pitchDialToggle.setTitle("Harmonic Dial");
    pitchDialToggle.setButtonText("Harmonic");
    pitchDialToggle.setToggleable(true);
    pitchDialToggle.setClickingTogglesState(true);
    pitchDialToggle.setToggleState(true, dontSendNotification);
    addAndMakeVisible(pitchDialToggle);
    steppedToggle.setLookAndFeel(&laf);
    steppedToggle.addListener(this);
    steppedToggle.setTitle("Stepped Dial");
    steppedToggle.setButtonText("Stepped");
    steppedToggle.changeWidthToFitText();
    steppedToggle.setToggleable(true);
    steppedToggle.setClickingTogglesState(true);
    steppedToggle.setToggleState(true, dontSendNotification);
    addAndMakeVisible(steppedToggle);
    displayPitch();
    getDial(OscPitch)->setRange(-16, 16., steppedToggle.getToggleState() ? 1 : 0.01 );
    getDial(OscPitch)->setText("Harmonics", dontSendNotification);
//    smoothingToggle.setLookAndFeel(&laf);
//    smoothingToggle.addListener(this);
//    smoothingToggle.setButtonText("Smoothed");
//    addAndMakeVisible(smoothingToggle);
    
    harmonicsLabel.setLookAndFeel(&laf);
    harmonicsLabel.setEditable(true);
    harmonicsLabel.setJustificationType(Justification::centred);
    harmonicsLabel.setColour(Label::backgroundColourId, Colours::darkgrey.withBrightness(0.2f));
    harmonicsLabel.addListener(this);
    addAndMakeVisible(harmonicsLabel);
    
    pitchLabel.setLookAndFeel(&laf);
    pitchLabel.setEditable(true);
    pitchLabel.setJustificationType(Justification::centred);
    pitchLabel.setColour(Label::backgroundColourId, Colours::darkgrey.withBrightness(0.2f));
    pitchLabel.addListener(this);
    addAndMakeVisible(pitchLabel);
    
    freqLabel.setLookAndFeel(&laf);
    freqLabel.setEditable(true);
    freqLabel.setJustificationType(Justification::centred);
    freqLabel.setColour(Label::backgroundColourId, Colours::darkgrey.withBrightness(0.2f));
    freqLabel.addListener(this);
    addAndMakeVisible(freqLabel);
    
    displayPitch();
    
    RangedAudioParameter* set = vts.getParameter(ac.getName() + " ShapeSet");
    updateShapeCB();
    shapeCB.setSelectedItemIndex(set->convertFrom0to1(set->getValue()), dontSendNotification);
    shapeCB.setLookAndFeel(&laf);
    shapeCB.addListener(this);
    shapeCB.addMouseListener(this, true);
    addAndMakeVisible(shapeCB);
//    comboBoxAttachments.add(new ComboBoxAttachment(vts, ac.getName() + " ShapeSet", shapeCB));
    
    sendSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
    sendSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 10, 10);
    
    addAndMakeVisible(sendSlider);
    sliderAttachments.add(new SliderAttachment(vts, ac.getName() + " FilterSend", sendSlider));
    
    f1Label.setText("F1", dontSendNotification);
    f1Label.setJustificationType(Justification::bottomRight);
    f1Label.setLookAndFeel(&laf);
    addAndMakeVisible(f1Label);
    
    f2Label.setText("F2", dontSendNotification);
    f2Label.setJustificationType(Justification::topRight);
    f2Label.setLookAndFeel(&laf);
    addAndMakeVisible(f2Label);
    
    s = std::make_unique<MappingSource>
    (editor, *editor.processor.getMappingSource(ac.getName()), ac.getName());
    addAndMakeVisible(s.get());
    
    
}

OscModule::~OscModule()
{
    sliderAttachments.clear();
    buttonAttachments.clear();
    comboBoxAttachments.clear();
}

void OscModule::resized()
{
    ElectroModule::resized();
    
    s->setBounds(4, 4, getWidth()*0.1f, enabledToggle.getHeight()-8);
   
    harmonicsLabel.setBoundsRelative(relLeftMargin + relDialWidth * 0.25,
                                     0.02f, relDialWidth+relDialSpacing * 0.25, 0.16f);
    
    pitchDialToggle.setBoundsRelative(0.0f, 0.412f, 0.05f, 0.15f);
    steppedToggle.setBoundsRelative(0.0f, 0.2f, 0.05f, 0.15f);

    
    
    pitchLabel.setBoundsRelative(relLeftMargin+relDialWidth * 1.25,
                                 0.02f, relDialWidth+relDialSpacing, 0.16f);
    
    freqLabel.setBoundsRelative(relLeftMargin+2*relDialWidth+1.5*relDialSpacing,
                                 0.02f, relDialWidth+relDialSpacing, 0.16f);
    
    shapeCB.setBoundsRelative(relLeftMargin+3*(relDialWidth+relDialSpacing), 0.02f,
                              relDialWidth+relDialSpacing, 0.16f);
    
    sendSlider.setBoundsRelative(0.96f, 0.f, 0.04f, 1.0f);
    
    enabledToggle.setBoundsRelative(0.917f, 0.41f, 0.04f, 0.15f);
    smoothingToggle.setBoundsRelative(0.0f, 0.41f, 0.04f, 0.15f);
    f1Label.setBoundsRelative(0.9f, 0.05f, 0.06f, 0.15f);
    f2Label.setBoundsRelative(0.9f, 0.80f, 0.06f, 0.15f);
}

void OscModule::sliderValueChanged(Slider* slider)
{
    if (slider == &getDial(OscPitch)->getSlider() ||
        slider == &getDial(OscFine)->getSlider() ||
        slider == &getDial(OscFreq)->getSlider())
    {
        displayPitch();
    }
    else if (MappingTarget* mt = dynamic_cast<MappingTarget*>(slider))
    {
        dynamic_cast<ElectroDial*>(mt->getParentComponent())->sliderValueChanged(slider);
        displayPitchMapping(mt);
    }
}

void OscModule::buttonClicked(Button* button)
{
    if (button == &enabledToggle)
    {
        f1Label.setAlpha(enabledToggle.getToggleState() ? 1. : 0.5);
        f2Label.setAlpha(enabledToggle.getToggleState() ? 1. : 0.5);
        sendSlider.setEnabled(enabledToggle.getToggleState());
        sendSlider.setAlpha(enabledToggle.getToggleState() ? 1. : 0.5);
    }
    else if (button == &pitchDialToggle || button == &steppedToggle)
    {
        if (!pitchDialToggle.getToggleState())
        {
            getDial(OscPitch)->setRange(-24, 24., steppedToggle.getToggleState() ? 1 : 0.01 );
            getDial(OscPitch)->setText("Pitch", dontSendNotification);
            pitchDialToggle.setButtonText("Pitch");
        } else
        {
            getDial(OscPitch)->setRange(-16, 16., steppedToggle.getToggleState() ? 1 : 0.01 );
            getDial(OscPitch)->setText("Harmonics", dontSendNotification);
            pitchDialToggle.setButtonText("Harmonic");
        }
        
        if (!steppedToggle.getToggleState())
        {
            steppedToggle.setButtonText("Smooth");
        }
        else
        {
            steppedToggle.setButtonText("Stepped");
        }
        displayPitch();
    }
}

void OscModule::labelTextChanged(Label* label)
{
    if (label == &pitchLabel)
    {
        auto value = pitchLabel.getText().getDoubleValue();
        int i = value;
        double f = value-i;
        //getDial(OscPitch)->getSlider().setValue(i, sendNotificationAsync);
        getDial(OscFine)->getSlider().setValue(f*100., sendNotificationAsync);
    }
    else if (label == &freqLabel)
    {
        auto value = freqLabel.getText().getDoubleValue();
        getDial(OscFreq)->getSlider().setValue(value, sendNotificationAsync);
    }
    else if (label == &harmonicsLabel)
    {
        auto value = harmonicsLabel.getText().getDoubleValue();
        getDial(OscPitch)->getSlider().setValue(value, sendNotificationAsync);
    }
}

void OscModule::comboBoxChanged(ComboBox *comboBox)
{
    if (comboBox == &shapeCB)
    {
        // Select file... option
        if (shapeCB.getSelectedItemIndex() == shapeCB.getNumItems()-1)
        {
            if (chooser != nullptr)
                delete chooser;
            chooser = new FileChooser ("Pick a wav table", editor.processor.getLastFile());
            chooser->launchAsync (FileBrowserComponent::openMode |
                                 FileBrowserComponent::canSelectFiles,
                                 [this] (const FileChooser& chooser)
                                 {
                editor.processor.setLastFile(chooser);
                String path = chooser.getResult().getFullPathName();
                Oscillator& osc = static_cast<Oscillator&>(ac);
                editor.processor.setLastFile(chooser);
                if (path.isEmpty())
                {
                    shapeCB.setSelectedItemIndex(0, dontSendNotification);
                    vts.getParameter(ac.getName() + " ShapeSet")->setValueNotifyingHost(0.);
                    return;
                }
                
                File file(path);
                osc.setWaveTables(file);
                vts.getParameter(ac.getName() + " ShapeSet")->setValueNotifyingHost(1.);
                updateShapeCB();
            
            });
        }
        // Selected a loaded file
        else if (shapeCB.getSelectedItemIndex() >= UserOscShapeSet)
        {
            Oscillator& osc = static_cast<Oscillator&>(ac);
            File file = editor.processor.waveTableFiles[shapeCB.getSelectedItemIndex()-UserOscShapeSet];
            osc.setWaveTables(file);
            vts.getParameter(ac.getName() + " ShapeSet")->setValueNotifyingHost(1.);
            updateShapeCB();
        }
        // Selected built ins
        else
        {
            float normValue = shapeCB.getSelectedItemIndex() / float(UserOscShapeSet);
            vts.getParameter(ac.getName() + " ShapeSet")->setValueNotifyingHost(normValue);
            updateShapeCB();
        }
        
        if (shapeCB.getSelectedItemIndex() >= UserOscShapeSet)
        {
            // Maybe should check that the loaded table has more
            // than one waveform and set alpha accordingly
            getDial(OscShape)->setAlpha(1.f);
            getDial(OscShape)->setInterceptsMouseClicks(true, true);
        }
        else if (shapeCB.getSelectedItemIndex() > SineTriOscShapeSet &&
                 shapeCB.getSelectedItemIndex() != PulseOscShapeSet)
        {
            getDial(OscShape)->setAlpha(0.5f);
            getDial(OscShape)->setInterceptsMouseClicks(false, false);
        }
        else
        {
            getDial(OscShape)->setAlpha(1.f);
            getDial(OscShape)->setInterceptsMouseClicks(true, true);
        }
    }
}

void OscModule::mouseDown(const MouseEvent& e)
{
    updateShapeCB();
}

void OscModule::mouseEnter(const MouseEvent& e)
{
    if (MappingTarget* mt = dynamic_cast<MappingTarget*>(e.originalComponent->getParentComponent()))
    {
        displayPitchMapping(mt);
    }
}

void OscModule::mouseExit(const MouseEvent& e)
{
    displayPitch();
}

void OscModule::updateShapeCB()
{
    shapeCB.clear(dontSendNotification);
    for (int i = 0; i < oscShapeSetNames.size()-1; ++i)
    {
        shapeCB.addItem(oscShapeSetNames[i], shapeCB.getNumItems()+1);
    }
    for (auto file : editor.processor.waveTableFiles)
    {
        shapeCB.addItem(file.getFileNameWithoutExtension(), shapeCB.getNumItems()+1);
    }
    shapeCB.addItem(oscShapeSetNames[oscShapeSetNames.size()-1], shapeCB.getNumItems()+1);
    
    RangedAudioParameter* param = vts.getParameter(ac.getName() + " ShapeSet");
    int index = param->getNormalisableRange().convertFrom0to1(param->getValue());
    if (index == UserOscShapeSet)
    {
        Oscillator& osc = static_cast<Oscillator&>(ac);
        index = editor.processor.waveTableFiles.indexOf(osc.getWaveTableFile())+UserOscShapeSet;
    }
    shapeCB.setSelectedItemIndex(index, dontSendNotification);
}
//MAKE CHANGE HERE
void OscModule::displayPitch()
{
    
   
    auto harm = getDial(OscPitch)->getSlider().getValue();
    harmonicsLabel.setColour(Label::textColourId, Colours::gold.withBrightness(0.95f));
    String t = String(abs(harm),3);
    t = harm >=0 ? t : String("1/" + t);
    if (pitchDialToggle.getToggleState())
        harmonicsLabel.setText(t, dontSendNotification);
    else
        harmonicsLabel.setText("0.000", dontSendNotification);
    auto pitch = pitchDialToggle.getToggleState() ? 0 : getDial(OscPitch)->getSlider().getValue();
    auto fine = getDial(OscFine)->getSlider().getValue()*0.01;
    pitchLabel.setColour(Label::textColourId, Colours::gold.withBrightness(0.95f));
    String text = pitch+fine >= 0 ? "+" : "";
    text += String(pitch+fine, 3);
    pitchLabel.setText(text, dontSendNotification);


    auto freq = getDial(OscFreq)->getSlider().getValue();
    freqLabel.setColour(Label::textColourId, Colours::gold.withBrightness(0.95f));
    text = freq >= 0 ? "+" : "";
    text += String(int(freq)) + " Hz";
    freqLabel.setText(text, dontSendNotification);
}

void OscModule::displayPitchMapping(MappingTarget* mt)
{
    if (!mt->isActive())
    {
        displayPitch();
        return;
    }
    auto start = mt->getModel().start;
    auto end = mt->getModel().end;
    if (mt->getParentComponent() == getDial(OscPitch))
    {
        pitchLabel.setColour(Label::textColourId, mt->getColour());
        String text;
        if (mt->isBipolar())
        {
            if (mt->getSkewFactor() != 1. && start != end)
            {
                text = (start >= 0 ? "+" : "");
                text += String(start, 3) + "/";
                text += (end >= 0 ? "+" : "");
                text += String(end, 3);
            }
            else
            {
                text = String::charToString(0xb1);
                text += String(fabs(mt->getModel().end), 3);
            }
        }
        else
        {
            text = (end >= 0 ? "+" : "");
            text += String(end, 3);
        }
        pitchLabel.setText(text, dontSendNotification);
    }
    else if (mt->getParentComponent() == getDial(OscFine))
    {
        pitchLabel.setColour(Label::textColourId, mt->getColour());
        String text;
        start *= 0.01f;
        end *= 0.01f;
        if (mt->isBipolar())
        {
            if (mt->getSkewFactor() != 1. && start != end)
            {
                text = (start >= 0 ? "+" : "");
                text += String(start, 3) + "/";
                text += (end >= 0 ? "+" : "");
                text += String(end, 3);
            }
            else
            {
                text = String::charToString(0xb1);
                text += String(fabs(mt->getModel().end), 3);
            }
        }
        else
        {
            text = (end >= 0 ? "+" : "");
            text += String(end, 3);
        }
        pitchLabel.setText(text, dontSendNotification);
    }
    else if (mt->getParentComponent() == getDial(OscFreq))
    {
        freqLabel.setColour(Label::textColourId, mt->getColour());
        String text;
        if (mt->isBipolar())
        {
            if (mt->getSkewFactor() != 1. && start != end)
            {
                text = (start >= 0 ? "+" : "");
                text += String(int(start)) + "/";
                text += (end >= 0 ? "+" : "");
                text += String(int(end));
            }
            else
            {
                text = String::charToString(0xb1);
                text += String(abs(mt->getModel().end));
            }
        }
        else
        {
            text = (end >= 0 ? "+" : "");
            text += String(int(end));
        }
        freqLabel.setText(text, dontSendNotification);
    }
}

//==============================================================================
//==============================================================================

NoiseModule::NoiseModule(ElectroAudioProcessorEditor& editor, AudioProcessorValueTreeState& vts,
                     AudioComponent& ac) :
ElectroModule(editor, vts, ac, 0.15f, 0.3f, 0.05f, 0.0f, 0.98f)
{
    outlineColour = Colours::darkgrey;
    
    sendSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
    sendSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 10, 10);
    addAndMakeVisible(sendSlider);
    sliderAttachments.add(new SliderAttachment(vts, ac.getName() + " FilterSend", sendSlider));
    
    f1Label.setText("F1", dontSendNotification);
    f1Label.setJustificationType(Justification::bottomRight);
    f1Label.setLookAndFeel(&laf);
    addAndMakeVisible(f1Label);
    
    f2Label.setText("F2", dontSendNotification);
    f2Label.setJustificationType(Justification::topRight);
    f2Label.setLookAndFeel(&laf);
    addAndMakeVisible(f2Label);
    
    s = std::make_unique<MappingSource>
    (editor, *editor.processor.getMappingSource(ac.getName()), ac.getName());
    addAndMakeVisible(s.get());
}

NoiseModule::~NoiseModule()
{
    sliderAttachments.clear();
    buttonAttachments.clear();
    comboBoxAttachments.clear();
}

void NoiseModule::resized()
{
    ElectroModule::resized();
    
    s->setBounds(4, 4, getWidth()*0.2f, enabledToggle.getHeight()-8);
    s->toFront(false);
    
    sendSlider.setBoundsRelative(0.92f, 0.f, 0.08f, 1.0f);
    
    enabledToggle.setBoundsRelative(0.84f, 0.4f, 0.1f, 0.18f);

    
    
    f1Label.setBoundsRelative(0.81f, 0.05f, 0.12f, 0.17f);
    f2Label.setBoundsRelative(0.81f, 0.78f, 0.12f, 0.17f);
    
}

void NoiseModule::buttonClicked(Button* button)
{
    if (button == &enabledToggle)
    {
        f1Label.setAlpha(enabledToggle.getToggleState() ? 1. : 0.5);
        f2Label.setAlpha(enabledToggle.getToggleState() ? 1. : 0.5);
        sendSlider.setEnabled(enabledToggle.getToggleState());
        sendSlider.setAlpha(enabledToggle.getToggleState() ? 1. : 0.5);
    }
}

//==============================================================================
//==============================================================================

FilterModule::FilterModule(ElectroAudioProcessorEditor& editor, AudioProcessorValueTreeState& vts,
                           AudioComponent& ac) :
ElectroModule(editor, vts, ac, 0.04f, 0.215f, 0.05f, 0.18f, 0.8f) //0.05f, 0.132f, 0.05f, 0.18f, 0.8f),
{
    outlineColour = Colours::darkgrey;
    
    //double cutoff = getDial(FilterCutoff)->getSlider().getValue();
//    cutoffLabel.setText(String(cutoff, 2), dontSendNotification);
//    cutoffLabel.setLookAndFeel(&laf);
//    cutoffLabel.setEditable(true);
//    cutoffLabel.setJustificationType(Justification::centred);
//    cutoffLabel.setColour(Label::backgroundColourId, Colours::darkgrey.withBrightness(0.2f));
//    cutoffLabel.addListener(this);
    //addAndMakeVisible(cutoffLabel);
    
    RangedAudioParameter* set = vts.getParameter(ac.getName() + " Type");
    typeCB.addItemList(filterTypeNames, 1);
    typeCB.setSelectedItemIndex(set->convertFrom0to1(set->getValue()), dontSendNotification);
    typeCB.setLookAndFeel(&laf);
    addAndMakeVisible(typeCB);
    comboBoxAttachments.add(new ComboBoxAttachment(vts, ac.getName() + " Type", typeCB));
}

FilterModule::~FilterModule()
{
    sliderAttachments.clear();
    buttonAttachments.clear();
    comboBoxAttachments.clear();
}

void FilterModule::resized()
{
    ElectroModule::resized();
    
    for (int i = 0; i < ac.getParamNames().size(); ++i)
    {
        dials[i]->setBoundsRelative((relDialWidth*(i))+(relDialSpacing * 0.6f*(i+1)),
                                    relTopMargin, relDialWidth, relDialHeight);
    }
    

    typeCB.setBounds(enabledToggle.getRight(), 4, getWidth()*0.3f, enabledToggle.getHeight()-4);
//    typeCB.setBoundsRelative(relLeftMargin, 0.01f, relDialWidth+relDialSpacing, 0.16f);
}

void FilterModule::sliderValueChanged(Slider* slider)
{
    if (slider == &getDial(FilterCutoff)->getSlider())
    {
    }
    else if (MappingTarget* mt = dynamic_cast<MappingTarget*>(slider))
    {
        dynamic_cast<ElectroDial*>(mt->getParentComponent())->sliderValueChanged(slider);
    }
}


void FilterModule::mouseEnter(const MouseEvent& e)
{
    if (MappingTarget* mt = dynamic_cast<MappingTarget*>(e.originalComponent->getParentComponent()))
    {
    }
}

void FilterModule::mouseExit(const MouseEvent& e)
{
}



//==============================================================================
//==============================================================================

EnvModule::EnvModule(ElectroAudioProcessorEditor& editor, AudioProcessorValueTreeState& vts,
                     AudioComponent& ac) :
ElectroModule(editor, vts, ac, 0.03f, 0.14f, 0.06f, 0.16f, 0.84f)
{
    velocityToggle.setButtonText("Scale to velocity");
    addAndMakeVisible(velocityToggle);
    buttonAttachments.add(new ButtonAttachment(vts, ac.getName() + " Velocity", velocityToggle));
}

EnvModule::~EnvModule()
{
    sliderAttachments.clear();
    buttonAttachments.clear();
    comboBoxAttachments.clear();
}

void EnvModule::resized()
{
    ElectroModule::resized();
    
    velocityToggle.setBoundsRelative(relLeftMargin, 0.f, 2*relDialWidth+relDialSpacing, 0.16f);
}

//==============================================================================
//==============================================================================

LFOModule::LFOModule(ElectroAudioProcessorEditor& editor, AudioProcessorValueTreeState& vts,
                     AudioComponent& ac) :
ElectroModule(editor, vts, ac, 0.12f, 0.14f, 0.17f, 0.16f, 0.84f),
chooser("Select wavetable file or folder...",
        File::getSpecialLocation(File::userDocumentsDirectory))
{
    double rate = getDial(LowFreqRate)->getSlider().getValue();
    rateLabel.setText(String(rate, 2) + " Hz", dontSendNotification);
    rateLabel.setLookAndFeel(&laf);
    rateLabel.setEditable(true);
    rateLabel.setJustificationType(Justification::centred);
    rateLabel.setColour(Label::backgroundColourId, Colours::darkgrey.withBrightness(0.2f));
    rateLabel.addListener(this);
    addAndMakeVisible(rateLabel);
    
    RangedAudioParameter* set = vts.getParameter(ac.getName() + " ShapeSet");
    shapeCB.addItemList(lfoShapeSetNames, 1);
    shapeCB.setSelectedItemIndex(set->convertFrom0to1(set->getValue()), dontSendNotification);
    shapeCB.setLookAndFeel(&laf);
    addAndMakeVisible(shapeCB);
    comboBoxAttachments.add(new ComboBoxAttachment(vts, ac.getName() + " ShapeSet", shapeCB));
    
    syncToggle.setButtonText("Sync to note-on");
    addAndMakeVisible(syncToggle);
    buttonAttachments.add(new ButtonAttachment(vts, ac.getName() + " Sync", syncToggle));
}

LFOModule::~LFOModule()
{
    sliderAttachments.clear();
    buttonAttachments.clear();
    comboBoxAttachments.clear();
}

void LFOModule::resized()
{
    ElectroModule::resized();
    
    rateLabel.setBoundsRelative(relLeftMargin-0.3*relDialSpacing, 0.f,
                                relDialWidth+0.6f*relDialSpacing, 0.16f);
    shapeCB.setBoundsRelative(relLeftMargin+relDialWidth+0.7f*relDialSpacing, 0.f,
                              relDialWidth+0.6f*relDialSpacing, 0.16f);
    syncToggle.setBoundsRelative(relLeftMargin+2*relDialWidth+1.7f*relDialSpacing, 0.f,
                                 relDialWidth+0.6f*relDialSpacing, 0.16f);
}

void LFOModule::sliderValueChanged(Slider* slider)
{
    DBG(slider->getSkewFactor());
    if (slider == &getDial(LowFreqRate)->getSlider())
    {
        displayRate();
    }
    else if (MappingTarget* mt = dynamic_cast<MappingTarget*>(slider))
    {
        dynamic_cast<ElectroDial*>(mt->getParentComponent())->sliderValueChanged(slider);
        displayRateMapping(mt);
    }
}

void LFOModule::labelTextChanged(Label* label)
{
    if (label == &rateLabel)
    {
        auto value = rateLabel.getText().getDoubleValue();
        getDial(LowFreqRate)->getSlider().setValue(value);
    }
}

void LFOModule::comboBoxChanged(ComboBox *comboBox)
{
    if (comboBox == &shapeCB)
    {
        if (shapeCB.getSelectedItemIndex() > SawPulseLFOShapeSet &&
            shapeCB.getSelectedItemIndex() != PulseLFOShapeSet)
        {
            getDial(LowFreqShape)->setAlpha(0.5f);
            getDial(LowFreqShape)->setInterceptsMouseClicks(false, false);
        }
        else
        {
            getDial(LowFreqShape)->setAlpha(1.f);
            getDial(LowFreqShape)->setInterceptsMouseClicks(true, true);
        }
    }
}

void LFOModule::mouseEnter(const MouseEvent& e)
{
    if (MappingTarget* mt = dynamic_cast<MappingTarget*>(e.originalComponent->getParentComponent()))
    {
        displayRateMapping(mt);
    }
}

void LFOModule::mouseExit(const MouseEvent& e)
{
    displayRate();
}

void LFOModule::displayRate()
{
    double rate = getDial(LowFreqRate)->getSlider().getValue();
    rateLabel.setColour(Label::textColourId, Colours::gold.withBrightness(0.95f));
    rateLabel.setText(String(rate, 3) + " Hz", dontSendNotification);
}

void LFOModule::displayRateMapping(MappingTarget* mt)
{
    if (!mt->isActive()) displayRate();
    else if (mt->getParentComponent() == getDial(LowFreqRate))
    {
        auto start = mt->getModel().start;
        auto end = mt->getModel().end;
        rateLabel.setColour(Label::textColourId, mt->getColour());
        String text;
        if (mt->isBipolar())
        {
            if (mt->getSkewFactor() != 1. && start != end)
            {
                text = (start >= 0 ? "+" : "-");
                text += String(fabs(start), 2) + "/";
                text += (end >= 0 ? "+" : "-");
                text += String(fabs(end), 2) + " Hz";
            }
            else
            {
                text = String::charToString(0xb1);
                text += String(fabs(mt->getModel().end), 2) + " Hz";
            }
        }
        else
        {
            text = (end >= 0 ? "+" : "-");
            text += String(fabs(end), 2) + " Hz";
        }
        rateLabel.setText(text, dontSendNotification);
    }
}

//==============================================================================
//==============================================================================

OutputModule::OutputModule(ElectroAudioProcessorEditor& editor, AudioProcessorValueTreeState& vts,
                           AudioComponent& ac) :
ElectroModule(editor, vts, ac, 0.07f, 0.22f, 0.07f, 0.11f, 0.78f)
{
    outlineColour = Colours::darkgrey;
    meters.setChannelFormat(juce::AudioChannelSet::stereo());
    sd::SoundMeter::Options meterOptions;
    meterOptions.faderEnabled     = true;
    meterOptions.headerEnabled    = true;
    meterOptions.peakSegment_db    = -3.0f;
    meterOptions.warningSegment_db = -12.0f;
    meters.setOptions (meterOptions);
    addAndMakeVisible (meters);
    masterDial = std::make_unique<ElectroDial>(editor, "Master", "Master", false, false);
    sliderAttachments.add(new SliderAttachment(vts, "Master", masterDial->getSlider()));
  
    addAndMakeVisible(masterDial.get());
    startTimerHz (30);
    
}

// The 'polling' timer.
void OutputModule::timerCallback()
{
   // Loop through all meters (channels)...
   for (int meterIndex = 0; meterIndex < meters.getNumChannels(); ++meterIndex)
   {
      // Get the level, of the specified meter (channel), from the audio processor...
      meters.setInputLevel (meterIndex, editor.processor.getPeakLevel (meterIndex));
   }

   meters.refresh();
}
OutputModule::~OutputModule()
{
    sliderAttachments.clear();
    buttonAttachments.clear();
    comboBoxAttachments.clear();
}

void OutputModule::resized()
{
    ElectroModule::resized();
    
    masterDial->setBoundsRelative(0.65f, relTopMargin, 0.17f, relDialHeight);
    meters.setBoundsRelative(0.8f, relTopMargin, 0.17f, relDialHeight);
}

