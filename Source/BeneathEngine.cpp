#include "BeneathEngine.h"

#include <cmath>

namespace
{
constexpr float soundSpeed = 1500.0f;

float materialValue (BeneathEngine::Seabed material,
                     const std::array<float, 5>& values) noexcept
{
    return values[static_cast<size_t> (material)];
}
}

void BeneathEngine::prepare (double newSampleRate, int maximumBlockSize)
{
    sampleRate = juce::jmax (8000.0, newSampleRate);
    preparedBlockSize = juce::jmax (1, maximumBlockSize);
    const auto maximumDelaySamples = static_cast<int> (sampleRate * 4.0) + preparedBlockSize + 4;
    delayBuffer.setSize (2, maximumDelaySamples);
    diffuseBuffer.setSize (2, preparedBlockSize);

    for (auto& delay : pathDelaySamples)
    {
        delay.reset (sampleRate, 0.08);
        delay.setCurrentAndTargetValue (64.0f);
    }

    juce::Reverb::Parameters reverbParameters;
    reverbParameters.roomSize = 0.72f;
    reverbParameters.damping = 0.78f;
    reverbParameters.wetLevel = 0.58f;
    reverbParameters.dryLevel = 0.0f;
    reverbParameters.width = 0.9f;
    reverbParameters.freezeMode = 0.0f;
    diffuseReverb.setParameters (reverbParameters);
    reset();
}

void BeneathEngine::reset() noexcept
{
    delayBuffer.clear();
    diffuseBuffer.clear();
    diffuseReverb.reset();
    filterState = {};
    bubbles = {};
    whaleVoices = {};
    rainDrops = {};
    surfFilterState = {};
    pinkNoiseState = {};
    rumbleLowpassState = {};
    reverbDarkState = {};
    feedbackInjection = {};
    pirateLowCutState = {};
    pirateHighCutState = {};
    rainBedHighpassState = {};
    rainBedLowpassState = {};
    writePosition = 0;
    samplesUntilBubble = 1.0;
    samplesUntilWave = 1.0;
    samplesUntilWhale = 1.0;
    samplesUntilRain = 1.0;
    samplesUntilRainMotionChange = 1.0;
    surfacePhase = 0.0f;
    swellPhase = 0.0f;
    currentPhase = 0.0f;
    currentPhaseSecondary = 1.7f;
    currentEbb = 0.62f;
    currentEbbTarget = 0.62f;
    currentDrift = 0.0f;
    currentDriftTarget = 0.0f;
    samplesUntilCurrentChange = 1.0;
    rumblePhase = 0.8f;
    rumblePhaseSecondary = 2.4f;
    rainMotionPhase = 0.0f;
    rainBedMotion = 0.5f;
    rainBedMotionTarget = 0.5f;
    rainMotionSlewSeconds = 1.8f;
    surfEnvelope = 0.0f;
    waveEventLevel = 0.0f;
    waveAge = 0;
    waveDuration = 1;
    waveSequenceIndex = 0;
    waveActive = false;
    motionSourceHorizontal.store (0.0f);
    motionSourceVertical.store (0.0f);
    motionHydrophoneHorizontal.store (0.0f);
    motionHydrophoneVertical.store (0.0f);
    motionCurrentEbb.store (0.0f);
    randomState = 0x42454e45u;
}

BeneathEngine::Motion BeneathEngine::getMotion() const noexcept
{
    return { motionSourceHorizontal.load(), motionSourceVertical.load(),
             motionHydrophoneHorizontal.load(), motionHydrophoneVertical.load(),
             motionCurrentEbb.load() };
}

float BeneathEngine::waveCeilingForDepth (float normalizedDepth) noexcept
{
    // Smooth both ends of the travel. In particular, the zero derivative at the
    // seabed prevents a centimetre of upward movement from suddenly revealing
    // a large band of wave noise. The middle half of the water column carries
    // most of the audible transition.
    const auto openness = 1.0f - juce::jlimit (0.0f, 1.0f, normalizedDepth);
    const auto smooth = openness * openness * openness
                      * (openness * (openness * 6.0f - 15.0f) + 10.0f);
    return std::exp (juce::jmap (smooth, std::log (90.0f), std::log (12000.0f)));
}

BeneathEngine::Geometry BeneathEngine::calculateGeometry (const Parameters& p) const noexcept
{
    Geometry g;
    g.waterDepthMetres = 8.0f + 192.0f * p.waterDepth * p.waterDepth;
    g.horizontalDistanceMetres = 1.5f + 148.5f * p.distance * p.distance;
    g.shoreDepthMetres = juce::jmax (1.5f, g.waterDepthMetres * (1.0f - 0.92f * p.shore));
    g.sourceBottomDepthMetres = juce::jmap (p.sourceX, g.waterDepthMetres, g.shoreDepthMetres);
    g.hydrophoneBottomDepthMetres = juce::jmap (p.hydrophoneX, g.waterDepthMetres, g.shoreDepthMetres);
    g.sourceDepthMetres = 0.4f + p.sourceDepth * (g.sourceBottomDepthMetres - 0.9f);
    g.hydrophoneDepthMetres = 0.4f + p.hydrophoneDepth * (g.hydrophoneBottomDepthMetres - 0.9f);

    const auto direction = p.hydrophoneX >= p.sourceX ? 1.0f : -1.0f;
    const auto dx = g.horizontalDistanceMetres * direction;
    const auto sy = g.sourceDepthMetres;
    const auto hy = g.hydrophoneDepthMetres;
    g.pathMetres[0] = std::hypot (dx, sy - hy);
    g.pathMetres[1] = std::hypot (dx, sy + hy);

    // Mirror the source across the sloping seabed y = depth - slope*x.
    const auto slope = (g.hydrophoneBottomDepthMetres - g.sourceBottomDepthMetres) / dx;
    const auto a = -slope;
    const auto b = 1.0f;
    const auto c = -g.sourceBottomDepthMetres;
    const auto projection = (a * 0.0f + b * sy + c) / (a * a + b * b);
    const auto mirroredX = -2.0f * a * projection;
    const auto mirroredY = sy - 2.0f * b * projection;
    g.pathMetres[2] = std::hypot (dx - mirroredX, hy - mirroredY);
    return g;
}

float BeneathEngine::readDelay (int channel, float delaySamples) const noexcept
{
    const auto size = delayBuffer.getNumSamples();
    auto position = static_cast<float> (writePosition) - delaySamples;
    while (position < 0.0f) position += static_cast<float> (size);
    while (position >= static_cast<float> (size)) position -= static_cast<float> (size);
    const auto i0 = static_cast<int> (position);
    const auto i1 = (i0 + 1) % size;
    const auto fraction = position - static_cast<float> (i0);
    return juce::jmap (fraction,
                       delayBuffer.getSample (channel, i0),
                       delayBuffer.getSample (channel, i1));
}

