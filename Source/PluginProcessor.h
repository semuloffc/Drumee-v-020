#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include "DSP.h"
#include "PresetManager.h"

class DrumeeAudioProcessor : public juce::AudioProcessor
{
public:
    DrumeeAudioProcessor();
    ~DrumeeAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioFormatManager formatManager;

    Sequencer sequencer;
    std::array<SampleTrack, kNumTracks> tracks;
    PresetManager presetManager;
    MasterBus masterBus;

    bool loadSampleForTrack(int trackIndex, const juce::File& file);

    bool hostIsPlaying = false;
    double hostBpm = 120.0;

private:
    juce::AudioBuffer<float> scratchBuffer;

    // Ramped per-block, not stepped, so flipping Mute/Solo mid-playback
    // fades rather than clicks (see processBlock()).
    std::array<float, kNumTracks> trackMuteGainState { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumeeAudioProcessor)
};
