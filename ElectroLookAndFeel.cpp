/*
 ==============================================================================
 
 ElectroLookAndFeel.cpp
 Created: 1 Jun 2021 4:52:36pm
 Author:  Matthew Wang
 
 ==============================================================================
 */

#include "ElectroLookAndFeel.h"
#include "ElectroComponents.h"

ElectroLookAndFeel::ElectroLookAndFeel()
{
    setColour(ComboBox::backgroundColourId, Colour(10, 10, 10));
    setColour(ComboBox::textColourId, Colours::lightgrey);
    setColour(ComboBox::buttonColourId, Colours::black);
    setColour(ComboBox::outlineColourId, Colours::transparentBlack);
    setColour(ComboBox::arrowColourId, Colours::lightgrey);
    
    setColour(PopupMenu::backgroundColourId, Colour(10, 10, 10));
    setColour(PopupMenu::textColourId, Colours::lightgrey);
    
    setColour(Label::textColourId, Colours::gold.withBrightness(0.95f));
    
    setColour(TextEditor::textColourId, Colours::gold.withBrightness(0.95f));
    setColour(TextEditor::backgroundColourId, Colours::transparentWhite);
    setColour(TextEditor::outlineColourId, Colours::transparentWhite);
    
    setColour(TextButton::textColourOffId, Colours::white.withBrightness(0.8f));
    setColour(TextButton::textColourOnId, Colours::gold.withBrightness(0.95f));
    setColour(TextButton::buttonColourId, Colour(40, 40, 40));
    setColour(TextButton::buttonOnColourId, Colour(30, 30, 30));
    
    tp = Typeface::createSystemTypefaceFor(BinaryData::EuphemiaCAS_ttf,
                                           BinaryData::EuphemiaCAS_ttfSize);
}

ElectroLookAndFeel::~ElectroLookAndFeel()
{
    clearSingletonInstance();
}

Font ElectroLookAndFeel::getPopupMenuFont (void)
{
    Font font (tp);
    return font;
}

Font ElectroLookAndFeel::getTextButtonFont (TextButton& b, int buttonHeight)
{
    Font font (tp);
    float h = buttonHeight * 0.4f;
    if (b.getToggleState()) h *= 1.1f;
    font.setHeight(h);
    return font;
}

Font ElectroLookAndFeel2::getTextButtonFont (TextButton& b, int buttonHeight)
{
    Font font (tp);
    font.setHeight(14);
    return font;
}

Font ElectroLookAndFeel::getLabelFont (Label& label)
{
    Font font (tp);
    float height = label.getHeight() * 0.8f;
    if (label.isEditable() && dynamic_cast<CopedentTable::EditableTextCustomComponent*>(&label) != nullptr)
        height = label.getHeight() * 0.5f;
    
    font.setHeight(height);
    font.setDefaultMinimumHorizontalScaleFactor(0.01f);
    return font;
}

Font ElectroLookAndFeel2::getLabelFont (Label& label)
{
    Font font (tp);
    font.setHeight(18);
    font.setDefaultMinimumHorizontalScaleFactor(0.01f);
    return font;
}

void ElectroLookAndFeel::drawButtonBackground (Graphics& g,
                                          Button& button,
                                          const Colour& backgroundColour,
                                          bool shouldDrawButtonAsHighlighted,
                                          bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);
    
    auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
    .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);
    
    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        baseColour = baseColour.contrasting (shouldDrawButtonAsDown ? 0.2f : 0.05f);
    
    g.setColour (baseColour);
    
    auto flatOnLeft   = button.isConnectedOnLeft();
    auto flatOnRight  = button.isConnectedOnRight();
    auto flatOnTop    = button.isConnectedOnTop();
    auto flatOnBottom = button.isConnectedOnBottom();
    
    if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
    {
        Path path;
        path.addRoundedRectangle (bounds.getX(), bounds.getY(),
                                  bounds.getWidth(), bounds.getHeight(),
                                  0, 0,
                                  ! (flatOnLeft  || flatOnTop),
                                  ! (flatOnRight || flatOnTop),
                                  ! (flatOnLeft  || flatOnBottom),
                                  ! (flatOnRight || flatOnBottom));
        
        g.fillPath (path);
        
        g.setColour (button.findColour (ComboBox::outlineColourId));
        g.strokePath (path, PathStrokeType (1.0f));
    }
    else
    {
        g.fillRoundedRectangle (bounds, 0);
        
        g.setColour (button.findColour (ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds, 0, 1.0f);
    }
}

