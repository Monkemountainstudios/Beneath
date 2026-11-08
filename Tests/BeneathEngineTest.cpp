#include <JuceHeader.h>
#include "BeneathEngine.h"
#include <iostream>

namespace
{
bool finiteAndAudible (const juce::AudioBuffer<float>& buffer, double& energy)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto x = buffer.getSample (channel, sample);
            if (! std::isfinite (x)) return false;
            energy += static_cast<double> (x) * x;
        }
    return true;
}
}

int main()
{
    const auto calmFlag = BeneathEngine::pirateLoopAmountForControl (0.85f);
    const auto warningFlag = BeneathEngine::pirateLoopAmountForControl (0.875f);
    const auto pazuzuFlag = BeneathEngine::pirateLoopAmountForControl (0.90f);
    if (std::abs (calmFlag - 0.85f) > 0.0001f
        || warningFlag <= 0.875f || warningFlag >= 1.0f
        || std::abs (pazuzuFlag - 1.0f) > 0.0001f)
    {
        std::cerr << "pirate feedback chaos curve is outside its intended bounds\n";
        return 16;
    }

    const auto bottomCeiling = BeneathEngine::waveCeilingForDepth (1.0f);
    const auto oneCentimetreLift = BeneathEngine::waveCeilingForDepth (0.99f);
    const auto middleCeiling = BeneathEngine::waveCeilingForDepth (0.5f);
    const auto surfaceCeiling = BeneathEngine::waveCeilingForDepth (0.0f);
    if (bottomCeiling < 89.0f || bottomCeiling > 91.0f
        || oneCentimetreLift > 91.0f
        || middleCeiling < 950.0f || middleCeiling > 1120.0f
        || surfaceCeiling < 11950.0f || surfaceCeiling > 12050.0f)
    {
        std::cerr << "wave-depth ceiling curve is outside its audition bounds\n";
        return 8;
    }

    BeneathEngine engine;
    engine.prepare (48000.0, 256);
    BeneathEngine::Parameters p;
    p.seabed = BeneathEngine::Seabed::rock;
    p.waves = 0.7f;
    p.shore = 0.45f;
    p.dream = 0.55f;

    const auto geometry = engine.calculateGeometry (p);
    if (geometry.pathMetres[0] <= 0.0f
        || geometry.pathMetres[1] <= geometry.pathMetres[0]
        || geometry.pathMetres[2] <= 0.0f)
    {
        std::cerr << "invalid acoustic geometry\n";
        return 1;
    }

    juce::AudioBuffer<float> input (2, 256), output;
    double phase = 0.0, energy = 0.0;
    for (int block = 0; block < 600; ++block)
    {
        for (int sample = 0; sample < input.getNumSamples(); ++sample)
        {
            const auto x = static_cast<float> (0.2 * std::sin (phase));
            phase += juce::MathConstants<double>::twoPi * 220.0 / 48000.0;
            input.setSample (0, sample, x);
            input.setSample (1, sample, x * 0.8f);
        }
        engine.process (input, output, p);
        if (! finiteAndAudible (output, energy))
        {
            std::cerr << "non-finite output\n";
            return 2;
        }
    }
    if (energy < 0.01)
    {
        std::cerr << "input path produced no audible output\n";
        return 3;
    }

    p.current = 1.0f;
    p.waves = 0.0f;
    for (int block = 0; block < 240; ++block)
        engine.process (input, output, p);
    const auto motion = engine.getMotion();
    if (std::abs (motion.hydrophoneHorizontal) < 0.001f
        || std::abs (motion.hydrophoneHorizontal - motion.sourceHorizontal) < 0.001f
        || motion.currentEbb < 0.05f)
    {
        std::cerr << "current did not produce distinct physical transducer motion\n";
        return 9;
    }

    BeneathEngine whaleEngine;
    whaleEngine.prepare (48000.0, 256);
    BeneathEngine::Parameters whaleParameters;
    whaleParameters.waves = 0.0f;
    whaleParameters.pump = 0.0f;
    whaleParameters.whales = 1.0f;
    whaleParameters.distanceReverb = 0.65f;
    input.clear();
    double whaleEnergy = 0.0;
    for (int block = 0; block < 900; ++block)
    {
        whaleEngine.process (input, output, whaleParameters);
        if (! finiteAndAudible (output, whaleEnergy)) return 10;
    }
    if (whaleEnergy < 0.001)
    {
        std::cerr << "procedural whale calls produced no audible output\n";
        return 11;
    }

    BeneathEngine rainEngine;
    rainEngine.prepare (48000.0, 256);
    BeneathEngine::Parameters rainParameters;
    rainParameters.waves = 0.0f;
    rainParameters.pump = 0.0f;
    rainParameters.whales = 0.0f;
    rainParameters.rain = 1.0f;
    rainParameters.rainVolume = 1.0f;
    rainParameters.distanceReverb = 0.35f;
    double rainEnergy = 0.0;
    for (int block = 0; block < 900; ++block)
    {
        rainEngine.process (input, output, rainParameters);
        if (! finiteAndAudible (output, rainEnergy)) return 12;
    }
    if (rainEnergy < 0.001)
    {
        std::cerr << "procedural rain produced no audible output\n";
        return 13;
    }

    const auto highFrequencyEnergy = [] (float distance)
    {
        BeneathEngine distanceEngine;
        distanceEngine.prepare (48000.0, 256);
        BeneathEngine::Parameters distanceParameters;
        distanceParameters.distance = distance;
        distanceParameters.seabed = BeneathEngine::Seabed::rock;
        distanceParameters.waterDepth = 1.0f;
        distanceParameters.sourceDepth = 1.0f;
        distanceParameters.hydrophoneDepth = 1.0f;
        distanceParameters.waves = 0.0f;
        distanceParameters.distanceReverb = 0.0f;
        juce::AudioBuffer<float> source (2, 256), result;
        double oscillatorPhase = 0.0, measured = 0.0;
        for (int block = 0; block < 700; ++block)
        {
            for (int sample = 0; sample < source.getNumSamples(); ++sample)
            {
                const auto x = static_cast<float> (0.15 * std::sin (oscillatorPhase));
                oscillatorPhase += juce::MathConstants<double>::twoPi * 5000.0 / 48000.0;
                source.setSample (0, sample, x);
                source.setSample (1, sample, x);
            }
            distanceEngine.process (source, result, distanceParameters);
            if (block > 400)
                finiteAndAudible (result, measured);
        }
        return measured;
    };

    const auto nearHighEnergy = highFrequencyEnergy (0.0f);
    const auto farDeepHighEnergy = highFrequencyEnergy (1.0f);
    if (farDeepHighEnergy >= nearHighEnergy * 0.08)
    {
        std::cerr << "maximum distance and depth were not sufficiently dark\n";
        return 7;
    }

    engine.reset();
    input.clear();
    p.waves = 0.0f;
    p.shore = 0.0f;
    p.pump = 0.9f;
    p.airStonePosition = 0.0f;
    double farBubbleEnergy = 0.0;
    for (int block = 0; block < 300; ++block)
    {
        engine.process (input, output, p);
        if (! finiteAndAudible (output, farBubbleEnergy)) return 4;
    }

    engine.reset();
    p.airStonePosition = 1.0f;
    double nearBubbleEnergy = 0.0;
    for (int block = 0; block < 300; ++block)
    {
        engine.process (input, output, p);
        if (! finiteAndAudible (output, nearBubbleEnergy)) return 4;
    }
    if (nearBubbleEnergy < 0.0001)
    {
        std::cerr << "bubble pump produced no sound\n";
        return 5;
    }
    if (nearBubbleEnergy <= farBubbleEnergy * 2.0)
    {
        std::cerr << "air stone proximity did not affect bubble level\n";
        return 6;
    }

    BeneathEngine pirateEngine;
    pirateEngine.prepare (48000.0, 256);
    BeneathEngine::Parameters pirateParameters;
    pirateParameters.pirateFeedback = 1.0f;
    pirateParameters.distance = 0.12f;
    pirateParameters.waves = 0.0f;
    pirateParameters.pump = 0.0f;
    pirateParameters.whales = 0.0f;
    pirateParameters.rain = 0.0f;
    pirateParameters.distanceReverb = 0.0f;
    double pirateEnergy = 0.0;
    float piratePeak = 0.0f;
    phase = 0.0;
    for (int block = 0; block < 1800; ++block)
    {
        for (int sample = 0; sample < input.getNumSamples(); ++sample)
        {
            const auto driven = block < 120
                ? static_cast<float> (0.9 * std::sin (phase)) : 0.0f;
            phase += juce::MathConstants<double>::twoPi * 740.0 / 48000.0;
            input.setSample (0, sample, driven);
            input.setSample (1, sample, driven * -0.83f);
        }
        pirateEngine.process (input, output, pirateParameters);
        if (! finiteAndAudible (output, pirateEnergy))
        {
            std::cerr << "maximum pirate feedback became non-finite\n";
            return 14;
        }
        for (int channel = 0; channel < output.getNumChannels(); ++channel)
            for (int sample = 0; sample < output.getNumSamples(); ++sample)
                piratePeak = juce::jmax (piratePeak,
                    std::abs (output.getSample (channel, sample)));
    }
    if (piratePeak > 1.01f || pirateEnergy < 0.01)
    {
        std::cerr << "pirate feedback safety ceiling or return energy failed\n";
        return 15;
    }

    std::cout << "Beneath DSP smoke test passed; near/far bubble energy="
              << nearBubbleEnergy / juce::jmax (1.0e-12, farBubbleEnergy) << "\n";
    return 0;
}