float BeneathEngine::lowpass (int path, int channel, float input, float cutoff) noexcept
{
    const auto a = std::exp (-juce::MathConstants<float>::twoPi
                             * juce::jlimit (80.0f, 20000.0f, cutoff)
                             / static_cast<float> (sampleRate));
    auto& state = filterState[static_cast<size_t> (path)][static_cast<size_t> (channel)];
    state = (1.0f - a) * input + a * state;
    return state;
}

void BeneathEngine::process (const juce::AudioBuffer<float>& input,
                             juce::AudioBuffer<float>& wet,
                             const Parameters& p) noexcept
{
    const auto numSamples = input.getNumSamples();
    wet.setSize (2, numSamples, false, false, true);
    wet.clear();
    diffuseBuffer.setSize (2, numSamples, false, false, true);
    diffuseBuffer.clear();

    const auto geometry = calculateGeometry (p);
    const auto exaggeration = 1.0f + 2.5f * p.dream;
    for (int path = 0; path < pathCount; ++path)
        pathDelaySamples[static_cast<size_t> (path)].setTargetValue (
            geometry.pathMetres[static_cast<size_t> (path)] / soundSpeed
            * static_cast<float> (sampleRate) * exaggeration);

    const auto bottomGain = materialValue (p.seabed, { 0.035f, 0.10f, 0.30f, 0.72f, 0.52f });
    const auto bottomCutoff = materialValue (p.seabed, { 620.0f, 1450.0f, 5600.0f, 17800.0f, 11800.0f });
    const auto distanceLoss = 1.0f / std::sqrt (1.0f + geometry.pathMetres[0] * 0.052f);
    const auto farCutoff = materialValue (p.seabed,
        { 150.0f, 410.0f, 880.0f, 1650.0f, 1180.0f }) * (1.0f - 0.32f * p.dream);
    const auto distanceDarkness = std::pow (p.distance, 2.30f);
    // Interpolate in log-frequency space so the far half of the control reaches
    // genuinely remote, murky territory rather than stopping at a polite low-pass.
    const auto materialDistanceCutoff = std::exp (juce::jmap (distanceDarkness,
        std::log (17800.0f), std::log (juce::jmax (90.0f, farCutoff))));
    const auto basementAmount = juce::jlimit (0.0f, 1.0f, (p.distance - 0.50f) * 2.0f);
    const auto basementCurve = basementAmount * basementAmount * (3.0f - 2.0f * basementAmount);
    const auto depthWeight = juce::jlimit (0.0f, 1.0f,
        0.50f * p.waterDepth + 0.25f * p.sourceDepth + 0.25f * p.hydrophoneDepth);
    const auto basementFarCutoff = juce::jmap (depthWeight, 380.0f, 150.0f);
    const auto basementCutoff = std::exp (juce::jmap (basementCurve,
        std::log (17800.0f), std::log (basementFarCutoff)));
    const auto directCutoff = juce::jmin (materialDistanceCutoff, basementCutoff);
    const auto surfaceCutoff = juce::jmin (8800.0f - 4200.0f * p.waves,
                                           directCutoff * 1.22f);
    const auto distanceBottomCutoff = juce::jmin (bottomCutoff, directCutoff * 0.88f);
    const auto columnScale = std::sqrt (geometry.sourceBottomDepthMetres
                                       * geometry.hydrophoneBottomDepthMetres);
    const auto surfaceCloseness = 1.0f - juce::jlimit (0.0f, 1.0f,
        std::sqrt (geometry.sourceDepthMetres * geometry.hydrophoneDepthMetres) / columnScale);
    const auto sourceBottomDistance = geometry.sourceBottomDepthMetres - geometry.sourceDepthMetres;
    const auto hydroBottomDistance = geometry.hydrophoneBottomDepthMetres - geometry.hydrophoneDepthMetres;
    const auto bottomCloseness = 1.0f - juce::jlimit (0.0f, 1.0f,
        std::sqrt (sourceBottomDistance * hydroBottomDistance) / columnScale);
    const auto surfaceGain = (0.20f + 0.52f * std::pow (surfaceCloseness, 0.7f))
                           * (1.0f + 0.55f * p.dream) * (1.0f - 0.24f * p.waves);
    const auto positionedBottomGain = bottomGain
                                    * (0.38f + 1.35f * std::pow (bottomCloseness, 0.72f))
                                    * (1.0f + 0.60f * p.dream);
    const auto materialFeedback = materialValue (p.seabed, { 0.15f, 0.28f, 0.52f, 1.0f, 0.78f });
    const auto abyssDistance = juce::jlimit (0.0f, 1.0f, (p.distance - 0.65f) / 0.35f);
    const auto abyssDepth = juce::jlimit (0.0f, 1.0f, (depthWeight - 0.55f) / 0.45f);
    const auto abyssFeedback = 0.075f * abyssDistance * abyssDistance
                            * (3.0f - 2.0f * abyssDistance)
                            * abyssDepth * abyssDepth * (3.0f - 2.0f * abyssDepth);
    const auto feedbackAmount = (0.018f + 0.19f * bottomCloseness * materialFeedback)
                              * (0.72f + 0.28f * p.dream) + abyssFeedback;
    const auto normalizedSeparation = juce::jmax (0.01f, std::abs (p.hydrophoneX - p.sourceX));
    const auto metresPerNormalizedX = geometry.horizontalDistanceMetres / normalizedSeparation;
    const auto stoneDepth = juce::jmap (p.airStonePosition,
                                        geometry.waterDepthMetres, geometry.shoreDepthMetres);
    const auto stoneHorizontalDistance = (p.airStonePosition - p.hydrophoneX)
                                       * metresPerNormalizedX;
    const auto stoneDistance = std::hypot (stoneHorizontalDistance,
                                           geometry.hydrophoneDepthMetres - stoneDepth);
    const auto bubbleProximity = 0.025f + 1.85f * std::exp (-stoneDistance / 19.0f);
    const auto bubblePan = juce::jlimit (-0.92f, 0.92f,
        stoneHorizontalDistance / 32.0f);
    const auto reverbSend = p.distanceReverb
                          * (0.16f + 0.84f * juce::jlimit (0.0f, 1.0f,
                              geometry.pathMetres[0] / 90.0f));
    const auto hydrophoneWaveCeiling = waveCeilingForDepth (p.hydrophoneDepth);
    const auto phaseRate = juce::MathConstants<float>::twoPi / static_cast<float> (sampleRate);
    const auto pirateAmount = juce::jlimit (0.0f, 1.0f, p.pirateFeedback);
    const auto pirateBlend = pirateAmount * pirateAmount * (3.0f - 2.0f * pirateAmount);
    const auto pirateLowCutCoefficient = 1.0f - std::exp (
        -juce::MathConstants<float>::twoPi * 110.0f / static_cast<float> (sampleRate));
    const auto pirateHighCutCoefficient = 1.0f - std::exp (
        -juce::MathConstants<float>::twoPi * 7800.0f / static_cast<float> (sampleRate));

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto inputL = input.getSample (0, sample);
        const auto inputR = input.getNumChannels() > 1 ? input.getSample (1, sample) : inputL;
        delayBuffer.setSample (0, writePosition, inputL + feedbackInjection[0]);
        delayBuffer.setSample (1, writePosition, inputR + feedbackInjection[1]);

        if (! p.pauseEnvironment)
        {
            surfacePhase += phaseRate * (0.07f + 0.42f * p.waves);
            swellPhase += phaseRate * (0.017f + 0.08f * p.current);
            if (--samplesUntilCurrentChange <= 0.0)
            {
                currentEbbTarget = 0.28f + 0.72f * random01();
                currentDriftTarget = randomSigned() * 0.32f;
                samplesUntilCurrentChange = sampleRate * (3.5 + 8.5 * random01());
            }
            const auto currentSlew = 1.0f - std::exp (-1.0f / static_cast<float> (sampleRate * 2.8));
            currentEbb += (currentEbbTarget - currentEbb) * currentSlew;
            currentDrift += (currentDriftTarget - currentDrift) * currentSlew;
            const auto currentRateHz = 0.003f * std::pow (300.0f, p.current)
                                     * (0.68f + 0.52f * currentEbb);
            currentPhase += phaseRate * currentRateHz;
            currentPhaseSecondary += phaseRate * currentRateHz * (0.54f + 0.14f * currentEbb);
            const auto rumbleRateHz = 0.005f + 0.075f * p.waves * p.waves;
            rumblePhase += phaseRate * rumbleRateHz;
            rumblePhaseSecondary += phaseRate * rumbleRateHz * 0.59f;

            if (p.waves > 0.001f)
            {
                if (waveActive)
                {
                    if (++waveAge >= waveDuration)
                    {
                        waveActive = false;
                        surfEnvelope = 0.0f;
                        const auto intervalSeconds = std::exp (juce::jmap (
                            std::pow (p.waves, 0.74f), std::log (15.0f), std::log (1.0f)));
                        const auto eventSeconds = static_cast<double> (waveDuration) / sampleRate;
                        const auto wasLargeWave = waveEventLevel > 1.0f;
                        const auto timingVariation = wasLargeWave
                            ? 1.30 + 0.75 * random01()
                            : 0.55 + 1.10 * random01();
                        const auto remainingInterval = juce::jmax (0.04,
                            intervalSeconds * timingVariation - eventSeconds);
                        samplesUntilWave = sampleRate * remainingInterval;
                    }
                }
                else if (--samplesUntilWave <= 0.0)
                {
                    waveActive = true;
                    waveAge = 0;
                    waveDuration = static_cast<int> (sampleRate * juce::jmap (p.waves, 4.8f, 1.05f));
                    const auto seventhWave = waveSequenceIndex == 6;
                    const auto sequenceRise = 0.52f + 0.48f
                        * static_cast<float> (waveSequenceIndex) / 6.0f;
                    const auto randomCrest = 0.42f + 0.58f * std::pow (random01(), 1.35f);
                    const auto featuredWave = (seventhWave && random01() < 0.86f)
                                           || (! seventhWave && random01() < 0.06f);
                    waveEventLevel = featuredWave
                        ? 1.08f + 0.44f * random01()
                        : sequenceRise * randomCrest;
                    waveSequenceIndex = (waveSequenceIndex + 1) % 7;
                }
            }
            else
            {
                waveActive = false;
                surfEnvelope = 0.0f;
                samplesUntilWave = 1.0;
            }

            if (p.whales > 0.001f)
            {
                if (--samplesUntilWhale <= 0.0)
                {
                    spawnWhaleCall (p);
                    const auto nominalInterval = std::exp (juce::jmap (
                        std::pow (p.whales, 0.72f), std::log (28.0f), std::log (5.0f)));
                    samplesUntilWhale = sampleRate * nominalInterval
                                      * (0.62 + 0.92 * random01());
                }
            }
            else
                samplesUntilWhale = 1.0;

            if (p.rain > 0.001f)
            {
                if (--samplesUntilRain <= 0.0)
                {
                    spawnRainDrop (p);
                    if (random01() < 0.18f + 0.42f * p.rain)
                        spawnRainDrop (p);
                    const auto eventsPerSecond = std::exp (juce::jmap (
                        std::pow (p.rain, 0.82f), std::log (0.10f), std::log (185.0f)));
                    samplesUntilRain = sampleRate / eventsPerSecond
                                     * (0.28 + 1.34 * random01());
                }

                rainMotionPhase += phaseRate * (0.025f + 0.018f * p.rain);
                if (--samplesUntilRainMotionChange <= 0.0)
                {
                    const auto sampledSine = 0.5f + 0.5f * std::sin (rainMotionPhase);
                    rainBedMotionTarget = juce::jlimit (0.04f, 0.96f,
                        sampledSine + randomSigned() * 0.13f);
                    rainMotionSlewSeconds = 1.15f + 1.25f * random01();
                    samplesUntilRainMotionChange = sampleRate * (2.4 + 5.8 * random01());
                }
                const auto rainMotionSlew = 1.0f - std::exp (
                    -1.0f / static_cast<float> (sampleRate * rainMotionSlewSeconds));
                rainBedMotion += (rainBedMotionTarget - rainBedMotion) * rainMotionSlew;
            }
            else
            {
                samplesUntilRain = 1.0;
                samplesUntilRainMotionChange = 1.0;
            }
        }
        if (surfacePhase >= juce::MathConstants<float>::twoPi) surfacePhase -= juce::MathConstants<float>::twoPi;
        if (swellPhase >= juce::MathConstants<float>::twoPi) swellPhase -= juce::MathConstants<float>::twoPi;
        if (currentPhase >= juce::MathConstants<float>::twoPi) currentPhase -= juce::MathConstants<float>::twoPi;
        if (currentPhaseSecondary >= juce::MathConstants<float>::twoPi) currentPhaseSecondary -= juce::MathConstants<float>::twoPi;
        if (rumblePhase >= juce::MathConstants<float>::twoPi) rumblePhase -= juce::MathConstants<float>::twoPi;
        if (rumblePhaseSecondary >= juce::MathConstants<float>::twoPi) rumblePhaseSecondary -= juce::MathConstants<float>::twoPi;
        if (rainMotionPhase >= juce::MathConstants<float>::twoPi) rainMotionPhase -= juce::MathConstants<float>::twoPi;

        float waveProgress = 0.0f;
        if (waveActive)
        {
            waveProgress = static_cast<float> (waveAge) / static_cast<float> (juce::jmax (1, waveDuration));
            constexpr float breakPoint = 0.62f;
            const auto risePeak = 0.18f + 0.42f * p.waves;
            if (waveProgress < breakPoint)
                surfEnvelope = waveEventLevel * std::pow (waveProgress / breakPoint, 2.0f)
                             * risePeak;
            else
            {
                const auto tailProgress = (waveProgress - breakPoint) / (1.0f - breakPoint);
                const auto recedingSwell = risePeak * std::exp (-tailProgress * (2.4f + 1.4f * p.waves));
                const auto softCrash = (0.04f + 0.22f * p.waves)
                                     * (1.0f - std::exp (-tailProgress * 9.0f))
                                     * std::exp (-tailProgress * (2.0f + 1.3f * p.waves));
                surfEnvelope = waveEventLevel * (recedingSwell + softCrash);
            }
        }
        const auto surfaceMotion = (std::sin (surfacePhase) + 0.45f * std::sin (swellPhase))
                                 * (1.0f + surfEnvelope)
                                 * (1.0f + 52.0f * p.waves * p.waves * (0.4f + p.dream));

        const auto directDelayBase = pathDelaySamples[0].getNextValue();
        const auto surfaceDelayBase = pathDelaySamples[1].getNextValue();
        const auto bottomDelayBase = pathDelaySamples[2].getNextValue();

        // The delay motion now comes from displaced objects rather than an
        // abstract delay LFO. Both devices lean with the same body of water,
        // but their different drag and cable lengths keep the paths alive.
        const auto flow = ((std::sin (currentPhase) + 0.34f * std::sin (currentPhaseSecondary))
                          / 1.34f) * currentEbb + 0.22f * currentDrift;
        const auto currentStrength = p.current * p.current * (0.72f + 0.48f * p.dream);
        const auto sourceDx = flow * currentStrength * 3.2f * (0.35f + 0.65f * p.sourceDepth);
        const auto hydroDx = flow * currentStrength * 5.0f * (0.35f + 0.65f * p.hydrophoneDepth);
        const auto sourceCableRise = -0.055f * std::abs (sourceDx);
        const auto hydroCableRise = -0.070f * std::abs (hydroDx);
        const auto sourceSurfaceAmount = std::pow (1.0f - p.sourceDepth, 2.2f);
        const auto hydroSurfaceAmount = std::pow (1.0f - p.hydrophoneDepth, 2.2f);
        const auto waveBob = (std::sin (surfacePhase) + 0.32f * std::sin (swellPhase))
                           * p.waves * p.waves * (0.45f + 0.55f * p.dream)
                           * (1.0f + 0.35f * surfEnvelope);
        const auto sourceDy = sourceCableRise + waveBob * 0.52f * sourceSurfaceAmount;
        const auto hydroDy = hydroCableRise + waveBob * 0.92f * hydroSurfaceAmount;

        const auto sx = sourceDx;
        const auto hx = (p.hydrophoneX >= p.sourceX ? 1.0f : -1.0f)
                      * geometry.horizontalDistanceMetres + hydroDx;
        const auto sy = juce::jlimit (0.12f, geometry.sourceBottomDepthMetres - 0.12f,
                                      geometry.sourceDepthMetres + sourceDy);
        const auto hy = juce::jlimit (0.12f, geometry.hydrophoneBottomDepthMetres - 0.12f,
                                      geometry.hydrophoneDepthMetres + hydroDy);
        std::array<float, 3> movedPath {};
        movedPath[0] = std::hypot (hx - sx, sy - hy);
        movedPath[1] = std::hypot (hx - sx, sy + hy);
        const auto restingDx = (p.hydrophoneX >= p.sourceX ? 1.0f : -1.0f)
                             * geometry.horizontalDistanceMetres;
        const auto slope = (geometry.hydrophoneBottomDepthMetres
                          - geometry.sourceBottomDepthMetres) / restingDx;
        const auto a = -slope;
        constexpr float b = 1.0f;
        const auto c = -geometry.sourceBottomDepthMetres;
        const auto projection = (a * sx + b * sy + c) / (a * a + b * b);
        const auto mirroredX = sx - 2.0f * a * projection;
        const auto mirroredY = sy - 2.0f * b * projection;
        movedPath[2] = std::hypot (hx - mirroredX, hy - mirroredY);
        const auto delayScale = static_cast<float> (sampleRate) / soundSpeed * exaggeration;
        const auto directMotion = (movedPath[0] - geometry.pathMetres[0]) * delayScale;
        const auto surfacePathMotion = (movedPath[1] - geometry.pathMetres[1]) * delayScale
                                     + surfaceMotion * 0.18f;
        const auto bottomMotion = (movedPath[2] - geometry.pathMetres[2]) * delayScale;

        motionSourceHorizontal.store (sourceDx / 5.0f);
        motionSourceVertical.store (sourceDy / 1.0f);
        motionHydrophoneHorizontal.store (hydroDx / 5.0f);
        motionHydrophoneVertical.store (hydroDy / 1.0f);
        motionCurrentEbb.store (currentEbb * p.current);

        float out[2] {};
        for (int channel = 0; channel < 2; ++channel)
        {
            const auto direct = readDelay (channel, juce::jmax (1.0f, directDelayBase + directMotion));
            const auto surface = readDelay (channel, juce::jmax (1.0f,
                surfaceDelayBase + surfacePathMotion));
            auto bottomDelay = bottomDelayBase + bottomMotion;
            if (channel == 1)
                bottomDelay += 3.0f + 19.0f * p.shore;
            const auto bottom = readDelay (channel, juce::jmax (1.0f, bottomDelay));

            out[channel] = lowpass (0, channel, direct, directCutoff) * distanceLoss * 0.86f
                         - lowpass (1, channel, surface, surfaceCutoff)
                           * surfaceGain * (0.46f + 0.54f * distanceLoss)
                         + lowpass (2, channel, bottom, distanceBottomCutoff)
                           * positionedBottomGain * distanceLoss;
        }

        // A low pink-noise sea floor remains whenever waves are enabled. It lifts
        // modestly with intensity and swells beneath each discrete wave event.
        if (p.waves > 0.001f)
        {
            const auto rumbleCutoffMotion = 1.0f
                + (0.12f + 0.20f * p.waves) * (0.72f * std::sin (rumblePhase)
                                               + 0.28f * std::sin (rumblePhaseSecondary));
            const auto rumbleCutoff = juce::jmin (
                (82.0f + 720.0f * std::pow (p.waves, 1.45f)) * rumbleCutoffMotion,
                hydrophoneWaveCeiling);
            const auto rumbleCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                                                          * rumbleCutoff / static_cast<float> (sampleRate));
            constexpr float waveOutputScale = 0.22f;
            const auto rumbleGain = juce::jmap (p.waves, 0.10f, 0.40f)
                                   * p.waveVolume * waveOutputScale;
            const auto swellGain = 1.0f + surfEnvelope * (0.55f + 1.15f * p.waves);
            const auto levelMotion = 1.0f
                + (0.08f + 0.12f * p.waves) * (0.68f * std::sin (rumblePhaseSecondary)
                                               + 0.32f * std::sin (rumblePhase));
            const auto stereoDrift = (0.04f + 0.10f * p.waves) * std::sin (rumblePhaseSecondary);
            for (int channel = 0; channel < 2; ++channel)
            {
                const auto white = randomSigned();
                auto& pink = pinkNoiseState[static_cast<size_t> (channel)];
                pink[0] = 0.99765f * pink[0] + white * 0.0990460f;
                pink[1] = 0.96300f * pink[1] + white * 0.2965164f;
                pink[2] = 0.57000f * pink[2] + white * 1.0526913f;
                const auto pinkSample = (pink[0] + pink[1] + pink[2] + white * 0.1848f) * 0.05f;
                auto& low = rumbleLowpassState[static_cast<size_t> (channel)];
                low += rumbleCoefficient * (pinkSample - low);
                const auto channelDrift = channel == 0 ? 1.0f + stereoDrift : 1.0f - stereoDrift;
                out[channel] += low * rumbleGain * swellGain * levelMotion * channelDrift;
            }
        }

        // Reflection geometry changes immediately, but breaking beach wash only
        // appears after the first fifth of the slope.
        const auto shoreSurf = std::pow (juce::jlimit (0.0f, 1.0f,
            (p.shore - 0.20f) / 0.80f), 1.55f);
        if (shoreSurf > 0.0f && p.waves > 0.001f && waveActive)
        {
            constexpr float breakPoint = 0.62f;
            const auto preBreakCutoff = juce::jmap (p.waves * p.waves, 105.0f, 720.0f);
            const auto postBreakCutoff = juce::jmap (std::pow (p.waves, 1.6f), 260.0f, 6500.0f);
            const auto brightnessProgress = juce::jlimit (0.0f, 1.0f,
                (waveProgress - breakPoint) / 0.18f);
            const auto smoothBrightness = brightnessProgress * brightnessProgress
                                        * (3.0f - 2.0f * brightnessProgress);
            const auto cutoff = juce::jmin (
                juce::jmap (smoothBrightness, preBreakCutoff, postBreakCutoff),
                hydrophoneWaveCeiling);
            const auto filterCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                                                          * cutoff / static_cast<float> (sampleRate));
            for (int channel = 0; channel < 2; ++channel)
            {
                const auto noise = randomSigned();
                surfFilterState[static_cast<size_t> (channel)] +=
                    filterCoefficient * (noise - surfFilterState[static_cast<size_t> (channel)]);
                out[channel] += surfFilterState[static_cast<size_t> (channel)]
                              * surfEnvelope * shoreSurf * p.waveVolume
                              * (0.055f + 0.32f * p.waves) * 0.22f;
            }
        }

        if (p.pump > 0.001f)
        {
            --samplesUntilBubble;
            if (samplesUntilBubble <= 0.0)
            {
                spawnBubble (p, bubbleProximity, bubblePan);
                const auto rateScale = 0.2f + 1.8f * p.bubbleRate;
                const auto eventsPerSecond = (0.7f + 38.0f * p.pump * p.pump) * rateScale;
                samplesUntilBubble = sampleRate / eventsPerSecond * (0.45 + random01());
            }
        }
        float bubblesL = 0.0f, bubblesR = 0.0f;
        renderBubbles (bubblesL, bubblesR);
        out[0] += bubblesL;
        out[1] += bubblesR;

        float whalesL = 0.0f, whalesR = 0.0f;
        const auto listeningHydrophoneX = juce::jlimit (0.0f, 1.0f,
            p.hydrophoneX + hydroDx / metresPerNormalizedX);
        renderWhales (whalesL, whalesR, p.whaleVolume,
                      listeningHydrophoneX, p.hydrophoneDepth);
        out[0] += whalesL;
        out[1] += whalesR;

        float rainL = 0.0f, rainR = 0.0f;
        renderRain (rainL, rainR, p, listeningHydrophoneX);
        out[0] += rainL;
        out[1] += rainR;

        for (int channel = 0; channel < 2; ++channel)
        {
            const auto safe = std::tanh (out[channel]);
            wet.setSample (channel, sample, safe);
            const auto whale = channel == 0 ? whalesL : whalesR;
            const auto rainSample = channel == 0 ? rainL : rainR;
            diffuseBuffer.setSample (channel, sample,
                safe * reverbSend + whale * (0.22f + 0.30f * p.whales)
                + rainSample * (0.06f + 0.12f * p.rain));
        }

        // A quiet cross-coupled return lets reflections recede in repeated bounces.
        // Hoisting the hidden flag morphs that return toward a protected, near-unity
        // loop. The low cut rejects DC and subsonics, upper damping prevents
        // ultrasonic build-up, and two limiting stages keep full sail playable.
        const auto feedbackMotion = juce::jlimit (0.42f, 1.62f,
            1.0f + p.current * (0.44f * std::sin (currentPhase)
                              + 0.18f * std::sin (currentPhaseSecondary)));
        for (int channel = 0; channel < 2; ++channel)
        {
            const auto opposite = 1 - channel;
            const auto returnSample = std::tanh (out[opposite]);
            const auto normalReturn = returnSample * feedbackAmount * feedbackMotion;

            auto& lowCut = pirateLowCutState[static_cast<size_t> (channel)];
            auto& highCut = pirateHighCutState[static_cast<size_t> (channel)];
            lowCut += pirateLowCutCoefficient * (returnSample - lowCut);
            const auto highPassed = returnSample - lowCut;
            highCut += pirateHighCutCoefficient * (highPassed - highCut);

            const auto currentFlutter = 0.97f + 0.03f * std::sin (currentPhaseSecondary);
            const auto pirateGain = juce::jmin (0.995f,
                (0.35f + 0.645f * pirateAmount) * currentFlutter);
            const auto protectedReturn = juce::jlimit (-0.92f, 0.92f,
                std::tanh (highCut * 1.35f) * pirateGain);
            feedbackInjection[static_cast<size_t> (channel)] =
                juce::jmap (pirateBlend, normalReturn, protectedReturn);
        }

        writePosition = (writePosition + 1) % delayBuffer.getNumSamples();
    }

    diffuseReverb.processStereo (diffuseBuffer.getWritePointer (0),
                                 diffuseBuffer.getWritePointer (1), numSamples);
    for (int channel = 0; channel < 2; ++channel)
    {
        const auto reverbCutoff = 2400.0f + 1800.0f * (1.0f - p.distanceReverb);
        const auto a = std::exp (-juce::MathConstants<float>::twoPi * reverbCutoff
                                 / static_cast<float> (sampleRate));
        for (int sample = 0; sample < numSamples; ++sample)
        {
            auto& state = reverbDarkState[static_cast<size_t> (channel)];
            state = (1.0f - a) * diffuseBuffer.getSample (channel, sample) + a * state;
            wet.addSample (channel, sample, state);
        }
    }
}

