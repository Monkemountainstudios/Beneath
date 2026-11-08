#pragma once

#include <JuceHeader.h>
#include "BeneathEngine.h"

class BeneathAudioProcessor final : public juce::AudioProcessor
{
public:
    BeneathAudioProcessor();
    void prepareToPlay (double, int maximumBlockSize) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    [[nodiscard]] BeneathEngine::Motion getMotion() const noexcept { return engine.getMotion(); }

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    BeneathEngine engine;
    juce::AudioBuffer<float> dry, wet;
    juce::SmoothedValue<float> inputGain, outputGain;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeneathAudioProcessor)
};

