/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    auto enabled = slider.isEnabled();

    g.setColour(enabled ? Colour(255u, 126u, 13u) : Colours::darkgrey); //apka Digital Color Meter
    g.fillEllipse(bounds);

    g.setColour(enabled ? Colour(207u, 34u, 0u) : Colours::grey);
    g.drawEllipse(bounds, 2.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();

        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle); //zmapowanie wartosci radionow pomiedzy granice rotary slidera

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(center);

        g.setColour(enabled ? Colours::black : Colours::darkgrey);
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics& g,
    juce::ToggleButton& toggleButton,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    using namespace juce;

    Path powerButton;

    auto bounds = toggleButton.getLocalBounds();
    auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 80;
    auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

    float ang = 30.f;

    size -= 10;

    powerButton.addCentredArc(r.getCentreX(),
        r.getCentreY(),
        size * 0.5,
        size * 0.5, 0.f,
        degreesToRadians(ang),
        degreesToRadians(360.f - ang),
        true);

    powerButton.startNewSubPath(r.getCentreX(), r.getY());
    powerButton.lineTo(r.getCentre());

    PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

    auto color = toggleButton.getToggleState() ? Colour(207u, 34u, 0u) : Colours::dimgrey;

    g.setColour(color);
    g.strokePath(powerButton, pst);
    g.drawEllipse(r, 2);
}

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 55.f);
    auto endAng = degreesToRadians(180.f - 55.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    getLookAndFeel().drawRotarySlider(g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
        startAng,
        endAng,
        *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(255u, 126u, 13u));

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.22f);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);

        Rectangle<float> r;
        auto str = labels[i].label;

        if (labels[i].pos == 1.22f) {
            g.setFont(getTextHeight() + 1);
            r.setSize(g.getCurrentFont().getStringWidthFloat(str), getTextHeight() - 6);
            r.setCentre(c);
            r.setY(r.getY() + getTextHeight() - 6);
        }
        else {
            g.setFont(getTextHeight() - 2);
            r.setSize(g.getCurrentFont().getStringWidthFloat(str), getTextHeight() + 2);
            r.setCentre(c);
            r.setY(r.getY() + getTextHeight() + 2);
        }

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::verticallyCentred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    //return getLocalBounds();
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
    {
        return choiceParam->getCurrentChoiceName();
    }

    juce::String str;
    bool addK = false;

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
        {
            auto val = getValue();
            float minValue = floatParam->range.start;
            float maxValue = floatParam->range.end;

            if (val > 999.f)
            {
                val /= 1000.f;
                addK = true;
            }

            if (suffix == "%") {
                float percentValue = round((val - minValue) / (maxValue - minValue) * 100.f);
                str = juce::String(percentValue);
            }
            else if (suffix == "ms") {
                float msValue = val * 1000.f;
                str = juce::String(msValue);
            }
            else {
                str = juce::String(val);
            }     
        }
        else
        {
            jassertfalse; //probably not necessery
        }
    }

    if (suffix.isNotEmpty())
    {
        str << " ";

        if (addK)
            str << "k";

        str << suffix;
    }

    return str;
}

void RotarySliderWithLabels::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown() && this->isEnabled())
    {
        showTextEditor();
    }
    else
    {
        juce::Slider::mouseDown(event);
    }
}

void RotarySliderWithLabels::showTextEditor()
{
    auto* editor = new juce::TextEditor();
    auto* editorListener = new juce::TextEditor::Listener;
    auto localBounds = getLocalBounds();
    editor->setJustification(juce::Justification::centred);
    if (suffix == "ms") {
        editor->setText(juce::String(getValue() * 1000));
    }
    else if (suffix == "%") {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
        {
            auto val = getValue();
            float minValue = floatParam->range.start;
            float maxValue = floatParam->range.end;

            editor->setText(juce::String(round((val - minValue) / (maxValue - minValue) * 100.f)));

        }
        else
        {
            jassertfalse; //probably not necessery
        }
    }
    else {
        editor->setText(juce::String(getValue()));
    }

    editor->setBounds(localBounds.getCentreX(), localBounds.getCentreY(), 50, 25);
    editor->addListener(editorListener);

    editor->onReturnKey = [this, editor]() {
        updateSliderValue(editor, this);
        };

    editor->onFocusLost = [this, editor]() {
        updateSliderValue(editor, this);
        };

    addAndMakeVisible(editor);
    editor->grabKeyboardFocus();
}

