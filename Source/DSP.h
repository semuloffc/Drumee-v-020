#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include <functional>

static constexpr int kNumTracks = 5;
static constexpr int kNumSteps = 16;
static constexpr int kMaxVoicesPerTrack = 4;

// Serum-style envelope segment shaping: tension in [-1, 1], 0 = linear.
// Positive bends the segment below the straight line (slow start,
// accelerating toward the end - "convex"); negative bends it above (fast
// start, decelerating - "concave"). Shared by the DSP engine (actual
// audio envelope) and the GUI's interactive graph, so what gets dragged
// on screen is exactly what plays.
inline float envelopeTensionCurve(float x, float tension)
{
    x = juce::jlimit(0.0f, 1.0f, x);
    float k = juce::jlimit(-0.999f, 0.999f, tension);
    if (std::abs(k) < 1.0e-4f)
        return x;
    return (x - k * x) / (k - 2.0f * k * x + 1.0f);
}

struct StepData
{
    bool active[kNumTracks] = { false, false, false, false, false };
    float velocity[kNumTracks] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
};

class SamplePlayerVoice
{
public:
    // attackMs/decayMs/sustainLevel(0..1)/releaseMs shape a one-shot ADSR
    // amplitude envelope. There is no note-off in this drum sampler, so
    // Release is scheduled to land at the end of the sample's own playback
    // length (or right after Decay if the sample is too short for that).
    // reversed: 0.2.9, plays the source buffer from its end back to its
    // start instead of start to end. Timing/envelope maths is unaffected -
    // only the direction position advances each sample changes (see
    // renderNextBlock()).
    void start(const juce::AudioBuffer<float>* buffer, double sourceSampleRate,
                double outputSampleRate, float pitchSemitones, float gain,
                float attackMs, float decayMs, float sustainLevel, float releaseMs,
                float attackCurve, float decayCurve, float releaseCurve,
                int startDelaySamples, bool reversed);
    void renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples);
    bool active() const { return isActive; }

private:
    double envelopeGainAt(double elapsedSamples) const;

    const juce::AudioBuffer<float>* sourceBuffer = nullptr;
    bool isActive = false;
    bool reversedPlayback = false;
    double position = 0.0;
    double ratio = 1.0;
    float gainLevel = 1.0f;
    int delaySamples = 0;

    double envAttackSamples = 0.0;
    double envDecaySamples = 0.0;
    double envReleaseSamples = 0.0;
    double envSustainLevel = 0.0;
    double envReleaseStartSample = 0.0;
    double envElapsedSamples = 0.0;
    float envAttackCurve = 0.0f;
    float envDecayCurve = 0.0f;
    float envReleaseCurve = 0.0f;
};

class SampleTrack
{
public:
    juce::String name;
    juce::File sourceFile;
    juce::AudioBuffer<float> buffer;
    double sourceSampleRate = 44100.0;
    bool loaded = false;

    // Bumped on every trigger() call (audio thread) so the GUI's envelope
    // visualizer can detect hits and animate without any locking - the GUI
    // timer just polls this and compares against the value it last saw.
    std::atomic<int> triggerCount { 0 };

    bool loadFile(const juce::File& file, juce::AudioFormatManager& formatManager);
    void trigger(double outputSampleRate, float pitchSemitones, float velocityGain,
                 float attackMs, float decayMs, float sustainLevel, float releaseMs,
                 float attackCurve, float decayCurve, float releaseCurve,
                 int startDelaySamples, bool reversed);
    void renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples);

private:
    std::array<SamplePlayerVoice, kMaxVoicesPerTrack> voices;
    int nextVoice = 0;
};

class Sequencer
{
public:
    using TriggerCallback = std::function<void(int trackIndex, int sampleOffset, float velocity, float pitchOffsetSemitones)>;

    std::array<StepData, kNumSteps> pattern;
    std::atomic<int> currentStep { 0 };
    std::atomic<bool> stepFlash[kNumSteps] {};

    void prepare(double sampleRate);
    void reset();
    // pitchRandSt and velocityDepth are now per-track (one Pitch & Sound
    // setting per sample, configured on that sample's page in the editor panel) instead
    // of a single global value applied to every track.
    void process(int numSamples, double bpm, bool isPlaying,
                 float nudgeMs, float swingPct, float humanizePct,
                 const std::array<float, kNumTracks>& pitchRandSt,
                 const std::array<float, kNumTracks>& velocityDepth,
                 float ratchetAmount, float probabilityPct,
                 const TriggerCallback& callback);

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    bool wasPlaying = false;
    juce::Random random;

    void fireStep(int step, int offsetInBlock, double samplesPerStep,
                  float nudgeMs, float swingPct, float humanizePct,
                  const std::array<float, kNumTracks>& pitchRandSt,
                  const std::array<float, kNumTracks>& velocityDepth,
                  float ratchetAmount, float probabilityPct,
                  int blockSize, const TriggerCallback& callback);
};

// 0.2.8: post-mix master bus, run once after every track has been rendered
// and summed into the main output buffer. Applies overall output Volume,
// then a simple feed-forward peak limiter (fast attack, slower release, no
// lookahead) that keeps the mix from exceeding the Limiter ceiling.
class MasterBus
{
public:
    void prepare(double sampleRate);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, float volumeGain, float limiterCeilingDb);

private:
    double sampleRate = 44100.0;
    float previousVolumeGain = 1.0f;
    float limiterEnvelope = 1.0f;
};
