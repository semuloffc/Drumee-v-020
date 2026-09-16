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
    void start(const juce::AudioBuffer<float>* buffer, double sourceSampleRate,
                double outputSampleRate, float pitchSemitones, float gain, float decayMs,
                int startDelaySamples);
    void renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples);
    bool active() const { return isActive; }

private:
    const juce::AudioBuffer<float>* sourceBuffer = nullptr;
    bool isActive = false;
    double position = 0.0;
    double ratio = 1.0;
    float gainLevel = 1.0f;
    double envelope = 1.0f;
    double envelopeDecayPerSample = 0.0;
    int delaySamples = 0;
};

class SampleTrack
{
public:
    juce::String name;
    juce::File sourceFile;
    juce::AudioBuffer<float> buffer;
    double sourceSampleRate = 44100.0;
    bool loaded = false;

    bool loadFile(const juce::File& file, juce::AudioFormatManager& formatManager);
    void trigger(double outputSampleRate, float pitchSemitones, float velocityGain, float decayMs, int startDelaySamples);
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
    // setting per sample, configured in that sample's own window) instead
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