void ElectroLookAndFeel::drawButtonText (Graphics& g, TextButton& button,
                                    bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/)
{
    Font font (getTextButtonFont (button, button.getHeight()));
    g.setFont (font);
    g.setColour (button.findColour (button.getToggleState() ? TextButton::textColourOnId
                                    : TextButton::textColourOffId)
                 .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));
    
    const int yIndent = jmin (7, button.proportionOfHeight (0.3f));
    const int cornerSize = 0;//jmin (button.getHeight(), button.getWidth()) / 2;
    
    const int fontHeight = roundToInt (font.getHeight() * 0.6f);
    const int leftIndent  = jmin (fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
    const int rightIndent = jmin (fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
    const int textWidth = button.getWidth() - leftIndent - rightIndent;
    
    if (textWidth > 0)
        g.drawFittedText (button.getButtonText(),
                          leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2,
                          Justification::centred, 2);
}

void ElectroLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                                      float sliderPos,
                                      float minSliderPos,
                                      float maxSliderPos,
                                      const Slider::SliderStyle style, Slider& slider)
{
    if (slider.isBar())
    {
        g.setColour (slider.findColour (Slider::trackColourId));
        g.fillRect (slider.isHorizontal() ?
                    Rectangle<float> (static_cast<float> (x), (float) y + 0.5f,
                                      sliderPos - (float) x, (float) height - 1.0f)
                    : Rectangle<float> ((float) x + 0.5f, sliderPos, (float) width - 1.0f,
                                        (float) y + ((float) height - sliderPos)));
        g.setColour (slider.findColour (Slider::backgroundColourId));
        g.fillRect (Rectangle<float> (static_cast<float> (x), (float) y + 0.5f,
                                      (float) width, (float) height));
    }
    else
    {
        auto isTwoVal   = (style == Slider::SliderStyle::TwoValueVertical   || style == Slider::SliderStyle::TwoValueHorizontal);
        auto isThreeVal = (style == Slider::SliderStyle::ThreeValueVertical || style == Slider::SliderStyle::ThreeValueHorizontal);
        
        auto trackWidth = jmin (6.0f, slider.isHorizontal() ? (float) height * 0.25f : (float) width * 0.25f);
        
        Point<float> startPoint (slider.isHorizontal() ? (float) x : (float) x + (float) width * 0.5f,
                                 slider.isHorizontal() ? (float) y + (float) height * 0.5f : (float) (height + y));
        
        Point<float> endPoint (slider.isHorizontal() ? (float) (width + x) : startPoint.x,
                               slider.isHorizontal() ? startPoint.y : (float) y);
        
        Path backgroundTrack;
        backgroundTrack.startNewSubPath (startPoint);
        backgroundTrack.lineTo (endPoint);
        g.setColour (slider.findColour (Slider::backgroundColourId));
        g.strokePath (backgroundTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });
        
        Path valueTrack;
        Point<float> minPoint, maxPoint, thumbPoint;
        
        if (isTwoVal || isThreeVal)
        {
            minPoint = { slider.isHorizontal() ? minSliderPos : (float) width * 0.5f,
                slider.isHorizontal() ? (float) height * 0.5f : minSliderPos };
            
            if (isThreeVal)
                thumbPoint = { slider.isHorizontal() ? sliderPos : (float) width * 0.5f,
                    slider.isHorizontal() ? (float) height * 0.5f : sliderPos };
            
            maxPoint = { slider.isHorizontal() ? maxSliderPos : (float) width * 0.5f,
                slider.isHorizontal() ? (float) height * 0.5f : maxSliderPos };
        }
        else
        {
            auto kx = slider.isHorizontal() ? sliderPos : ((float) x + (float) width * 0.5f);
            auto ky = slider.isHorizontal() ? ((float) y + (float) height * 0.5f) : sliderPos;
            
            minPoint = startPoint;
            maxPoint = { kx, ky };
        }
        
        auto thumbWidth = getSliderThumbRadius (slider);
        
        valueTrack.startNewSubPath (minPoint);
        valueTrack.lineTo (isThreeVal ? thumbPoint : maxPoint);
        g.setColour (slider.findColour (Slider::trackColourId));
        g.strokePath (valueTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });
        
        if (! isTwoVal)
        {
            g.setColour (slider.findColour (Slider::thumbColourId));
            g.fillEllipse (Rectangle<float> (static_cast<float> (thumbWidth), static_cast<float> (thumbWidth)).withCentre (isThreeVal ? thumbPoint : maxPoint));
        }
        
        if (isTwoVal || isThreeVal)
        {
            auto sr = jmin (trackWidth, (slider.isHorizontal() ? (float) height : (float) width) * 0.4f);
            auto pointerColour = slider.findColour (Slider::thumbColourId);
            
            if (slider.isHorizontal())
            {
                drawPointer (g, minSliderPos - sr,
                             jmax (0.0f, (float) y + (float) height * 0.5f - trackWidth * 2.0f),
                             trackWidth * 2.0f, pointerColour, 2);
                
                drawPointer (g, maxSliderPos - trackWidth,
                             jmin ((float) (y + height) - trackWidth * 2.0f, (float) y + (float) height * 0.5f),
                             trackWidth * 2.0f, pointerColour, 4);
            }
            else
            {
                drawPointer (g, jmax (0.0f, (float) x + (float) width * 0.5f - trackWidth * 2.0f),
                             minSliderPos - trackWidth,
                             trackWidth * 2.0f, pointerColour, 1);
                
                drawPointer (g, jmin ((float) (x + width) - trackWidth * 2.0f, (float) x + (float) width * 0.5f), maxSliderPos - sr,
                             trackWidth * 2.0f, pointerColour, 3);
            }
        }
    }
}