void BeneathEngine::spawnWhaleCall (const Parameters& p) noexcept
{
    const auto highBand = random01() < 0.24f;
    const auto downward = random01() < (highBand ? 0.38f : 0.90f);
    const auto longCall = downward && random01() < (highBand ? 0.26f : 0.70f);
    // Rising gestures are quick strokes. The low descending family carries the
    // long, slowly undulating drone behaviour seen in many song spectrograms.
    const auto durationSeconds = ! downward
        ? (highBand ? 0.08f + 0.28f * random01() : 0.16f + 0.58f * random01())
        : highBand
            ? (longCall ? 0.85f + 1.15f * random01() : 0.32f + 0.62f * random01())
            : (longCall ? 4.5f + 5.0f * random01() : 1.35f + 2.15f * random01());
    const auto terminalRise = ! highBand && downward && random01() < 0.24f;
    float baseStart = 0.0f;
    float baseEnd = 0.0f;
    float terminalRiseSemitones = 0.0f;
    if (highBand)
    {
        const auto upperHz = 9000.0f + 1500.0f * random01();
        const auto lowerHz = 7000.0f + 1000.0f * random01();
        baseStart = downward ? upperHz : lowerHz;
        baseEnd = downward ? lowerHz : upperHz;
    }
    else if (downward)
    {
        baseStart = std::exp (juce::jmap (random01(), std::log (40.0f),
            std::log (terminalRise ? 330.0f : 400.0f)));
        const auto fallSemitones = terminalRise
            ? 0.05f + 0.40f * random01()
            : 0.08f + 2.92f * std::pow (random01(), 1.65f);
        baseEnd = juce::jmax (32.0f,
            baseStart * std::exp2 (-fallSemitones / 12.0f));
        terminalRiseSemitones = terminalRise ? 0.8f + 2.2f * random01() : 0.0f;
    }
    else
    {
        baseStart = 34.0f + 125.0f * random01();
        baseEnd = juce::jmin (400.0f,
            baseStart * std::exp2 ((4.0f + 12.0f * random01()) / 12.0f));
    }
    const auto sourceOnLeft = p.hydrophoneX > 0.56f
        || (p.hydrophoneX >= 0.44f && random01() < 0.5f);
    // The source exists well beyond the visible aquarium. Its bearing is fixed
    // for the phrase, so receiver movement creates only distant parallax.
    const auto distantX = sourceOnLeft
        ? -(0.45f + 0.75f * random01())
        : 1.45f + 0.75f * random01();
    const auto predelaySeconds = 0.018f + 0.055f * random01();
    // Most low clusters are almost simultaneous (10–30 ms); the skewed tail
    // occasionally opens them out as far as 150 ms.
    const auto lowClusterSpan = 0.010f + 0.140f * std::pow (random01(), 3.0f);
    const auto highBurstSpacing = 0.042f + 0.068f * random01();
    const auto highPhraseGap = 0.10f + 0.18f * random01();
    const auto highPhraseCount = 2 + static_cast<int> (random01() * 2.0f);
    const auto highBlipsPerPhrase = 3 + static_cast<int> (random01() * 3.0f);
    const auto layerCount = highBand ? highPhraseCount * highBlipsPerPhrase : 4;
    const auto highPhraseDriftSemitones = randomSigned() * (0.45f + 1.35f * random01());
    const auto compoundDrone = ! highBand && downward && longCall && random01() < 0.68f;

    int layer = 0;
    for (auto& voice : whaleVoices)
    {
        if (voice.active) continue;
        const auto detune = std::exp (randomSigned() * 0.022f);
        voice.active = true;
        voice.age = 0;
        const auto phraseIndex = highBand ? layer / highBlipsPerPhrase : 0;
        const auto blipIndex = highBand ? layer % highBlipsPerPhrase : 0;
        const auto layerOffset = highBand
            ? static_cast<float> (phraseIndex)
                * (highBurstSpacing * static_cast<float> (highBlipsPerPhrase - 1)
                   + highPhraseGap)
              + highBurstSpacing * static_cast<float> (blipIndex)
            : lowClusterSpan * static_cast<float> (layer) / 3.0f;
        voice.startDelay = static_cast<int> (sampleRate
            * (predelaySeconds + layerOffset));
        voice.duration = static_cast<int> (sampleRate * durationSeconds
            * (0.94f + 0.12f * random01()));
        voice.phase = random01() * juce::MathConstants<float>::twoPi;
        voice.vibratoPhase = random01() * juce::MathConstants<float>::twoPi;
        const auto bandMinimum = highBand ? 7000.0f : 32.0f;
        const auto bandMaximum = highBand ? 10500.0f : 400.0f;
        const auto phrasePitch = highBand && highPhraseCount > 1
            ? std::exp2 (highPhraseDriftSemitones
                * static_cast<float> (phraseIndex)
                / static_cast<float> (highPhraseCount - 1) / 12.0f)
            : 1.0f;
        voice.startHz = juce::jlimit (bandMinimum, bandMaximum,
            baseStart * detune * phrasePitch);
        const auto endVariation = (! highBand && downward)
            ? std::exp (randomSigned() * 0.004f)
            : 0.96f + 0.08f * random01();
        voice.endHz = juce::jlimit (bandMinimum, bandMaximum,
            baseEnd * detune * endVariation * phrasePitch);
        voice.vibratoRate = 0.42f + 1.18f * random01();
        voice.bend = randomSigned() * (longCall ? 0.20f : 0.11f);
        voice.waveBend = highBand ? 0.075f : 0.18f + 0.035f * random01();
        voice.harmonic = highBand ? 0.012f
                                  : compoundDrone ? 0.11f + 0.07f * random01()
                                                  : 0.045f + 0.025f * random01();
        voice.harmonic3 = compoundDrone ? 0.045f + 0.055f * random01() : 0.0f;
        voice.harmonic4 = compoundDrone ? 0.018f + 0.035f * random01() : 0.0f;
        voice.swayDepth = (! highBand && downward)
            ? (longCall ? 0.008f + 0.018f * random01() : 0.004f + 0.009f * random01())
            : 0.0f;
        voice.swayCycles = 0.65f + (longCall ? 1.35f : 0.55f) * random01();
        voice.swayPhase = random01() * juce::MathConstants<float>::twoPi;
        voice.terminalRise = terminalRise;
        voice.terminalRiseSemitones = terminalRiseSemitones
                                    * (0.94f + 0.12f * random01());
        voice.distantX = distantX;
        const auto layerGain = highBand
            ? std::pow (0.73f, static_cast<float> (blipIndex))
              * std::pow (0.88f, static_cast<float> (phraseIndex))
            : std::pow (0.73f, static_cast<float> (layer));
        voice.gain = 0.075f * (highBand ? 0.42f : 0.82f)
                   * layerGain
                   * (0.82f + 0.30f * random01());
        voice.pan = randomSigned() * 0.035f
                  + (static_cast<float> (blipIndex) - 1.5f) * 0.008f;
        if (++layer == layerCount) break;
    }
}

