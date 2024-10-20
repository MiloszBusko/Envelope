/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EnvelopeAudioProcessor::EnvelopeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

EnvelopeAudioProcessor::~EnvelopeAudioProcessor()
{
}

//==============================================================================
const juce::String EnvelopeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EnvelopeAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EnvelopeAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EnvelopeAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EnvelopeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EnvelopeAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EnvelopeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EnvelopeAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EnvelopeAudioProcessor::getProgramName (int index)
{
    return {};
}

void EnvelopeAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EnvelopeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // Initialize peak filters
    filters.clear();
    for (int i = 0; i < getTotalNumInputChannels(); ++i) {
        juce::IIRFilter* filter;
        filters.add(filter = new juce::IIRFilter());
    }

    // Initialize envelopes
    for (int i = 0; i < getTotalNumInputChannels(); ++i)
        envelopes.add(0.0f);
}

void EnvelopeAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EnvelopeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EnvelopeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    auto chainSettings = getChainSettings(apvts);

    auto gain = chainSettings.gainFactor;
    auto q = chainSettings.qFactor;
    auto mix = chainSettings.dryWetMix;
    auto attack = chainSettings.attackTime;
    auto release = chainSettings.releaseTime;
    auto bs = chainSettings.bandStart;
    auto bw = chainSettings.bandWidth;
    auto bypass = chainSettings.bypass;

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (!bypass) {
        for (int channel = 0; channel < totalNumInputChannels; ++channel) {
            float* channelData = buffer.getWritePointer(channel);

            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {

                const float in = channelData[sample];

                // Envelope
                float envelope;
                aa = exp(-1.0f / (attack * getSampleRate()));
                ar = exp(-1.0f / (release * getSampleRate()));

                // Level detector
                if (fabs(in) > envelopes[channel]) {
                    envelope = aa * envelopes[channel] + (1.0f - aa) * fabs(in);
                }
                else {
                    envelope = ar * envelopes[channel] + (1.0f - ar) * fabs(in);
                }

                envelopes.set(channel, envelope);

                // Update filter parameters depending on level
                fc = bs + bw * envelopes[channel];

                // Peak filter
                for (int i = 0; i < filters.size(); ++i) {
                    filters[i]->setCoefficients(juce::IIRCoefficients::makePeakFilter(getSampleRate(), fc, q, gain));
                }

                // Filtering
                float filtered = filters[channel]->processSingleSampleRaw(in);
                channelData[sample] = filtered * mix + channelData[sample] * (1.f - mix);


            }
        }
    }

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

//==============================================================================
bool EnvelopeAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EnvelopeAudioProcessor::createEditor()
{
    return new EnvelopeAudioProcessorEditor(*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void EnvelopeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void EnvelopeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.gainFactor = apvts.getRawParameterValue("Gain")->load();
    settings.qFactor = apvts.getRawParameterValue("Q")->load();
    settings.dryWetMix = apvts.getRawParameterValue("Dry/Wet Mix")->load();
    settings.attackTime = apvts.getRawParameterValue("Attack Time")->load();
    settings.releaseTime = apvts.getRawParameterValue("Release Time")->load();
    settings.bandStart = apvts.getRawParameterValue("Band Start")->load();
    settings.bandWidth = apvts.getRawParameterValue("Band Width")->load();

    settings.bypass = apvts.getRawParameterValue("Bypass")->load() > 0.5f;

    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout EnvelopeAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("Gain", "Gain", juce::NormalisableRange<float>(1.0f, 30.0f, 0.1f, 1.f), 6.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Q", "Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f, 1.f), 3.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Dry/Wet Mix", "Dry/Wet Mix", juce::NormalisableRange<float>(0.00f, 1.00f, 0.01f, 1.f), 1.00f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Attack Time", "Attack Time", juce::NormalisableRange<float>(0.001f, 0.050f, 0.001f, 1.f), 0.001f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Release Time", "Release Time", juce::NormalisableRange<float>(0.050f, 0.500f, 0.001f, 1.f), 0.080f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Band Start", "Band Start", juce::NormalisableRange<float>(50.f, 2000.f, 1.f, 1.f), 250.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Band Width", "Band Width", juce::NormalisableRange<float>(50.f, 10000.f, 1.f, 1.f), 1000.f));

    layout.add(std::make_unique<juce::AudioParameterBool>("Bypass", "Bypass", false));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EnvelopeAudioProcessor();
}
