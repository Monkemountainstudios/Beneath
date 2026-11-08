#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
const juce::Colour casing { 0xffb9aa83 };
const juce::Colour casingLight { 0xffe0d3ad };
const juce::Colour casingShadow { 0xff71654c };
const juce::Colour bezel { 0xff5d4938 };
const juce::Colour ink { 0xff172722 };
const juce::Colour waterTop { 0xff86aca1 };
const juce::Colour waterBottom { 0xff315b5c };
const juce::Colour printedGreen { 0xff49634b };
const juce::Colour printedBrown { 0xff765a3f };

void drawSegmentDigit (juce::Graphics& g, int digit, float x, float y, float scale,
                       juce::Colour colour)
{
    static constexpr std::array<unsigned char, 10> masks
    {
        0b1110111, 0b0100100, 0b1011101, 0b1101101, 0b0101110,
        0b1101011, 0b1111011, 0b0100101, 0b1111111, 0b1101111
    };
    digit = juce::jlimit (0, 9, digit);
    const auto mask = masks[static_cast<size_t> (digit)];
    const auto t = 1.35f * scale;
    const auto w = 5.0f * scale;
    const auto h = 6.0f * scale;
    const auto horizontal = [&] (float px, float py, int bit)
    {
        if ((mask & (1 << bit)) != 0) g.fillRect (px, py, w, t);
    };
    const auto vertical = [&] (float px, float py, int bit)
    {
        if ((mask & (1 << bit)) != 0) g.fillRect (px, py, t, h);
    };
    g.setColour (colour);
    horizontal (x + t, y, 0);
    vertical (x, y + t, 1);
    vertical (x + w, y + t, 2);
    horizontal (x + t, y + h + t, 3);
    vertical (x, y + h + 2.0f * t, 4);
    vertical (x + w, y + h + 2.0f * t, 5);
    horizontal (x + t, y + 2.0f * h + 2.0f * t, 6);
}

void fillAngularSegment (juce::Graphics& g, juce::Point<float> start,
                         juce::Point<float> end, float thickness,
                         juce::Colour colour)
{
    const auto delta = end - start;
    const auto length = delta.getDistanceFromOrigin();
    if (length < 1.0f) return;
    const auto direction = delta / length;
    const juce::Point<float> normal { -direction.y, direction.x };
    const auto half = thickness * 0.5f;
    const auto bevel = juce::jmin (2.5f, length * 0.24f);
    juce::Path cell;
    cell.startNewSubPath (start);
    cell.lineTo (start + direction * bevel - normal * half);
    cell.lineTo (end - direction * bevel - normal * half);
    cell.lineTo (end);
    cell.lineTo (end - direction * bevel + normal * half);
    cell.lineTo (start + direction * bevel + normal * half);
    cell.closeSubPath();
    g.setColour (colour);
    g.fillPath (cell);
}
}

void RubberButton::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    auto bounds = getLocalBounds().toFloat();
    auto labelArea = bounds.removeFromBottom (18.0f);
    const auto diameter = juce::jmin (44.0f, bounds.getHeight() - 3.0f);
    auto buttonArea = juce::Rectangle<float> (diameter, diameter)
                          .withCentre (bounds.getCentre().translated (0.0f, 6.0f));

    if (panelSelected)
    {
        g.setColour (juce::Colour (0x55fff0b0));
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 9.0f);
    }

    if (getToggleState())
    {
        g.setColour (juce::Colour (0x24ff3f32));
        g.fillEllipse (buttonArea.expanded (12.0f));
        g.setColour (juce::Colour (0x55ff4b3b));
        g.drawEllipse (buttonArea.expanded (8.0f), 3.0f);
    }

    g.setColour (juce::Colour (0xff201b17));
    g.fillEllipse (buttonArea.expanded (4.0f));
    g.setColour (juce::Colour (0xff6d5742));
    g.drawEllipse (buttonArea.expanded (5.5f), 1.2f);

    const auto pressed = isButtonDown ? 2.0f : 0.0f;
    buttonArea = buttonArea.translated (0.0f, pressed);
    auto top = juce::Colour (getToggleState() ? 0xffff6351 : 0xffc94739);
    auto bottom = juce::Colour (getToggleState() ? 0xff8f1d20 : 0xff68191a);
    if (isMouseOver) top = top.brighter (0.10f);
    juce::ColourGradient rubber (top, buttonArea.getCentreX(), buttonArea.getY(),
                                 bottom, buttonArea.getCentreX(), buttonArea.getBottom(), false);
    rubber.addColour (0.42, juce::Colour (0xffbd302b));
    g.setGradientFill (rubber);
    g.fillEllipse (buttonArea);
    g.setColour (juce::Colour (0x66fff3e4));
    g.drawEllipse (buttonArea.reduced (3.0f).translated (-1.0f, -1.0f), 1.3f);
    g.setColour (juce::Colour (0x550d0807));
    g.drawEllipse (buttonArea, 1.5f);

    g.setColour (juce::Colour (0xff342c22));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText (getButtonText(), labelArea.toNearestInt(), juce::Justification::centred);
}

void RubberPillButton::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);
    if (isButtonDown) area.translate (0.0f, 1.5f);
    g.setColour (juce::Colour (0xff171311));
    g.fillRoundedRectangle (area, area.getHeight() * 0.5f);
    g.setColour (juce::Colour (0xff6d5742));
    g.drawRoundedRectangle (area.expanded (1.0f), area.getHeight() * 0.5f, 1.2f);
    auto rubber = area.reduced (4.0f, 4.0f);
    auto top = juce::Colour (getToggleState() ? 0xffff6754 : 0xffd84739);
    if (isMouseOver) top = top.brighter (0.08f);
    juce::ColourGradient gradient (top, rubber.getCentreX(), rubber.getY(),
                                   juce::Colour (0xff78191c), rubber.getCentreX(),
                                   rubber.getBottom(), false);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (rubber, rubber.getHeight() * 0.5f);
    g.setColour (juce::Colour (0x66fff2e0));
    g.drawRoundedRectangle (rubber.reduced (1.5f).translated (0.0f, -1.0f),
                            rubber.getHeight() * 0.5f, 1.0f);
    g.setColour (juce::Colour (0xfff1dfb4));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText (getButtonText(), area.toNearestInt(), juce::Justification::centred);
}

void RubberKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPos, float rotaryStartAngle,
                                               float rotaryEndAngle, juce::Slider& slider)
{
    const auto diameter = juce::jmin (static_cast<float> (width), static_cast<float> (height)) - 18.0f;
    auto area = juce::Rectangle<float> (diameter, diameter)
                    .withCentre ({ static_cast<float> (x) + width * 0.5f,
                                   static_cast<float> (y) + height * 0.5f + 2.0f });
    const auto border = slider.findColour (juce::Slider::rotarySliderOutlineColourId);
    g.setColour (juce::Colour (0xff171311));
    g.fillEllipse (area.expanded (4.0f));
    g.setColour (border);
    g.drawEllipse (area.expanded (5.5f), 2.2f);

    juce::ColourGradient rubber (juce::Colour (0xffff604d), area.getCentreX(), area.getY(),
                                 juce::Colour (0xff77191b), area.getCentreX(), area.getBottom(), false);
    rubber.addColour (0.46, juce::Colour (0xffc8322c));
    g.setGradientFill (rubber);
    g.fillEllipse (area);
    g.setColour (juce::Colour (0x55fff5e4));
    g.drawEllipse (area.reduced (3.0f).translated (-1.0f, -1.0f), 1.2f);

    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto radius = area.getWidth() * 0.32f;
    const juce::Point<float> dot { area.getCentreX() + std::sin (angle) * radius,
                                   area.getCentreY() - std::cos (angle) * radius };
    g.setColour (juce::Colour (0xff171311));
    g.fillEllipse (dot.x - 3.1f, dot.y - 3.1f, 6.2f, 6.2f);
}

void RubberKnobLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPos, float, float,
                                               juce::Slider::SliderStyle style,
                                               juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f,
                                                style, slider);
        return;
    }
    auto track = juce::Rectangle<float> (24.0f, static_cast<float> (height) - 8.0f)
                     .withCentre ({ static_cast<float> (x) + width * 0.5f,
                                    static_cast<float> (y) + height * 0.5f });
    g.setColour (juce::Colour (0xff171311));
    g.fillRoundedRectangle (track.expanded (4.0f), 8.0f);
    g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
    g.drawRoundedRectangle (track.expanded (5.0f), 9.0f, 2.0f);
    auto well = track.reduced (4.0f);
    g.setColour (juce::Colour (0xff4e4b42));
    g.fillRoundedRectangle (well, 4.0f);
    const auto fillTop = juce::jlimit (well.getY(), well.getBottom(), sliderPos);
    auto fill = juce::Rectangle<float> (well.getX(), fillTop, well.getWidth(), well.getBottom() - fillTop);
    juce::ColourGradient rubber (juce::Colour (0xffff604d), fill.getCentreX(), fill.getY(),
                                 juce::Colour (0xff76181a), fill.getCentreX(), well.getBottom(), false);
    g.setGradientFill (rubber);
    g.fillRoundedRectangle (fill, 4.0f);
    g.setColour (juce::Colour (0xff171311));
    g.fillRect (well.getX() - 2.0f, fillTop - 1.2f, well.getWidth() + 4.0f, 2.4f);
}

BeneathAudioProcessorEditor::BeneathAudioProcessorEditor (BeneathAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    oceanBackdrop = juce::ImageFileFormat::loadFrom (BinaryData::ocean_backdrop_png,
                                                     BinaryData::ocean_backdrop_pngSize);
    titleLogo = juce::ImageFileFormat::loadFrom (BinaryData::beneath_logo_png,
                                                 BinaryData::beneath_logo_pngSize);
    setOpaque (true);
    setSize (900, 700);

    std::array<juce::Slider*, 14> knobs { &distance, &waterDepth, &shore, &waves, &current,
        &dream, &pump, &bubbleRate, &whales, &whaleVolume, &distanceReverb, &mix,
        &rain, &rainVolume };
    const std::array<juce::String, 14> names { "DISTANCE", "WATER DEPTH", "RISING SEABED",
        "WAVE INTENSITY", "CURRENT", "DREAM PHYSICS", "PUMP PRESSURE", "BUBBLE RATE",
        "WHALE SONG", "WHALE VOLUME", "DISTANCE REVERB", "MIX",
        "RAIN INTENSITY", "RAIN VOLUME" };
    for (size_t i = 0; i < knobs.size(); ++i)
        configureKnob (*knobs[i], labels[i], names[i]);
    configureKnob (inputTrim, trimLabels[0], "INPUT");
    configureKnob (outputTrim, trimLabels[1], "OUTPUT");
    inputTrim.setSliderStyle (juce::Slider::LinearVertical);
    outputTrim.setSliderStyle (juce::Slider::LinearVertical);

    for (auto* knob : { &waves, &rain, &rainVolume, &whales, &whaleVolume, &pump, &bubbleRate })
        knob->setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff7e4934));
    for (auto* knob : { &waterDepth, &shore, &current, &dream, &distanceReverb })
        knob->setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff426c68));
    for (auto* knob : { &inputTrim, &outputTrim, &mix })
        knob->setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff312d27));

    distanceAttachment = std::make_unique<SliderAttachment> (p.parameters, "distance", distance);
    waterDepthAttachment = std::make_unique<SliderAttachment> (p.parameters, "waterDepth", waterDepth);
    shoreAttachment = std::make_unique<SliderAttachment> (p.parameters, "shore", shore);
    wavesAttachment = std::make_unique<SliderAttachment> (p.parameters, "waves", waves);
    currentAttachment = std::make_unique<SliderAttachment> (p.parameters, "current", current);
    dreamAttachment = std::make_unique<SliderAttachment> (p.parameters, "dream", dream);
    pumpAttachment = std::make_unique<SliderAttachment> (p.parameters, "pump", pump);
    bubbleRateAttachment = std::make_unique<SliderAttachment> (p.parameters, "bubbleRate", bubbleRate);
    whalesAttachment = std::make_unique<SliderAttachment> (p.parameters, "whales", whales);
    whaleVolumeAttachment = std::make_unique<SliderAttachment> (p.parameters, "whaleVolume", whaleVolume);
    rainAttachment = std::make_unique<SliderAttachment> (p.parameters, "rain", rain);
    rainVolumeAttachment = std::make_unique<SliderAttachment> (p.parameters, "rainVolume", rainVolume);
    reverbAttachment = std::make_unique<SliderAttachment> (p.parameters, "distanceReverb", distanceReverb);
    mixAttachment = std::make_unique<SliderAttachment> (p.parameters, "mix", mix);
    inputAttachment = std::make_unique<SliderAttachment> (p.parameters, "inputTrim", inputTrim);
    outputAttachment = std::make_unique<SliderAttachment> (p.parameters, "outputTrim", outputTrim);

    // Attachments install the parameter ranges, so display precision must be applied afterwards.
    std::array<juce::Slider*, 16> allKnobs { &distance, &waterDepth, &shore, &waves, &current,
        &dream, &pump, &bubbleRate, &whales, &whaleVolume, &distanceReverb, &mix,
        &rain, &rainVolume, &inputTrim, &outputTrim };
    for (auto* knob : allKnobs)
    {
        knob->setNumDecimalPlacesToDisplay (2);
        knob->textFromValueFunction = [] (double value)
        {
            return juce::String (value, 2);
        };
        knob->valueFromTextFunction = [] (const juce::String& text)
        {
            return text.getDoubleValue();
        };
        knob->updateText();
    }

    distance.onValueChange = [this]
    {
        if (updatingDistanceFromScene) return;
        const auto separation = static_cast<float> (distance.getValue() * distance.getValue());
        const auto source = parameter ("sourceX");
        const auto hydro = parameter ("hydroX");
        const auto forwards = hydro >= source;
        auto midpoint = 0.5f * (source + hydro);
        midpoint = juce::jlimit (0.5f * separation, 1.0f - 0.5f * separation, midpoint);
        const auto left = midpoint - 0.5f * separation;
        const auto right = midpoint + 0.5f * separation;
        setParameter ("sourceX", forwards ? left : right);
        setParameter ("hydroX", forwards ? right : left);
    };

    seabed.addItemList ({ "MUD", "SAND", "GRAVEL", "ROCK", "CORAL" }, 1);
    seabed.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (seabed);
    seabedLabel.setText ("SEABED", juce::dontSendNotification);
    seabedLabel.setJustificationType (juce::Justification::centred);
    seabedLabel.setColour (juce::Label::textColourId, juce::Colour (0xff342c22));
    addAndMakeVisible (seabedLabel);
    seabedAttachment = std::make_unique<ComboAttachment> (p.parameters, "seabed", seabed);

    pause.setColour (juce::ToggleButton::textColourId, juce::Colour (0xff342c22));
    addAndMakeVisible (pause);
    pauseAttachment = std::make_unique<ButtonAttachment> (p.parameters, "pauseEnvironment", pause);

    for (auto* button : { &wavesButton, &rainButton, &marineButton, &bubblesButton })
        addAndMakeVisible (*button);
    wavesButton.onClick = [this] { selectEnvironment (EnvironmentPanel::waves, "waves", 0.25f); };
    rainButton.onClick = [this] { selectEnvironment (EnvironmentPanel::rain, "rain", 0.45f); };
    marineButton.onClick = [this] { selectEnvironment (EnvironmentPanel::marineLife, "whales", 0.50f); };
    bubblesButton.onClick = [this] { selectEnvironment (EnvironmentPanel::bubbles, "pump", 0.45f); };

    settingsButton.setClickingTogglesState (true);
    settingsButton.onClick = [this]
    {
        advancedOpen = settingsButton.getToggleState();
        if (advancedOpen) environmentPanelOpen = false;
        updateControlVisibility();
    };
    addAndMakeVisible (settingsButton);

    syncEnvironmentButtons();
    updateControlVisibility();

    startTimerHz (30);
}

