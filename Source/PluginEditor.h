/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override;

    void drawToggleButton(juce::Graphics& g,
        juce::ToggleButton& toggleButton,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rap),
        suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }

    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }

    struct LabelPos
    {
        float pos;
        juce::String label;
    };

    juce::Array<LabelPos> labels;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    LookAndFeel lnf;

    juce::RangedAudioParameter* param;
    juce::RangedAudioParameter* name;
    juce::String suffix;

    void showTextEditor();
    void updateSliderValue(juce::TextEditor* editor, juce::Slider* slider);
};

struct PowerButton : juce::ToggleButton
{
    struct ButtonName
    {
        juce::String name;
    };

    juce::Array<ButtonName> names;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getButtonBounds() const;
    int getTextHeight() const { return 14; }
};

//==============================================================================
/**
*/

class EnvelopeAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    EnvelopeAudioProcessorEditor (EnvelopeAudioProcessor&);
    ~EnvelopeAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EnvelopeAudioProcessor& audioProcessor;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    RotarySliderWithLabels gainFactorSlider, qFactorSlider, dryWetMixSlider,
        attackTimeSlider, releaseTimeSlider, bandStartSlider, bandWidthSlider;

    Attachment gainFactorSliderAttachment, qFactorSliderAttachment, dryWetMixSliderAttachment,
        attackTimeSliderAttachment, releaseTimeSliderAttachment, bandStartSliderAttachment, bandWidthSliderAttachment;

    PowerButton bypassButton;

    ButtonAttachment bypassButtonAttachment;

    LookAndFeel lnf;

    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeAudioProcessorEditor)
};
