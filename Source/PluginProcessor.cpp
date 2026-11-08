#include "PluginProcessor.h"
#include "PluginEditor.h"

BeneathAudioProcessor::BeneathAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "BENEATH", createLayout())
{}

juce::AudioProcessorValueTreeState::ParameterLayout BeneathAudioProcessor::createLayout()
{
    using Float = juce::AudioParameterFloat;
    using Choice = juce::AudioParameterChoice;
    using Range = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<Float> ("sourceDepth", "Speaker Depth", Range (0.0f, 1.0f), 0.55f));
    layout.add (std::make_unique<Float> ("hydroDepth", "Hydrophone Depth", Range (0.0f, 1.0f), 0.48f));
    layout.add (std::make_unique<Float> ("distance", "Distance", Range (0.0f, 1.0f), 0.35f));
    layout.add (std::make_unique<Float> ("waterDepth", "Water Depth", Range (0.0f, 1.0f), 0.45f));
    layout.add (std::make_unique<Float> ("shore", "Rising Seabed", Range (0.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<Choice> ("seabed", "Seabed",
        juce::StringArray { "Mud", "Sand", "Gravel", "Rock", "Coral" }, 1));
    layout.add (std::make_unique<Float> ("waves", "Waves", Range (0.0f, 1.0f), 0.25f));
    layout.add (std::make_unique<Float> ("current", "Current", Range (0.0f, 1.0f), 0.15f));
    layout.add (std::make_unique<Float> ("pump", "Air Pump", Range (0.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<Float> ("bubbleSize", "Bubble Size", Range (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<Float> ("dream", "Dream Physics", Range (0.0f, 1.0f), 0.35f));
    layout.add (std::make_unique<Float> ("mix", "Mix", Range (0.0f, 1.0f), 0.65f));
    layout.add (std::make_unique<Float> ("inputTrim", "Input", Range (-18.0f, 6.0f, 0.1f), -3.0f, "dB"));
    layout.add (std::make_unique<Float> ("outputTrim", "Output", Range (-18.0f, 6.0f, 0.1f), 0.0f, "dB"));
    // New parameters are appended to keep host automation order stable.
    layout.add (std::make_unique<Float> ("bubbleRate", "Bubble Rate", Range (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<Float> ("airStone", "Air Stone Position", Range (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<Float> ("distanceReverb", "Distance Reverb", Range (0.0f, 1.0f), 0.55f));
    layout.add (std::make_unique<juce::AudioParameterBool> ("pauseEnvironment", "Pause Environment", false));
    layout.add (std::make_unique<Float> ("waveVolume", "Wave Volume", Range (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<Float> ("sourceX", "Speaker Horizontal Position", Range (0.0f, 1.0f), 0.24f));
    layout.add (std::make_unique<Float> ("hydroX", "Hydrophone Horizontal Position", Range (0.0f, 1.0f), 0.76f));
    layout.add (std::make_unique<Float> ("whales", "Whale Song", Range (0.0f, 0.75f), 0.0f));
    layout.add (std::make_unique<Float> ("whaleVolume", "Whale Volume", Range (0.0f, 0.5f), 0.375f));
    layout.add (std::make_unique<Float> ("rain", "Rain Intensity", Range (0.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<Float> ("rainVolume", "Rain Volume", Range (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<Float> ("pirateFeedback", "Pirate Feedback", Range (0.0f, 0.9f), 0.0f));
    return layout;
}

void BeneathAudioProcessor::prepareToPlay (double sampleRate, int maximumBlockSize)
{
    engine.prepare (sampleRate, maximumBlockSize);
    dry.setSize (2, maximumBlockSize);
    wet.setSize (2, maximumBlockSize);
    inputGain.reset (sampleRate, 0.035);
    outputGain.reset (sampleRate, 0.035);
    inputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (
        parameters.getRawParameterValue ("inputTrim")->load()));
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (
        parameters.getRawParameterValue ("outputTrim")->load()));
}

void BeneathAudioProcessor::releaseResources() { engine.reset(); }

bool BeneathAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void BeneathAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto samples = buffer.getNumSamples();
    dry.setSize (2, samples, false, false, true);
    dry.copyFrom (0, 0, buffer, 0, 0, samples);
    dry.copyFrom (1, 0, buffer, buffer.getNumChannels() > 1 ? 1 : 0, 0, samples);

    const auto value = [this] (const char* id) { return parameters.getRawParameterValue (id)->load(); };
    inputGain.setTargetValue (juce::Decibels::decibelsToGain (value ("inputTrim")));
    for (int sample = 0; sample < samples; ++sample)
    {
        const auto gain = inputGain.getNextValue();
        dry.setSample (0, sample, dry.getSample (0, sample) * gain);
        dry.setSample (1, sample, dry.getSample (1, sample) * gain);
    }

    BeneathEngine::Parameters p;
    p.sourceDepth = value ("sourceDepth");
    p.hydrophoneDepth = value ("hydroDepth");
    p.sourceX = value ("sourceX");
    p.hydrophoneX = value ("hydroX");
    p.distance = value ("distance");
    p.waterDepth = value ("waterDepth");
    p.shore = value ("shore");
    p.seabed = static_cast<BeneathEngine::Seabed> (juce::jlimit (0, 4,
        static_cast<int> (value ("seabed"))));
    p.waves = value ("waves");
    // The legacy parameter remains in the state layout for session and automation
    // compatibility, but the accepted instrument balance is now fixed internally.
    p.waveVolume = 0.45f;
    p.current = value ("current");
    p.pump = value ("pump");
    p.bubbleRate = value ("bubbleRate");
    p.bubbleSize = value ("bubbleSize");
    p.airStonePosition = value ("airStone");
    p.distanceReverb = value ("distanceReverb");
    p.whales = value ("whales");
    p.whaleVolume = value ("whaleVolume");
    p.rain = value ("rain");
    p.rainVolume = value ("rainVolume");
    p.dream = value ("dream");
    p.pirateFeedback = value ("pirateFeedback");
    p.pauseEnvironment = value ("pauseEnvironment") > 0.5f;
    engine.process (dry, wet, p);

    const auto mix = value ("mix");
    const auto dryLevel = std::cos (mix * juce::MathConstants<float>::halfPi);
    const auto wetLevel = std::sin (mix * juce::MathConstants<float>::halfPi);
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (value ("outputTrim")));
    for (int sample = 0; sample < samples; ++sample)
    {
        const auto gain = outputGain.getNextValue();
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto x = dry.getSample (channel, sample) * dryLevel
                         + wet.getSample (channel, sample) * wetLevel;
            buffer.setSample (channel, sample, std::tanh (x * gain));
        }
    }
}

juce::AudioProcessorEditor* BeneathAudioProcessor::createEditor()
{
    return new BeneathAudioProcessorEditor (*this);
}

void BeneathAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary (*xml, destination);
}

void BeneathAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size); xml != nullptr)
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BeneathAudioProcessor(); }