BeneathAudioProcessorEditor::~BeneathAudioProcessorEditor()
{
    for (auto* slider : { &distance, &waterDepth, &shore, &waves, &current, &dream,
                          &pump, &bubbleRate, &whales, &whaleVolume, &distanceReverb,
                          &mix, &rain, &rainVolume, &inputTrim, &outputTrim })
        slider->setLookAndFeel (nullptr);
}

void BeneathAudioProcessorEditor::configureKnob (juce::Slider& slider, juce::Label& label,
                                                  const juce::String& name)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setLookAndFeel (&knobLookAndFeel);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
    slider.setNumDecimalPlacesToDisplay (2);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff426c68));
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff5f543f));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff241f19));
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xff2e281f));
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xffc8ba91));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff6a5b43));
    addAndMakeVisible (slider);
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colour (0xff342c22));
    label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    addAndMakeVisible (label);
}

float BeneathAudioProcessorEditor::normalizedParameter (const char* id) const
{
    if (auto* p = processor.parameters.getParameter (id))
        return p->getValue();
    return 0.0f;
}

void BeneathAudioProcessorEditor::selectEnvironment (EnvironmentPanel panel,
                                                       const char* parameterId,
                                                       float fallbackValue)
{
    selectedEnvironment = panel;
    advancedOpen = false;
    environmentPanelOpen = true;
    settingsButton.setToggleState (false, juce::dontSendNotification);
    const auto index = static_cast<size_t> (panel);
    if (auto* p = processor.parameters.getParameter (parameterId))
    {
        const auto currentValue = p->getValue();
        const auto target = currentValue > 0.0001f
                          ? 0.0f
                          : juce::jmax (fallbackValue, rememberedLayerValues[index]);
        if (currentValue > 0.0001f)
            rememberedLayerValues[index] = currentValue;
        p->beginChangeGesture();
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, target));
        p->endChangeGesture();
    }
    syncEnvironmentButtons();
    updateControlVisibility();
}

void BeneathAudioProcessorEditor::syncEnvironmentButtons()
{
    wavesButton.setToggleState (parameter ("waves") > 0.0001f, juce::dontSendNotification);
    rainButton.setToggleState (parameter ("rain") > 0.0001f, juce::dontSendNotification);
    marineButton.setToggleState (parameter ("whales") > 0.0001f, juce::dontSendNotification);
    bubblesButton.setToggleState (parameter ("pump") > 0.0001f, juce::dontSendNotification);
    wavesButton.setPanelSelected (environmentPanelOpen && ! advancedOpen
                                  && selectedEnvironment == EnvironmentPanel::waves);
    rainButton.setPanelSelected (environmentPanelOpen && ! advancedOpen
                                 && selectedEnvironment == EnvironmentPanel::rain);
    marineButton.setPanelSelected (environmentPanelOpen && ! advancedOpen
                                   && selectedEnvironment == EnvironmentPanel::marineLife);
    bubblesButton.setPanelSelected (environmentPanelOpen && ! advancedOpen
                                    && selectedEnvironment == EnvironmentPanel::bubbles);
}

void BeneathAudioProcessorEditor::updateControlVisibility()
{
    std::array<juce::Slider*, 14> knobs { &distance, &waterDepth, &shore, &waves, &current,
        &dream, &pump, &bubbleRate, &whales, &whaleVolume, &distanceReverb, &mix,
        &rain, &rainVolume };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        knobs[i]->setVisible (false);
        labels[i].setVisible (false);
    }
    seabed.setVisible (advancedOpen);
    seabedLabel.setVisible (advancedOpen);
    pause.setVisible (advancedOpen);

    const auto show = [this, &knobs] (size_t index)
    {
        knobs[index]->setVisible (true);
        labels[index].setVisible (true);
    };

    if (advancedOpen)
    {
        for (const auto index : { size_t (1), size_t (2), size_t (4), size_t (5), size_t (10) })
            show (index);
    }
    else if (environmentPanelOpen)
    {
        switch (selectedEnvironment)
        {
            case EnvironmentPanel::waves:      show (3); break;
            case EnvironmentPanel::rain:       show (12); show (13); break;
            case EnvironmentPanel::marineLife: show (8); show (9); break;
            case EnvironmentPanel::bubbles:    show (6); show (7); break;
        }
    }
    show (11); // Mix remains a front-panel control in every view.
    syncEnvironmentButtons();
    repaint();
}

void BeneathAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient caseGradient (casingLight, 0.0f, 0.0f,
                                       casingShadow, 0.0f, bounds.getBottom(), false);
    caseGradient.addColour (0.38, casing);
    caseGradient.addColour (0.76, juce::Colour (0xffa39470));
    g.setGradientFill (caseGradient);
    g.fillAll();

    g.setColour (juce::Colour (0x18ffffff));
    for (int y = 3; y < getHeight(); y += 5)
        g.drawHorizontalLine (y, 9.0f, static_cast<float> (getWidth() - 9));

    g.setColour (juce::Colour (0xff4f4636));
    g.drawRoundedRectangle (bounds.reduced (7.0f), 20.0f, 2.0f);
    g.setColour (juce::Colour (0x66fff4cf));
    g.drawRoundedRectangle (bounds.reduced (10.0f), 17.0f, 1.0f);

    g.setColour (juce::Colour (0xff342c22));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText ("SUB-AQUATIC SIGNAL SYSTEM", 665, 8, 200, 20,
                juce::Justification::centredRight);

    for (const auto centre : { juce::Point<float> { 21.0f, 21.0f },
                               juce::Point<float> { bounds.getRight() - 21.0f, 21.0f },
                               juce::Point<float> { 21.0f, bounds.getBottom() - 21.0f },
                               juce::Point<float> { bounds.getRight() - 21.0f, bounds.getBottom() - 21.0f } })
    {
        g.setColour (juce::Colour (0xff655b47));
        g.fillEllipse (centre.x - 4.0f, centre.y - 4.0f, 8.0f, 8.0f);
        g.setColour (juce::Colour (0xffd8cba6));
        g.drawLine (centre.x - 2.0f, centre.y, centre.x + 2.0f, centre.y, 1.0f);
    }

    drawAquarium (g);

    if (titleLogo.isValid())
    {
        const juce::Rectangle<float> logoArea (45.0f, 490.0f, 580.0f, 193.0f);
        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (juce::degreesToRadians (-1.6f),
                                                         logoArea.getCentreX(),
                                                         logoArea.getCentreY()));
        g.setOpacity (0.82f);
        g.drawImage (titleLogo, logoArea, juce::RectanglePlacement::stretchToFit);
        g.restoreState();
    }
}

