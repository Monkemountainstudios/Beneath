#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class BeneathEngine
{
public:
    enum class Seabed : int { mud, sand, gravel, rock, coral };

    struct Parameters
    {
        float sourceDepth = 0.55f;
        float hydrophoneDepth = 0.48f;
        float sourceX = 0.24f;
        float hydrophoneX = 0.76f;
        float distance = 0.35f;
        float waterDepth = 0.45f;
        float shore = 0.0f;
        Seabed seabed = Seabed::sand;
        float waves = 0.25f;
        float waveVolume = 0.5f;
        float current = 0.15f;
        float pump = 0.0f;
        float bubbleRate = 0.5f;
        float bubbleSize = 0.5f;
        float airStonePosition = 0.5f;
        float distanceReverb = 0.55f;
        float whales = 0.0f;
        float whaleVolume = 0.375f;
        float rain = 0.0f;
        float rainVolume = 0.5f;
        float dream = 0.35f;
        float pirateFeedback = 0.0f;
        bool pauseEnvironment = false;
    };

    struct Geometry
    {
        float waterDepthMetres = 0.0f;
        float shoreDepthMetres = 0.0f;
        float sourceBottomDepthMetres = 0.0f;
        float hydrophoneBottomDepthMetres = 0.0f;
        float horizontalDistanceMetres = 0.0f;
        float sourceDepthMetres = 0.0f;
        float hydrophoneDepthMetres = 0.0f;
        std::array<float, 3> pathMetres {};
    };

    struct Motion
    {
        float sourceHorizontal = 0.0f;
        float sourceVertical = 0.0f;
        float hydrophoneHorizontal = 0.0f;
        float hydrophoneVertical = 0.0f;
        float currentEbb = 0.0f;
    };

    void prepare (double newSampleRate, int maximumBlockSize);
    void reset() noexcept;
    void process (const juce::AudioBuffer<float>& input,
                  juce::AudioBuffer<float>& wet,
                  const Parameters& parameters) noexcept;
    [[nodiscard]] Geometry calculateGeometry (const Parameters&) const noexcept;
    [[nodiscard]] Motion getMotion() const noexcept;
    [[nodiscard]] static float waveCeilingForDepth (float normalizedDepth) noexcept;

private:
    static constexpr int pathCount = 3;
    static constexpr int maxBubbles = 48;
    static constexpr int maxWhaleVoices = 32;
    static constexpr int maxRainDrops = 96;

    struct Bubble
    {
        bool active = false;
        float phase = 0.0f;
        float phaseDelta = 0.0f;
        float targetPhaseDelta = 0.0f;
        float envelope = 0.0f;
        float decay = 0.999f;
        float glideCoefficient = 0.0015f;
        float gain = 0.0f;
        float pan = 0.0f;
        int attackSamples = 32;
        int age = 0;
    };

    struct WhaleVoice
    {
        bool active = false;
        int age = 0;
        int startDelay = 0;
        int duration = 1;
        float phase = 0.0f;
        float vibratoPhase = 0.0f;
        float startHz = 120.0f;
        float endHz = 240.0f;
        float vibratoRate = 1.0f;
        float bend = 0.0f;
        float waveBend = 0.16f;
        float harmonic = 0.05f;
        float harmonic3 = 0.0f;
        float harmonic4 = 0.0f;
        float swayDepth = 0.0f;
        float swayCycles = 1.0f;
        float swayPhase = 0.0f;
        bool terminalRise = false;
        float terminalRiseSemitones = 0.0f;
        float distantX = -0.8f;
        float gain = 0.0f;
        float pan = 0.0f;
    };

    struct RainDrop
    {
        bool active = false;
        int age = 0;
        int duration = 1;
        float phase = 0.0f;
        float phaseDelta = 0.0f;
        float targetPhaseDelta = 0.0f;
        float glideCoefficient = 0.007f;
        float bubbleDecay = 0.999f;
        float impactDecay = 0.95f;
        float bubbleEnvelope = 1.0f;
        float impactEnvelope = 1.0f;
        float impactLowpass = 0.0f;
        float impactLowpassCoefficient = 0.2f;
        float gain = 0.0f;
        float impactGain = 0.0f;
        float sourceX = 0.5f;
        float highness = 0.5f;
    };

    [[nodiscard]] float readDelay (int channel, float delaySamples) const noexcept;
    [[nodiscard]] float lowpass (int path, int channel, float input, float cutoff) noexcept;
    void spawnBubble (const Parameters&, float proximityGain, float pan) noexcept;
    void renderBubbles (float& left, float& right) noexcept;
    void spawnWhaleCall (const Parameters&) noexcept;
    void renderWhales (float& left, float& right, float volume,
                       float hydrophoneX, float hydrophoneDepth) noexcept;
    void spawnRainDrop (const Parameters&) noexcept;
    void renderRain (float& left, float& right, const Parameters&,
                     float hydrophoneX) noexcept;
    [[nodiscard]] float random01() noexcept;
    [[nodiscard]] float randomSigned() noexcept { return random01() * 2.0f - 1.0f; }

    juce::AudioBuffer<float> delayBuffer;
    juce::AudioBuffer<float> diffuseBuffer;
    juce::Reverb diffuseReverb;
    std::array<juce::SmoothedValue<float>, pathCount> pathDelaySamples;
    std::array<std::array<float, 2>, pathCount> filterState {};
    std::array<Bubble, maxBubbles> bubbles {};
    std::array<WhaleVoice, maxWhaleVoices> whaleVoices {};
    std::array<RainDrop, maxRainDrops> rainDrops {};
    std::array<float, 2> surfFilterState {};
    std::array<std::array<float, 3>, 2> pinkNoiseState {};
    std::array<float, 2> rumbleLowpassState {};
    std::array<float, 2> reverbDarkState {};
    std::array<float, 2> feedbackInjection {};
    std::array<float, 2> pirateLowCutState {};
    std::array<float, 2> pirateHighCutState {};
    std::array<float, 2> rainBedHighpassState {};
    std::array<float, 2> rainBedLowpassState {};
    int writePosition = 0;
    int preparedBlockSize = 0;
    double sampleRate = 48000.0;
    double samplesUntilBubble = 1.0;
    double samplesUntilWave = 1.0;
    double samplesUntilWhale = 1.0;
    double samplesUntilRain = 1.0;
    double samplesUntilRainMotionChange = 1.0;
    float surfacePhase = 0.0f;
    float swellPhase = 0.0f;
    float currentPhase = 0.0f;
    float currentPhaseSecondary = 1.7f;
    float currentEbb = 0.62f;
    float currentEbbTarget = 0.62f;
    float currentDrift = 0.0f;
    float currentDriftTarget = 0.0f;
    double samplesUntilCurrentChange = 1.0;
    float rumblePhase = 0.8f;
    float rumblePhaseSecondary = 2.4f;
    float rainMotionPhase = 0.0f;
    float rainBedMotion = 0.5f;
    float rainBedMotionTarget = 0.5f;
    float rainMotionSlewSeconds = 1.8f;
    float surfEnvelope = 0.0f;
    float waveEventLevel = 0.0f;
    int waveAge = 0;
    int waveDuration = 1;
    int waveSequenceIndex = 0;
    bool waveActive = false;
    std::atomic<float> motionSourceHorizontal { 0.0f };
    std::atomic<float> motionSourceVertical { 0.0f };
    std::atomic<float> motionHydrophoneHorizontal { 0.0f };
    std::atomic<float> motionHydrophoneVertical { 0.0f };
    std::atomic<float> motionCurrentEbb { 0.0f };
    uint32_t randomState = 0x42454e45u;
};