void ElectroLookAndFeel::drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider)
{
    auto radius = jmin(width / 2, height / 2) - width*0.15f;
    auto centreX = x + width * 0.5f;
    auto centreY = y + height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    float b = rw * 0.025f;
    
    //        Path arc;
    //        arc.startNewSubPath(rx + rw*0.5f, ry + rw*0.5f);
    //        arc.addArc(rx - b*4, ry - b*4, rw + b*8, rw + b*8, rotaryStartAngle, angle);
    //        g.setColour(Colours::lightblue);
    //        g.fillPath(arc);
    
    Path lower;
    auto lowerLength = fmax(radius * 0.5f, 8.0f);
    auto lowerThickness = lowerLength * 0.125f;
    lower.addRectangle(-lowerThickness * 0.5f, -radius*1.5, lowerThickness, lowerLength);
    lower.applyTransform(AffineTransform::rotation(rotaryStartAngle).translated(centreX, centreY));
    g.setColour(Colours::lightgrey);
    g.fillPath(lower);
    
    Path upper;
    auto upperLength = fmax(radius * 0.5f, 8.0f);
    auto upperThickness = upperLength * 0.125f;
    upper.addRectangle(-upperThickness * 0.5f, -radius*1.5, upperThickness, upperLength);
    upper.applyTransform(AffineTransform::rotation(rotaryEndAngle).translated(centreX, centreY));
    g.setColour(Colours::lightgrey);
    g.fillPath(upper);
    
    g.setColour(Colours::dimgrey);
    g.fillEllipse(rx - b, ry - b, rw + b*2, rw + b*3);
    
    g.setColour(Colours::white);
    g.fillEllipse(rx, ry, rw, rw);
    
    Path pointer;
    auto pointerLength = fmax(radius * 0.6f, 4.0f);
    auto pointerThickness = pointerLength * 0.25f;
    pointer.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
    pointer.applyTransform(AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(Colours::black);
    g.fillPath(pointer);
}

void ElectroLookAndFeel::drawPopupMenuItem (Graphics& g, const Rectangle<int>& area,
                                       const bool isSeparator, const bool isActive,
                                       const bool isHighlighted, const bool isTicked,
                                       const bool hasSubMenu, const String& text,
                                       const String& shortcutKeyText,
                                       const Drawable* icon, const Colour* const textColourToUse)
{
    if (isSeparator)
    {
        auto r  = area.reduced (5, 0);
        r.removeFromTop (roundToInt ((r.getHeight() * 0.5f) - 0.5f));
        
        g.setColour (findColour (PopupMenu::textColourId).withAlpha (0.3f));
        g.fillRect (r.removeFromTop (1));
    }
    else
    {
        auto textColour = (textColourToUse == nullptr ? findColour (PopupMenu::textColourId)
                           : *textColourToUse);
        
        auto r  = area.reduced (1);
        
        if (isHighlighted && isActive)
        {
            g.setColour (findColour (PopupMenu::highlightedBackgroundColourId));
            g.fillRect (r);
            
            g.setColour (findColour (PopupMenu::highlightedTextColourId));
        }
        else
        {
            g.setColour (textColour.withMultipliedAlpha (isActive ? 1.0f : 0.5f));
        }
        
        r.reduce (jmin (5, area.getWidth() / 20), 0);
        
        auto font = getPopupMenuFont();
        
        const auto maxFontHeight = r.getHeight() / 1.2f;
        
        if (font.getHeight() > maxFontHeight)
            font.setHeight (maxFontHeight);
        
        g.setFont (font);
        
        auto iconArea = r.removeFromRight (roundToInt (maxFontHeight)).toFloat();
        
        if (icon != nullptr)
        {
            icon->drawWithin (g, iconArea, RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
        }
        else if (isTicked)
        {
            const auto tick = getTickShape (1.0f);
            g.fillPath (tick, tick.getTransformToScaleToFit (iconArea.reduced (iconArea.getWidth() / 5, 0).toFloat(), true));
        }
        
        if (hasSubMenu)
        {
            const auto arrowH = 0.6f * getPopupMenuFont().getAscent();
            
            const auto x = (float) r.removeFromRight ((int) arrowH).getX();
            const auto halfH = (float) r.getCentreY();
            
            Path path;
            path.startNewSubPath (x, halfH - arrowH * 0.5f);
            path.lineTo (x + arrowH * 0.6f, halfH);
            path.lineTo (x, halfH + arrowH * 0.5f);
            
            g.strokePath (path, PathStrokeType (2.0f));
        }
        
        r.removeFromRight (3);
        g.drawFittedText (text, r, Justification::centredLeft, 1);
        
        if (shortcutKeyText.isNotEmpty())
        {
            auto f2 = font;
            f2.setHeight (f2.getHeight() * 0.75f);
            f2.setHorizontalScale (0.95f);
            g.setFont (f2);
            g.drawText (shortcutKeyText, r, Justification::centredLeft, true);
        }
    }
}

int ElectroLookAndFeel::getTabButtonBestWidth(TabBarButton & button, int tabDepth)
{
    int n = button.getTabbedButtonBar().getNumTabs();
    int w = button.getTabbedButtonBar().getWidth() / n;
    int diff = button.getTabbedButtonBar().getWidth() - (w*n);
    if (button.getIndex() == 0) w += diff + 1;
    return w;
}

Rectangle<int> ElectroLookAndFeel::getTabButtonExtraComponentBounds (const TabBarButton& button, Rectangle<int>& textArea, Component& comp)
{
    auto depth = button.getHeight();
    return textArea.reduced(depth*0.2f, depth*0.2f);
}

void ElectroLookAndFeel::drawTableHeaderColumn (Graphics& g, TableHeaderComponent& header,
                                           const String& columnName, int /*columnId*/,
                                           int width, int height, bool isMouseOver, bool isMouseDown,
                                           int columnFlags)
{
    Rectangle<int> area (width, height);
    area.reduce (4, 0);
    
    g.setColour (header.findColour (TableHeaderComponent::textColourId));
    g.setFont (Font ((float) height * 0.5f, Font::bold));
    g.drawFittedText (columnName, area, Justification::centred, 1);
}


JUCE_IMPLEMENT_SINGLETON(ElectroLookAndFeel)