void BeneathAudioProcessorEditor::drawAquarium (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff392f27));
    g.fillRoundedRectangle (aquarium.expanded (13.0f), 14.0f);
    g.setColour (bezel);
    g.fillRoundedRectangle (aquarium.expanded (9.0f), 11.0f);
    g.setColour (juce::Colour (0xff211e19));
    g.fillRoundedRectangle (aquarium.expanded (3.0f), 7.0f);

    const auto screen = aquarium.reduced (7.0f);
    const auto waterline = screen.getY() + 64.0f;
    const juce::Rectangle<float> airArea (screen.getX(), screen.getY(), screen.getWidth(),
                                           waterline - screen.getY());
    const juce::Rectangle<float> underwaterArea (screen.getX(), waterline, screen.getWidth(),
                                                  screen.getBottom() - waterline);
    juce::ColourGradient printedSky (juce::Colour (0xffd5d7bd), screen.getCentreX(), screen.getY(),
                                     juce::Colour (0xffa9beb0), screen.getCentreX(), waterline, false);
    g.setGradientFill (printedSky);
    g.fillRect (airArea);

    if (oceanBackdrop.isValid())
        g.drawImage (oceanBackdrop, underwaterArea, juce::RectanglePlacement::stretchToFit);
    else
    {
        juce::ColourGradient fallback (waterTop, screen.getCentreX(), waterline,
                                       waterBottom, screen.getCentreX(), screen.getBottom(), false);
        g.setGradientFill (fallback);
        g.fillRect (underwaterArea);
    }

    // Printed plates on the old games were slightly muted beneath the LCD glass.
    juce::ColourGradient glassTint (juce::Colour (0x060f2825), screen.getCentreX(), waterline,
                                    juce::Colour (0x32202f2b), screen.getCentreX(),
                                    screen.getBottom(), false);
    g.setGradientFill (glassTint);
    g.fillRect (underwaterArea);

    const auto rainAmount = parameter ("rain");
    if (rainAmount > 0.001f)
    {
        const auto count = 2 + static_cast<int> (rainAmount * 38.0f);
        g.setColour (ink.withAlpha (0.22f + 0.36f * rainAmount));
        for (int i = 0; i < count; ++i)
        {
            const auto xSeed = std::fmod (static_cast<float> (i) * 0.6180339f + 0.17f, 1.0f);
            const auto fall = std::fmod (animation * (0.44f + 2.2f * rainAmount)
                                      + static_cast<float> (i) * 0.173f, 1.0f);
            const auto x = screen.getX() + xSeed * screen.getWidth();
            const auto y = screen.getY() + fall * (waterline - screen.getY());
            g.drawLine (x, y - 5.0f, x - 1.8f, y + 2.0f, 1.0f);
            if (fall > 0.91f)
            {
                const auto ring = (fall - 0.91f) / 0.09f;
                g.drawEllipse (x - 5.0f * ring, waterline - 1.5f * ring,
                               10.0f * ring, 3.0f * ring, 0.8f);
            }
        }
    }

    // Three stepped amplitudes built from bevel-ended LCD cells.
    const auto waveSetting = parameter ("waves");
    const auto waveAmount = waveSetting < 0.34f ? 2.0f : waveSetting < 0.68f ? 5.0f : 9.0f;
    const auto waveFrame = std::floor (animation * (1.0f + 3.0f * waveSetting)) * 0.72f;
    const auto waveY = [&] (float x)
    {
        return waterline + std::sin (waveFrame + x * 0.075f) * waveAmount;
    };
    for (float x = 0.0f; x < screen.getWidth(); x += 14.0f)
    {
        const auto nextX = juce::jmin (x + 13.0f, screen.getWidth());
        fillAngularSegment (g, { screen.getX() + x, waveY (x) },
                               { screen.getX() + nextX, waveY (nextX) },
                               3.8f, ink.withAlpha (0.92f));
    }

    // The rising seabed is an LCD contour: bold, uneven and mechanically movable.
    juce::Path floor;
    std::array<juce::Point<float>, 37> floorCells {};
    floor.startNewSubPath (screen.getX(), screen.getBottom());
    floor.lineTo (screen.getX(), bottomYAt (screen.getX()));
    for (int i = 0; i <= 36; ++i)
    {
        const auto x = screen.getX() + screen.getWidth() * static_cast<float> (i) / 36.0f;
        const auto bump = 3.0f * std::sin (static_cast<float> (i) * 1.71f)
                        + 1.7f * std::sin (static_cast<float> (i) * 0.63f);
        floorCells[static_cast<size_t> (i)] = { x, bottomYAt (x) - std::abs (bump) };
        floor.lineTo (floorCells[static_cast<size_t> (i)]);
    }
    floor.lineTo (screen.getRight(), screen.getBottom());
    floor.closeSubPath();
    const auto material = juce::jlimit (0, 4, static_cast<int> (parameter ("seabed")));
    const std::array<juce::Colour, 5> seabedColours
    {
        juce::Colour (0xff352a24), // mud
        juce::Colour (0xff6d5b36), // sand
        juce::Colour (0xff484b43), // gravel
        juce::Colour (0xff172722), // rock
        juce::Colour (0xff633d38)  // coral
    };
    const auto seabedColour = seabedColours[static_cast<size_t> (material)];
    g.setColour (seabedColour);
    g.fillPath (floor);
    for (size_t i = 0; i + 1 < floorCells.size(); ++i)
        fillAngularSegment (g, floorCells[i], floorCells[i + 1], 4.0f,
                            seabedColour.darker (0.72f));

    g.saveState();
    g.reduceClipRegion (floor);
    const auto textureColour = seabedColour.brighter (0.55f).withAlpha (0.50f);
    g.setColour (textureColour);
    const auto insideFloor = [&] (float x, float y)
    {
        return y > bottomYAt (x) + 6.0f && y < screen.getBottom() - 3.0f;
    };
    if (material == 0) // mud: slow, soft sediment grooves
    {
        for (int row = 0; row < 6; ++row)
            for (float x = screen.getX() + 16.0f + static_cast<float> (row % 2) * 23.0f;
                 x < screen.getRight() - 24.0f; x += 58.0f)
            {
                const auto y = bottomYAt (x) + 13.0f + static_cast<float> (row) * 17.0f;
                if (! insideFloor (x, y)) continue;
                juce::Path groove;
                groove.startNewSubPath (x, y);
                groove.cubicTo (x + 6.0f, y - 2.0f, x + 15.0f, y + 2.0f, x + 24.0f, y);
                g.strokePath (groove, juce::PathStrokeType (1.25f));
            }
    }
    else if (material == 1) // sand: short repeating ripple strokes
    {
        for (int row = 0; row < 7; ++row)
            for (float x = screen.getX() + 12.0f + static_cast<float> (row % 2) * 13.0f;
                 x < screen.getRight() - 12.0f; x += 27.0f)
            {
                const auto y = bottomYAt (x) + 11.0f + static_cast<float> (row) * 14.0f;
                if (insideFloor (x, y)) g.drawLine (x, y + 2.0f, x + 11.0f, y, 1.15f);
            }
    }
    else if (material == 2) // gravel: angular pebble cells
    {
        for (int row = 0; row < 6; ++row)
            for (float x = screen.getX() + 14.0f + static_cast<float> (row % 2) * 17.0f;
                 x < screen.getRight() - 10.0f; x += 35.0f)
            {
                const auto y = bottomYAt (x) + 13.0f + static_cast<float> (row) * 17.0f;
                if (! insideFloor (x, y)) continue;
                juce::Path stoneShape;
                stoneShape.startNewSubPath (x, y + 3.0f);
                stoneShape.lineTo (x + 5.0f, y);
                stoneShape.lineTo (x + 11.0f, y + 2.0f);
                stoneShape.lineTo (x + 8.0f, y + 7.0f);
                stoneShape.lineTo (x + 2.0f, y + 7.0f);
                stoneShape.closeSubPath();
                g.strokePath (stoneShape, juce::PathStrokeType (1.1f));
            }
    }
    else if (material == 3) // rock: broad fractured cells
    {
        for (float x = screen.getX() + 28.0f; x < screen.getRight() - 18.0f; x += 72.0f)
        {
            const auto y = bottomYAt (x) + 19.0f;
            if (! insideFloor (x, y)) continue;
            g.drawLine (x, y, x + 7.0f, y + 8.0f, 1.35f);
            g.drawLine (x + 7.0f, y + 8.0f, x + 2.0f, y + 16.0f, 1.35f);
            g.drawLine (x + 7.0f, y + 8.0f, x + 17.0f, y + 11.0f, 1.35f);
        }
    }
    else // coral: sparse branching marks
    {
        for (float x = screen.getX() + 24.0f; x < screen.getRight() - 16.0f; x += 64.0f)
        {
            const auto baseY = bottomYAt (x) + 38.0f;
            if (! insideFloor (x, baseY)) continue;
            g.drawLine (x, baseY, x, baseY - 22.0f, 1.5f);
            g.drawLine (x, baseY - 13.0f, x - 7.0f, baseY - 19.0f, 1.5f);
            g.drawLine (x, baseY - 9.0f, x + 8.0f, baseY - 17.0f, 1.5f);
            g.fillEllipse (x - 2.5f, baseY - 26.0f, 5.0f, 5.0f);
        }
    }
    g.restoreState();

    // Printed boat: fixed colour artwork rather than an active LCD cell.
    const auto shipX = screen.getX() + screen.getWidth() * 0.55f;
    g.setColour (juce::Colour (0xff81553c));
    juce::Path hull;
    hull.startNewSubPath (shipX - 58.0f, waterline - 17.0f);
    hull.lineTo (shipX + 58.0f, waterline - 17.0f);
    hull.lineTo (shipX + 39.0f, waterline + 3.0f);
    hull.lineTo (shipX - 41.0f, waterline + 3.0f);
    hull.closeSubPath();
    g.fillPath (hull);
    g.setColour (juce::Colour (0xffe0d5b2));
    g.fillRect (shipX - 18.0f, waterline - 39.0f, 36.0f, 22.0f);
    g.setColour (printedBrown);
    g.drawRect (shipX - 18.0f, waterline - 39.0f, 36.0f, 22.0f, 2.0f);
    const auto pirateFeedback = parameter ("pirateFeedback");
    const auto flagTop = waterline - 38.0f - pirateFeedback * 20.0f;
    g.drawLine (shipX, waterline - 39.0f, shipX, waterline - 58.0f,
                mastHovered ? 2.8f : 2.0f);
    g.setColour (juce::Colour (0xff486d67));
    g.fillRect (shipX - 12.0f, waterline - 34.0f, 9.0f, 7.0f);
    g.fillRect (shipX + 3.0f, waterline - 34.0f, 9.0f, 7.0f);

    if (pirateFeedback > 0.005f || mastHovered)
    {
        const auto reveal = juce::jmax (pirateFeedback, mastHovered ? 0.16f : 0.0f);
        const auto flagWidth = 25.0f + 8.0f * reveal;
        const auto flagHeight = 15.0f + 4.0f * reveal;
        juce::Path flag;
        flag.startNewSubPath (shipX + 1.0f, flagTop + 2.0f);
        flag.lineTo (shipX + flagWidth, flagTop + 4.0f);
        flag.lineTo (shipX + flagWidth - 5.0f, flagTop + flagHeight * 0.57f);
        flag.lineTo (shipX + flagWidth, flagTop + flagHeight);
        flag.lineTo (shipX + 1.0f, flagTop + flagHeight - 1.0f);
        flag.closeSubPath();
        g.setColour (juce::Colour (0xff171614).withAlpha (0.45f + 0.55f * reveal));
        g.fillPath (flag);
        g.setColour (juce::Colour (0xffe8dfc5).withAlpha (0.45f + 0.55f * reveal));
        const auto skull = juce::Point<float> { shipX + flagWidth * 0.52f,
                                                flagTop + flagHeight * 0.43f };
        g.fillEllipse (skull.x - 3.2f, skull.y - 3.0f, 6.4f, 5.8f);
        g.drawLine (skull.x - 5.0f, skull.y + 5.0f, skull.x + 5.0f, skull.y + 10.0f, 1.3f);
        g.drawLine (skull.x + 5.0f, skull.y + 5.0f, skull.x - 5.0f, skull.y + 10.0f, 1.3f);
        g.setColour (juce::Colour (0xff171614));
        g.fillEllipse (skull.x - 2.3f, skull.y - 1.0f, 1.5f, 1.5f);
        g.fillEllipse (skull.x + 0.8f, skull.y - 1.0f, 1.5f, 1.5f);

        if (mastHovered || dragTarget == DragTarget::mast)
        {
            g.setColour (ink.withAlpha (0.90f));
            g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
            g.drawText ("FEEDBACK " + juce::String (juce::roundToInt (pirateFeedback * 100.0f)) + "%",
                        static_cast<int> (shipX + 35.0f), static_cast<int> (flagTop), 90, 18,
                        juce::Justification::centredLeft);
        }
    }

    const auto speaker = speakerPoint();
    const auto hydro = hydrophonePoint();
    const auto restingSpeaker = restingSpeakerPoint();
    const auto restingHydro = restingHydrophonePoint();
    const auto stone = airStonePoint();
    const float cableDashes[] { 2.5f, 5.0f };
    const float activeCableDashes[] { 7.0f, 3.5f };
    g.setColour (ink.withAlpha (0.24f));
    g.drawDashedLine ({ shipX - 31.0f, waterline - 13.0f, restingSpeaker.x, restingSpeaker.y },
                      cableDashes, 2, 0.55f);
    g.drawDashedLine ({ shipX + 31.0f, waterline - 13.0f, restingHydro.x, restingHydro.y },
                      cableDashes, 2, 0.55f);
    g.drawEllipse (restingSpeaker.x - 4.0f, restingSpeaker.y - 4.0f, 8.0f, 8.0f, 1.0f);
    g.drawEllipse (restingHydro.x - 4.0f, restingHydro.y - 4.0f, 8.0f, 8.0f, 1.0f);
    g.setColour (ink.withAlpha (0.8f));
    g.drawDashedLine ({ shipX - 31.0f, waterline - 13.0f, speaker.x, speaker.y },
                      activeCableDashes, 2, 0.85f);
    g.drawDashedLine ({ shipX + 31.0f, waterline - 13.0f, hydro.x, hydro.y },
                      activeCableDashes, 2, 0.85f);

    g.setColour (ink.withAlpha (0.94f));
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.fillRoundedRectangle (speaker.x - 13.0f, speaker.y - 10.0f, 26.0f, 20.0f, 2.0f);
    g.setColour (waterTop.withAlpha (0.48f));
    g.fillEllipse (speaker.x - 7.0f, speaker.y - 7.0f, 14.0f, 14.0f);
    g.setColour (ink.withAlpha (0.94f));
    g.drawEllipse (speaker.x - 7.0f, speaker.y - 7.0f, 14.0f, 14.0f, 2.0f);
    const auto speakerLabelX = juce::jlimit (screen.getX(), screen.getRight() - 76.0f,
                                             speaker.x - 38.0f);
    g.setColour (ink.withAlpha (0.94f));
    g.fillEllipse (hydro.x - 8.0f, hydro.y - 12.0f, 16.0f, 24.0f);
    const auto hydroLabelX = juce::jlimit (screen.getX(), screen.getRight() - 100.0f,
                                           hydro.x - 50.0f);
    g.setColour (juce::Colour (0xff292d29));
    g.fillRoundedRectangle (stone.x - 15.0f, stone.y - 5.0f, 30.0f, 9.0f, 4.0f);
    g.setColour (ink);
    const auto stoneLabelX = juce::jlimit (screen.getX(), screen.getRight() - 84.0f,
                                           stone.x - 42.0f);
    g.drawText ("AIR STONE", static_cast<int> (stoneLabelX), static_cast<int> (stone.y + 5.0f),
                84, 15, juce::Justification::centred);

    const auto pumpAmount = parameter ("pump");
    if (pumpAmount > 0.001f)
    {
        g.setColour (juce::Colour (0xffd6e7d7).withAlpha (0.65f));
        const auto count = 3 + static_cast<int> (pumpAmount * 18.0f);
        for (int i = 0; i < count; ++i)
        {
            const auto phase = std::fmod (animation * (0.18f + 0.015f * i) + i * 0.137f, 1.0f);
            const auto y = stone.y - phase * (stone.y - waterline);
            const auto x = stone.x + std::sin (animation + i * 2.1f) * (3.0f + i % 4);
            const auto radius = 1.8f + static_cast<float> (i % 4) * 0.85f;
            g.drawEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f, 1.2f);
        }
    }

    // Tall printed plants frame the left edge, with inkier colour near the seabed.
    const auto currentAmount = parameter ("current");
    for (int i = 0; i < 7; ++i)
    {
        const auto x = screen.getX() + 22.0f + static_cast<float> (i) * 20.0f;
        const auto bottom = bottomYAt (x);
        const auto availableHeight = bottom - waterline;
        const auto height = availableHeight * (0.73f + 0.035f * static_cast<float> ((i * 5) % 6));
        const auto swayCell = std::round (std::sin (animation * 0.72f + static_cast<float> (i) * 0.43f)
                                         * 2.0f) * 0.5f;
        const auto sway = swayCell * currentAmount * (15.0f + static_cast<float> (i % 3) * 2.0f);
        juce::Path plant;
        plant.startNewSubPath (x, bottom);
        const auto crownX = x + static_cast<float> ((i % 3) - 1) * 5.0f + sway;
        plant.cubicTo (x - 12.0f + sway * 0.22f, bottom - height * 0.35f,
                       x + 14.0f + sway * 0.66f, bottom - height * 0.72f, crownX, bottom - height);
        const auto topColour = i % 3 == 0 ? juce::Colour (0xff98744d)
                                         : juce::Colour (0xff66835b);
        const auto bottomColour = i % 3 == 0 ? juce::Colour (0xff382b22)
                                            : juce::Colour (0xff203d35);
        juce::ColourGradient plantGradient (topColour, x, bottom - height,
                                            bottomColour, x, bottom, false);
        g.setGradientFill (plantGradient);
        g.strokePath (plant, juce::PathStrokeType (3.4f));
        for (int leaf = 1; leaf <= 6; ++leaf)
        {
            const auto heightFraction = 0.135f * static_cast<float> (leaf);
            const auto ly = bottom - height * heightFraction;
            const auto side = ((i + leaf) % 2 == 0) ? -1.0f : 1.0f;
            const auto leafWidth = 12.0f + static_cast<float> (leaf % 2) * 3.0f;
            g.fillEllipse (x + sway * heightFraction + side * 4.0f - leafWidth * 0.5f, ly - 3.5f,
                           leafWidth, 7.0f);
        }
    }

    const auto drawEquipmentCaption = [&] (const juce::String& text, float x, float y, float width)
    {
        const juce::Rectangle<float> area (x, y, width, 16.0f);
        g.setColour (ink.withAlpha (0.46f));
        g.fillRoundedRectangle (area.reduced (1.0f, 1.5f), 3.0f);
        g.setColour (juce::Colour (0xffeef4dc).withAlpha (0.96f));
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (text, area.toNearestInt(), juce::Justification::centred);
    };
    drawEquipmentCaption ("SPEAKER", speakerLabelX, speaker.y - 28.0f, 76.0f);
    drawEquipmentCaption ("HYDROPHONE", hydroLabelX, hydro.y - 30.0f, 100.0f);

    drawLcdReadouts (g, screen);
}