void BeneathEngine::renderWhales (float& left, float& right, float volume,
                                  float hydrophoneX, float hydrophoneDepth) noexcept
{
    const auto phaseScale = juce::MathConstants<float>::twoPi / static_cast<float> (sampleRate);
    for (auto& voice : whaleVoices)
    {
        if (! voice.active) continue;
        if (voice.startDelay > 0)
        {
            --voice.startDelay;
            continue;
        }

        const auto t = static_cast<float> (voice.age) / static_cast<float> (voice.duration);
        if (t >= 1.0f)
        {
            voice.active = false;
            continue;
        }

        const auto glide = t * t * (3.0f - 2.0f * t);
        const auto shapedGlide = juce::jlimit (0.0f, 1.0f,
            glide + voice.bend * std::sin (juce::MathConstants<float>::pi * t));
        auto frequency = std::exp (juce::jmap (shapedGlide,
            std::log (voice.startHz), std::log (voice.endHz)));
        frequency *= std::exp (voice.swayDepth
            * (0.74f * std::sin (voice.swayPhase
                               + juce::MathConstants<float>::twoPi * voice.swayCycles * t)
             + 0.26f * std::sin (voice.swayPhase * 0.61f
                               + juce::MathConstants<float>::twoPi
                               * (voice.swayCycles * 1.83f) * t)));
        if (voice.terminalRise)
        {
            const auto risePosition = juce::jlimit (0.0f, 1.0f, (t - 0.82f) / 0.13f);
            const auto fastRise = risePosition * risePosition
                                * (3.0f - 2.0f * risePosition);
            frequency *= std::exp2 (voice.terminalRiseSemitones * fastRise / 12.0f);
        }
        frequency *= 1.0f + 0.014f * std::sin (voice.vibratoPhase)
                           + 0.004f * std::sin (voice.vibratoPhase * 0.37f + 1.2f);
        const auto highVoice = voice.startHz > 1000.0f;
        frequency = highVoice ? juce::jlimit (7000.0f, 10500.0f, frequency)
                              : juce::jlimit (32.0f, 400.0f, frequency);

        const auto attackPosition = juce::jlimit (0.0f, 1.0f, t / 0.14f);
        const auto releasePosition = juce::jlimit (0.0f, 1.0f, (1.0f - t) / 0.30f);
        const auto attack = attackPosition * attackPosition * (3.0f - 2.0f * attackPosition);
        const auto release = releasePosition * releasePosition * (3.0f - 2.0f * releasePosition);
        const auto phraseBreath = 0.84f + 0.16f
            * std::sin (juce::MathConstants<float>::twoPi * (1.4f * t + 0.07f));
        // A mildly phase-bent sine: rounder than a saw by a wide margin, but
        // asymmetric enough to avoid the sterile purity of a test oscillator.
        const auto bentPhase = voice.phase
                             + voice.waveBend * std::sin (voice.phase - 0.38f);
        const auto tone = std::sin (bentPhase)
                        + voice.harmonic * std::sin (voice.phase * 2.0f - 0.55f)
                        + voice.harmonic3 * std::sin (voice.phase * 3.0f + 0.82f)
                        + voice.harmonic4 * std::sin (voice.phase * 4.0f - 1.17f);
        const auto relativeDistance = std::abs (voice.distantX - hydrophoneX);
        const auto parallaxGain = 1.0f / std::sqrt (0.78f + 0.22f * relativeDistance);
        const auto depthReception = highVoice
            ? 1.0f - 0.18f * hydrophoneDepth
            : 0.94f + 0.06f * hydrophoneDepth;
        const auto value = std::tanh (tone * 0.91f) * attack * release
                         * phraseBreath * voice.gain
                         * juce::jlimit (0.0f, 1.0f, volume)
                         * parallaxGain * depthReception;
        // A pan value of ±0.2 is approximately a 60/40 power balance. Full
        // hydrophone travel only nudges this image; it never becomes hard-panned.
        const auto receivedPan = juce::jlimit (-0.32f, 0.32f,
            0.16f * (voice.distantX - hydrophoneX) + voice.pan);
        left += value * std::sqrt (0.5f * (1.0f - receivedPan));
        right += value * std::sqrt (0.5f * (1.0f + receivedPan));

        voice.phase += frequency * phaseScale;
        if (voice.phase >= juce::MathConstants<float>::twoPi)
            voice.phase -= juce::MathConstants<float>::twoPi;
        voice.vibratoPhase += voice.vibratoRate * phaseScale;
        if (voice.vibratoPhase >= juce::MathConstants<float>::twoPi)
            voice.vibratoPhase -= juce::MathConstants<float>::twoPi;
        ++voice.age;
    }
}