void RotarySliderWithLabels::updateSliderValue(juce::TextEditor* editor, juce::Slider* slider)
{
    // Update the slider value
    if (suffix == "ms") {
        double newVal = editor->getText().getDoubleValue() / double(1000);
        slider->setValue(newVal);
    }
    else if (suffix == "%") {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
        {
            auto val = getValue();
            float minVal = floatParam->range.start;
            float maxVal = floatParam->range.end;

            double newVal = editor->getText().getDoubleValue();
            newVal = minVal + (newVal / 100.f) * (maxVal - minVal);
            slider->setValue(newVal);
        }
        else
        {
            jassertfalse; //probably not necessery
        }
    }
    else {
        double newVal = editor->getText().getDoubleValue();
        slider->setValue(newVal);
    }

    // Remove the editor
    editor->removeChildComponent(editor);
    delete editor;
}

void PowerButton::paint(juce::Graphics& g)
{
    using namespace juce;

    auto buttonBounds = getButtonBounds();

    getLookAndFeel().drawToggleButton(g,
        *this,
        true,
        true);

    auto center = buttonBounds.toFloat().getCentre();
    auto radius = buttonBounds.getWidth() * 0.5f;

    g.setColour(Colour(255u, 126u, 13u));

    auto numChoices = names.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, degreesToRadians(180.f));

        Rectangle<float> r;
        auto str = names[i].name;

        g.setFont(getTextHeight());
        r.setSize(g.getCurrentFont().getStringWidthFloat(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight() - 20);

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::verticallyCentred, 1);
    }
}

juce::Rectangle<int> PowerButton::getButtonBounds() const
{
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;
}