void BeneathAudioProcessorEditor::drawLcdReadouts (juce::Graphics& g,
                                                    juce::Rectangle<float> screen) const
{
    const auto y = screen.getY() + 8.0f;
    const auto colour = ink.withAlpha (0.86f);
    const std::array<std::pair<const char*, float>, 3> readouts
    {{
        { "DIST", parameter ("distance") },
        { "SPK",  parameter ("sourceDepth") },
        { "HYD",  parameter ("hydroDepth") }
    }};
    auto x = screen.getX() + 10.0f;
    for (const auto& [label, normalized] : readouts)
    {
        g.setColour (colour);
        g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
        g.drawText (label, static_cast<int> (x), static_cast<int> (y + 4.0f), 24, 13,
                    juce::Justification::centredLeft);
        const auto value = juce::jlimit (0, 99, juce::roundToInt (normalized * 99.0f));
        drawSegmentDigit (g, value / 10, x + 27.0f, y - 1.0f, 1.0f, colour);
        drawSegmentDigit (g, value % 10, x + 39.0f, y - 1.0f, 1.0f, colour);
        x += 76.0f;
    }
}

void BeneathAudioProcessorEditor::resized()
{
    aquarium = juce::Rectangle<float> (112.0f, 54.0f, static_cast<float> (getWidth() - 224), 375.0f);
    std::array<juce::Slider*, 14> knobs { &distance, &waterDepth, &shore, &waves, &current,
        &dream, &pump, &bubbleRate, &whales, &whaleVolume, &distanceReverb, &mix,
        &rain, &rainVolume };
    const auto place = [this, &knobs] (size_t index, int x, int y)
    {
        knobs[index]->setBounds (x, y + 18, 92, 88);
        labels[index].setBounds (x - 5, y, 102, 22);
    };

    wavesButton.setBounds (20, 207, 78, 76);
    rainButton.setBounds (20, 349, 78, 76);
    marineButton.setBounds (802, 207, 78, 76);
    bubblesButton.setBounds (802, 349, 78, 76);

    // Context row: only the selected environment's controls are visible.
    place (3, 404, 474);
    place (12, 350, 474); place (13, 458, 474);
    place (8, 350, 474);  place (9, 458, 474);
    place (6, 350, 474);  place (7, 458, 474);

    // Settings row replaces the context row when the drawer is opened.
    place (1, 150, 474); place (2, 260, 474); place (4, 370, 474);
    place (5, 480, 474); place (10, 590, 474);
    seabedLabel.setBounds (684, 484, 105, 20);
    seabed.setBounds (684, 514, 105, 27);
    pause.setBounds (676, 542, 210, 26);

    // Input and output are physical side faders; Mix remains below the display.
    inputTrim.setBounds (27, 82, 64, 118);
    trimLabels[0].setBounds (18, 60, 82, 20);
    outputTrim.setBounds (809, 82, 64, 118);
    trimLabels[1].setBounds (800, 60, 82, 20);
    mix.setBounds (698, 590, 78, 80);
    labels[11].setBounds (693, 572, 88, 20);

    settingsButton.setBounds (685, 450, 104, 30);
    updateControlVisibility();
}