void BeneathEngine::spawnRainDrop (const Parameters& p) noexcept
{
    for (auto& drop : rainDrops)
    {
        if (drop.active) continue;
        drop.active = true;
        drop.age = 0;
        drop.phase = random01() * juce::MathConstants<float>::twoPi;
        drop.sourceX = random01();

        const auto smallDrop = random01() < 0.45f - 0.15f * p.rain;
        const auto character = random01();
        const auto requestedResonanceHz = smallDrop
            ? 7500.0f + 1000.0f * character
            : 1800.0f + 3900.0f * std::pow (character, 0.72f);
        const auto resonanceHz = juce::jmin (requestedResonanceHz,
            static_cast<float> (sampleRate) * 0.42f);
        drop.highness = juce::jlimit (0.0f, 1.0f, (resonanceHz - 2200.0f) / 12800.0f);
        drop.targetPhaseDelta = resonanceHz * juce::MathConstants<float>::twoPi
                              / static_cast<float> (sampleRate);
        drop.phaseDelta = drop.targetPhaseDelta * (1.08f + 0.17f * random01());
        drop.glideCoefficient = 0.0065f + 0.0065f * random01();
        const auto bubbleTime = smallDrop
            ? 0.005f + 0.012f * random01()
            : 0.012f + 0.034f * random01();
        drop.bubbleDecay = std::exp (-1.0f / static_cast<float> (sampleRate * bubbleTime));
        drop.impactDecay = std::exp (-1.0f / static_cast<float> (sampleRate
            * (0.00038f + 0.00115f * (1.0f - drop.highness))));
        drop.bubbleEnvelope = 1.0f;
        drop.impactEnvelope = 1.0f;
        drop.impactLowpass = 0.0f;
        const auto impactHighpassHz = 1200.0f + 2300.0f * drop.highness;
        drop.impactLowpassCoefficient = 1.0f - std::exp (
            -juce::MathConstants<float>::twoPi * impactHighpassHz
            / static_cast<float> (sampleRate));
        const auto sizeGain = smallDrop ? 0.30f : 0.62f + 0.38f * character;
        drop.gain = 0.0085f * sizeGain * (0.68f + 0.58f * random01());
        drop.impactGain = drop.gain * (smallDrop ? 0.18f : 0.42f);
        drop.duration = static_cast<int> (sampleRate * bubbleTime * 8.5f);
        return;
    }
}