//==============================================================================
EnvelopeAudioProcessorEditor::EnvelopeAudioProcessorEditor (EnvelopeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    gainFactorSlider(*audioProcessor.apvts.getParameter("Gain"), " "),
    qFactorSlider(*audioProcessor.apvts.getParameter("Q"), " "),
    dryWetMixSlider(*audioProcessor.apvts.getParameter("Dry/Wet Mix"), "%"),
    attackTimeSlider(*audioProcessor.apvts.getParameter("Attack Time"), "ms"),
    releaseTimeSlider(*audioProcessor.apvts.getParameter("Release Time"), "ms"),
    bandStartSlider(*audioProcessor.apvts.getParameter("Band Start"), "Hz"),
    bandWidthSlider(*audioProcessor.apvts.getParameter("Band Width"), "Hz"),

    gainFactorSliderAttachment(audioProcessor.apvts, "Gain", gainFactorSlider),
    qFactorSliderAttachment(audioProcessor.apvts, "Q", qFactorSlider),
    dryWetMixSliderAttachment(audioProcessor.apvts, "Dry/Wet Mix", dryWetMixSlider),
    attackTimeSliderAttachment(audioProcessor.apvts, "Attack Time", attackTimeSlider),
    releaseTimeSliderAttachment(audioProcessor.apvts, "Release Time", releaseTimeSlider),
    bandStartSliderAttachment(audioProcessor.apvts, "Band Start", bandStartSlider),
    bandWidthSliderAttachment(audioProcessor.apvts, "Band Width", bandWidthSlider),

    bypassButtonAttachment(audioProcessor.apvts, "Bypass", bypassButton)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    auto chainSettings = getChainSettings(audioProcessor.apvts);

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    gainFactorSlider.labels.add({ 0.f, "1" });
    gainFactorSlider.labels.add({ 1.22f, "Gain Factor" });
    gainFactorSlider.labels.add({ 1.f, "30" });

    qFactorSlider.labels.add({ 0.f, "0.1" });
    qFactorSlider.labels.add({ 1.22f, "Q Factor" });
    qFactorSlider.labels.add({ 1.f, "10" });

    dryWetMixSlider.labels.add({ 0.f, "0 %" });
    dryWetMixSlider.labels.add({ 1.22f, "Dry/Wet Mix" });
    dryWetMixSlider.labels.add({ 1.f, "100 %" });

    attackTimeSlider.labels.add({ 0.f, "1 ms" });
    attackTimeSlider.labels.add({ 1.22f, "Attack Time" });
    attackTimeSlider.labels.add({ 1.f, "50 ms" });

    releaseTimeSlider.labels.add({ 0.f, "50 ms" });
    releaseTimeSlider.labels.add({ 1.22f, "Release Time" });
    releaseTimeSlider.labels.add({ 1.f, "500 ms" });

    bandStartSlider.labels.add({ 0.f, "50 Hz" });
    bandStartSlider.labels.add({ 1.22f, "Band Start" });
    bandStartSlider.labels.add({ 1.f, "2 kHz" });

    bandWidthSlider.labels.add({ 0.f, "50 Hz" });
    bandWidthSlider.labels.add({ 1.22f, "Band Width" });
    bandWidthSlider.labels.add({ 1.f, "10 kHz" });

    bypassButton.names.add({ "BYPASS" });

    bypassButton.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<EnvelopeAudioProcessorEditor>(this);

    bypassButton.onClick = [safePtr]()
        {
            if (auto* comp = safePtr.getComponent()) {
                auto bypassed = comp->bypassButton.getToggleState();

                comp->gainFactorSlider.setEnabled(!bypassed);
                comp->qFactorSlider.setEnabled(!bypassed);
                comp->dryWetMixSlider.setEnabled(!bypassed);
                comp->attackTimeSlider.setEnabled(!bypassed);
                comp->releaseTimeSlider.setEnabled(!bypassed);
                comp->bandStartSlider.setEnabled(!bypassed);
                comp->bandWidthSlider.setEnabled(!bypassed);

            }
        };

    setSize (600, 400);
}

EnvelopeAudioProcessorEditor::~EnvelopeAudioProcessorEditor()
{
    bypassButton.setLookAndFeel(nullptr);
}

//==============================================================================
void EnvelopeAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black);
}

void EnvelopeAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();

    bounds.removeFromTop(20);
    bounds.removeFromBottom(20);

    auto filterArea = bounds.removeFromTop(bounds.getHeight() * 1.f / 3.f);
    gainFactorSlider.setBounds(filterArea.removeFromLeft(filterArea.getWidth() * 1.f / 3.f));
    qFactorSlider.setBounds(filterArea.removeFromLeft(filterArea.getWidth() * 0.5f));
    dryWetMixSlider.setBounds(filterArea);

    auto envelopeArea = bounds.removeFromTop(bounds.getHeight() * 0.5f);
    envelopeArea.removeFromTop(10);
    attackTimeSlider.setBounds(envelopeArea.removeFromLeft(envelopeArea.getWidth() * 0.25f));
    releaseTimeSlider.setBounds(envelopeArea.removeFromLeft(envelopeArea.getWidth() * 1.f/3.f));
    bandStartSlider.setBounds(envelopeArea.removeFromLeft(envelopeArea.getWidth() * 0.5f));
    bandWidthSlider.setBounds(envelopeArea);

    bypassButton.setBounds(bounds);
}

std::vector<juce::Component*> EnvelopeAudioProcessorEditor::getComps()
{
    return
    {
        &gainFactorSlider,
        &qFactorSlider,
        &dryWetMixSlider,
        &attackTimeSlider,
        &releaseTimeSlider,
        &bandStartSlider,
        &bandWidthSlider,

        &bypassButton
    };
}