float BeneathAudioProcessorEditor::parameter (const char* id) const
{
    return processor.parameters.getRawParameterValue (id)->load();
}

float BeneathAudioProcessorEditor::bottomYAt (float x) const
{
    const auto screen = aquarium.reduced (7.0f);
    const auto waterline = screen.getY() + 64.0f;
    const auto deepY = screen.getBottom() - 8.0f;
    const auto shallowY = juce::jmap (parameter ("shore"), deepY, waterline + 42.0f);
    const auto t = juce::jlimit (0.0f, 1.0f, (x - screen.getX()) / screen.getWidth());
    return juce::jmap (t, deepY, shallowY);
}

juce::Point<float> BeneathAudioProcessorEditor::speakerPoint() const
{
    auto point = restingSpeakerPoint();
    const auto motion = processor.getMotion();
    point.x += motion.sourceHorizontal * 24.0f;
    point.y += motion.sourceVertical * 18.0f;
    return point;
}

juce::Point<float> BeneathAudioProcessorEditor::restingSpeakerPoint() const
{
    const auto screen = aquarium.reduced (7.0f);
    const auto x = juce::jlimit (screen.getX() + 15.0f, screen.getRight() - 15.0f,
                                 screen.getX() + parameter ("sourceX") * screen.getWidth());
    const auto waterline = screen.getY() + 64.0f;
    return { x, juce::jmap (parameter ("sourceDepth"), waterline + 10.0f, bottomYAt (x) - 13.0f) };
}

juce::Point<float> BeneathAudioProcessorEditor::hydrophonePoint() const
{
    auto point = restingHydrophonePoint();
    const auto motion = processor.getMotion();
    point.x += motion.hydrophoneHorizontal * 24.0f;
    point.y += motion.hydrophoneVertical * 18.0f;
    return point;
}