void BeneathEngine::renderRain (float& left, float& right, const Parameters& p,
                                float hydrophoneX) noexcept
{
    const auto volume = juce::jlimit (0.0f, 1.0f, p.rainVolume);
    for (auto& drop : rainDrops)
    {
        if (! drop.active) continue;
        const auto attackSamples = static_cast<float> (sampleRate)
                                 * (0.00035f + 0.00055f * (1.0f - drop.highness));
        const auto bubbleAttack = juce::jmin (1.0f,
            static_cast<float> (drop.age) / juce::jmax (1.0f, attackSamples));
        const auto white = randomSigned();
        drop.impactLowpass += drop.impactLowpassCoefficient
                            * (white - drop.impactLowpass);
        const auto impact = (white - drop.impactLowpass)
                          * drop.impactEnvelope * drop.impactGain;
        const auto bubble = std::sin (drop.phase) * bubbleAttack
                          * drop.bubbleEnvelope * drop.gain;
        const auto depthHighLoss = 1.0f - p.hydrophoneDepth
            * (0.42f + 0.44f * drop.highness);
        const auto value = (impact + bubble) * depthHighLoss * volume;
        const auto pan = juce::jlimit (-0.90f, 0.90f,
            (drop.sourceX - hydrophoneX) * 1.45f);
        left += value * std::sqrt (0.5f * (1.0f - pan));
        right += value * std::sqrt (0.5f * (1.0f + pan));

        drop.phase += drop.phaseDelta;
        if (drop.phase >= juce::MathConstants<float>::twoPi)
            drop.phase -= juce::MathConstants<float>::twoPi;
        drop.phaseDelta += (drop.targetPhaseDelta - drop.phaseDelta)
                         * drop.glideCoefficient;
        drop.bubbleEnvelope *= drop.bubbleDecay;
        drop.impactEnvelope *= drop.impactDecay;
        if (++drop.age >= drop.duration || drop.bubbleEnvelope < 0.00025f)
            drop.active = false;
    }

    const auto bedPosition = juce::jlimit (0.0f, 1.0f, (p.rain - 0.35f) / 0.65f);
    const auto bedAmount = bedPosition * bedPosition * (3.0f - 2.0f * bedPosition);
    if (bedAmount > 0.0f)
    {
        constexpr float highpassHz = 1350.0f;
        const auto lowpassHz = juce::jmap (p.hydrophoneDepth, 14500.0f, 4300.0f);
        const auto highpassCoefficient = 1.0f - std::exp (
            -juce::MathConstants<float>::twoPi * highpassHz / static_cast<float> (sampleRate));
        const auto lowpassCoefficient = 1.0f - std::exp (
            -juce::MathConstants<float>::twoPi * lowpassHz / static_cast<float> (sampleRate));
        for (int channel = 0; channel < 2; ++channel)
        {
            const auto white = randomSigned();
            auto& lowCutState = rainBedHighpassState[static_cast<size_t> (channel)];
            lowCutState += highpassCoefficient * (white - lowCutState);
            const auto highPassed = white - lowCutState;
            auto& highCutState = rainBedLowpassState[static_cast<size_t> (channel)];
            highCutState += lowpassCoefficient * (highPassed - highCutState);
            const auto motionGain = 0.78f + 0.44f * rainBedMotion;
            const auto bed = highCutState * bedAmount * motionGain * volume * 0.030f
                           * (1.0f - 0.28f * p.hydrophoneDepth);
            if (channel == 0) left += bed; else right += bed;
        }
    }
}

