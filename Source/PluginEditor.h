#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class RubberButton final : public juce::Button
{
public:
    explicit RubberButton (const juce::String& label) : juce::Button (label) {}
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
    void setPanelSelected (bool shouldBeSelected)
    {
        if (panelSelected != shouldBeSelected)
        {
            panelSelected = shouldBeSelected;
            repaint();
        }
    }

private:
    bool panelSelected = false;
};

class RubberPillButton final : public juce::Button
{
public:
    explicit RubberPillButton (const juce::String& label) : juce::Button (label) {}
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
};

class RubberKnobLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
};

class BeneathAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit BeneathAudioProcessorEditor (BeneathAudioProcessor&);
    ~BeneathAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    enum class DragTarget { none, speaker, hydrophone, airStone, mast };
    enum class EnvironmentPanel { waves, rain, marineLife, bubbles };

    void timerCallback() override;
    void configureKnob (juce::Slider&, juce::Label&, const juce::String&);
    void drawAquarium (juce::Graphics&);
    void drawLcdReadouts (juce::Graphics&, juce::Rectangle<float>) const;
    void selectEnvironment (EnvironmentPanel, const char* parameterId, float fallbackValue);
    void updateControlVisibility();
    void syncEnvironmentButtons();
    [[nodiscard]] float parameter (const char*) const;
    [[nodiscard]] float normalizedParameter (const char*) const;
    [[nodiscard]] juce::Point<float> restingSpeakerPoint() const;
    [[nodiscard]] juce::Point<float> restingHydrophonePoint() const;
    [[nodiscard]] juce::Point<float> speakerPoint() const;
    [[nodiscard]] juce::Point<float> hydrophonePoint() const;
    [[nodiscard]] juce::Point<float> airStonePoint() const;
    [[nodiscard]] juce::Rectangle<float> mastControlArea() const;
    [[nodiscard]] float bottomYAt (float x) const;
    void beginGesture (const char* id);
    void setParameter (const char* id, float normalizedValue);
    void endGesture (const char* id);

    BeneathAudioProcessor& processor;
    juce::Image oceanBackdrop;
    juce::Image titleLogo;
    juce::Rectangle<float> aquarium;
    DragTarget dragTarget = DragTarget::none;
    bool updatingDistanceFromScene = false;
    bool advancedOpen = false;
    bool environmentPanelOpen = false;
    EnvironmentPanel selectedEnvironment = EnvironmentPanel::waves;
    std::array<float, 4> rememberedLayerValues { 0.25f, 0.45f, 0.50f, 0.50f };
    float animation = 0.0f;
    bool mastHovered = false;
    float mastDragStartY = 0.0f;
    float mastDragStartFeedback = 0.0f;

    juce::Slider distance, waterDepth, shore, waves, current, dream;
    juce::Slider pump, bubbleRate, whales, whaleVolume, distanceReverb, mix;
    juce::Slider rain, rainVolume;
    juce::Slider inputTrim, outputTrim;
    std::array<juce::Label, 14> labels;
    std::array<juce::Label, 2> trimLabels;
    juce::ComboBox seabed;
    juce::Label seabedLabel;
    juce::ToggleButton pause { "PAUSE ENVIRONMENT" };
    RubberButton wavesButton { "WAVES" };
    RubberButton rainButton { "RAIN" };
    RubberButton marineButton { "MARINE" };
    RubberButton bubblesButton { "BUBBLES" };
    RubberPillButton settingsButton { "SETTINGS" };
    RubberKnobLookAndFeel knobLookAndFeel;

    std::unique_ptr<SliderAttachment> distanceAttachment, waterDepthAttachment, shoreAttachment;
    std::unique_ptr<SliderAttachment> wavesAttachment, currentAttachment, dreamAttachment;
    std::unique_ptr<SliderAttachment> pumpAttachment, bubbleRateAttachment;
    std::unique_ptr<SliderAttachment> whalesAttachment, whaleVolumeAttachment;
    std::unique_ptr<SliderAttachment> rainAttachment, rainVolumeAttachment;
    std::unique_ptr<SliderAttachment> reverbAttachment, mixAttachment;
    std::unique_ptr<SliderAttachment> inputAttachment, outputAttachment;
    std::unique_ptr<ComboAttachment> seabedAttachment;
    std::unique_ptr<ButtonAttachment> pauseAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeneathAudioProcessorEditor)
};

