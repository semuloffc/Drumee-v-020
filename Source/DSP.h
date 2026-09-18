#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <functional>

static constexpr int kNumTracks = 5;
static constexpr int kNumSteps = 16;
static constexpr int kMaxVoicesPerTrack = 4;

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
    void start(const juce::AudioBuffer<float>* buffer, double sourceSampleRate,
                double outputSampleRate, float pitchSemitones, float gain,
                float attackMs, float decayMs, float sustainLevel, float releaseMs,
                int startDelaySamples);
    void renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples);
    bool active() const { return isActive; }

private:
    double envelopeGainAt(double elapsedSamples) const;

    const juce::AudioBuffer<float>* sourceBuffer = nullptr;
    bool isActive = false;
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
                 int startDelaySamples);
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