void BeneathEngine::spawnBubble (const Parameters& p, float proximityGain, float pan) noexcept
{
    for (auto& bubble : bubbles)
    {
        if (bubble.active) continue;
        bubble.active = true;
        bubble.phase = 0.0f;
        const auto deepBlorpChance = juce::jlimit (0.0f, 0.78f,
            juce::jmap (p.bubbleRate, 0.08f, 0.42f) + (1.0f - p.pump) * 0.24f);
        const auto deepBlorp = random01() < deepBlorpChance;
        const auto character = random01();
        const auto frequency = deepBlorp
            ? juce::jmap (character, 48.0f, 126.0f)
            : juce::jmap (character * character, 880.0f, 220.0f);
        bubble.targetPhaseDelta = frequency * juce::MathConstants<float>::twoPi
                                / static_cast<float> (sampleRate);
        bubble.phaseDelta = bubble.targetPhaseDelta
                          * (deepBlorp ? 0.34f : 0.46f + 0.10f * random01());
        bubble.envelope = 1.0f;
        const auto duration = deepBlorp
            ? juce::jmap (character, 0.070f, 0.18f)
            : juce::jmap (character, 0.022f, 0.072f);
        bubble.decay = std::exp (-1.0f / static_cast<float> (sampleRate * duration));
        bubble.glideCoefficient = deepBlorp ? 0.00075f : 0.00155f;
        bubble.attackSamples = static_cast<int> (sampleRate
            * (deepBlorp ? 0.0045f : 0.0022f));
        const auto typeGain = deepBlorp ? 1.18f : 0.60f;
        bubble.gain = (0.012f + 0.055f * p.pump) * proximityGain * typeGain
                    * (0.7f + 0.6f * random01());
        bubble.pan = juce::jlimit (-0.95f, 0.95f, pan + randomSigned() * 0.12f);
        bubble.age = 0;
        return;
    }
}

void BeneathEngine::renderBubbles (float& left, float& right) noexcept
{
    for (auto& bubble : bubbles)
    {
        if (! bubble.active) continue;
        const auto attackPosition = juce::jmin (1.0f,
            static_cast<float> (bubble.age)
            / static_cast<float> (juce::jmax (1, bubble.attackSamples)));
        const auto attack = attackPosition * attackPosition * (3.0f - 2.0f * attackPosition);
        const auto resonant = std::sin (bubble.phase)
                            + 0.075f * std::sin (bubble.phase * 1.37f);
        const auto value = resonant * attack * bubble.envelope * bubble.gain;
        left += value * std::sqrt (0.5f * (1.0f - bubble.pan));
        right += value * std::sqrt (0.5f * (1.0f + bubble.pan));
        bubble.phase += bubble.phaseDelta;
        bubble.phaseDelta += (bubble.targetPhaseDelta - bubble.phaseDelta)
                           * bubble.glideCoefficient;
        bubble.envelope *= bubble.decay;
        ++bubble.age;
        if (bubble.envelope < 0.0005f) bubble.active = false;
    }
}

float BeneathEngine::random01() noexcept
{
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return static_cast<float> (randomState & 0x00ffffffu) / static_cast<float> (0x01000000u);
}