juce::Point<float> BeneathAudioProcessorEditor::restingHydrophonePoint() const
{
    const auto screen = aquarium.reduced (7.0f);
    const auto x = juce::jlimit (screen.getX() + 12.0f, screen.getRight() - 12.0f,
                                 screen.getX() + parameter ("hydroX") * screen.getWidth());
    const auto waterline = screen.getY() + 64.0f;
    return { x, juce::jmap (parameter ("hydroDepth"), waterline + 10.0f, bottomYAt (x) - 15.0f) };
}

juce::Point<float> BeneathAudioProcessorEditor::airStonePoint() const
{
    const auto screen = aquarium.reduced (7.0f);
    const auto x = juce::jlimit (screen.getX() + 17.0f, screen.getRight() - 17.0f,
                                 screen.getX() + parameter ("airStone") * screen.getWidth());
    return { x, bottomYAt (x) - 6.0f };
}

juce::Rectangle<float> BeneathAudioProcessorEditor::mastControlArea() const
{
    const auto screen = aquarium.reduced (7.0f);
    const auto waterline = screen.getY() + 64.0f;
    const auto shipX = screen.getX() + screen.getWidth() * 0.55f;
    // The visible mast is deliberately tiny, but its invisible grab region also
    // covers the cabin and the full flag sweep so the easter egg is easy to play.
    return { shipX - 28.0f, waterline - 76.0f, 112.0f, 61.0f };
}

void BeneathAudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    if (environmentPanelOpen || advancedOpen)
    {
        environmentPanelOpen = false;
        advancedOpen = false;
        settingsButton.setToggleState (false, juce::dontSendNotification);
        updateControlVisibility();
    }

    dragTarget = DragTarget::none;
    const auto point = event.position;
    if (point.getDistanceFrom (speakerPoint()) < 24.0f)
        dragTarget = DragTarget::speaker;
    else if (point.getDistanceFrom (hydrophonePoint()) < 24.0f)
        dragTarget = DragTarget::hydrophone;
    else if (point.getDistanceFrom (airStonePoint()) < 25.0f)
        dragTarget = DragTarget::airStone;
    else if (mastControlArea().contains (point))
        dragTarget = DragTarget::mast;

    if (dragTarget == DragTarget::speaker)
    {
        beginGesture ("sourceDepth");
        beginGesture ("sourceX");
        beginGesture ("distance");
    }
    else if (dragTarget == DragTarget::hydrophone)
    {
        beginGesture ("hydroDepth");
        beginGesture ("hydroX");
        beginGesture ("distance");
    }
    else if (dragTarget == DragTarget::airStone)
        beginGesture ("airStone");
    else if (dragTarget == DragTarget::mast)
    {
        mastDragStartY = event.position.y;
        mastDragStartFeedback = normalizedParameter ("pirateFeedback");
        beginGesture ("pirateFeedback");
    }
}

void BeneathAudioProcessorEditor::mouseDrag (const juce::MouseEvent& event)
{
    if (dragTarget == DragTarget::speaker || dragTarget == DragTarget::hydrophone)
    {
        const auto screen = aquarium.reduced (7.0f);
        const auto normalizedX = (event.position.x - screen.getX()) / screen.getWidth();
        const auto xParameter = dragTarget == DragTarget::speaker ? "sourceX" : "hydroX";
        setParameter (xParameter, normalizedX);
        const auto otherX = parameter (dragTarget == DragTarget::speaker ? "hydroX" : "sourceX");
        const auto separation = std::abs (juce::jlimit (0.0f, 1.0f, normalizedX) - otherX);
        updatingDistanceFromScene = true;
        setParameter ("distance", std::sqrt (separation));
        updatingDistanceFromScene = false;
        const auto x = dragTarget == DragTarget::speaker
                     ? restingSpeakerPoint().x : restingHydrophonePoint().x;
        const auto waterline = screen.getY() + 74.0f;
        const auto normalizedDepth = (event.position.y - waterline)
                                   / juce::jmax (1.0f, bottomYAt (x) - 14.0f - waterline);
        setParameter (dragTarget == DragTarget::speaker ? "sourceDepth" : "hydroDepth", normalizedDepth);
    }
    else if (dragTarget == DragTarget::airStone)
    {
        const auto screen = aquarium.reduced (7.0f);
        setParameter ("airStone", (event.position.x - screen.getX()) / screen.getWidth());
    }
    else if (dragTarget == DragTarget::mast)
    {
        // Use a long relative throw instead of mapping the short mast directly.
        // This permits small adjustments around the lively upper end of the loop.
        constexpr float fullScaleDrag = 96.0f;
        setParameter ("pirateFeedback",
                      mastDragStartFeedback + (mastDragStartY - event.position.y) / fullScaleDrag);
    }
}

void BeneathAudioProcessorEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragTarget == DragTarget::speaker)
    {
        endGesture ("sourceDepth");
        endGesture ("sourceX");
        endGesture ("distance");
    }
    else if (dragTarget == DragTarget::hydrophone)
    {
        endGesture ("hydroDepth");
        endGesture ("hydroX");
        endGesture ("distance");
    }
    else if (dragTarget == DragTarget::airStone)
        endGesture ("airStone");
    else if (dragTarget == DragTarget::mast)
        endGesture ("pirateFeedback");
    dragTarget = DragTarget::none;
}

void BeneathAudioProcessorEditor::mouseMove (const juce::MouseEvent& event)
{
    const auto wasHovered = mastHovered;
    mastHovered = mastControlArea().contains (event.position);
    setMouseCursor (mastHovered ? juce::MouseCursor::UpDownResizeCursor
                                : juce::MouseCursor::NormalCursor);
    if (mastHovered != wasHovered)
        repaint (mastControlArea().expanded (110.0f, 16.0f).toNearestInt());
}

void BeneathAudioProcessorEditor::beginGesture (const char* id)
{
    if (auto* p = processor.parameters.getParameter (id)) p->beginChangeGesture();
}

void BeneathAudioProcessorEditor::setParameter (const char* id, float normalizedValue)
{
    if (auto* p = processor.parameters.getParameter (id))
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, normalizedValue));
}

void BeneathAudioProcessorEditor::endGesture (const char* id)
{
    if (auto* p = processor.parameters.getParameter (id)) p->endChangeGesture();
}

void BeneathAudioProcessorEditor::timerCallback()
{
    if (parameter ("pauseEnvironment") < 0.5f)
        animation += 1.0f / 30.0f;
    const auto wasHovered = mastHovered;
    mastHovered = mastControlArea().contains (getMouseXYRelative().toFloat());
    if (mastHovered != wasHovered)
        setMouseCursor (mastHovered ? juce::MouseCursor::UpDownResizeCursor
                                    : juce::MouseCursor::NormalCursor);
    syncEnvironmentButtons();
    repaint (aquarium.expanded (14.0f).toNearestInt());
}

