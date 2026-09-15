#pragma once
#include <JuceHeader.h>
#include "DSP.h"

class DrumeeAudioProcessor;

class PresetManager
{
public:
    explicit PresetManager(DrumeeAudioProcessor& processorToUse);

    static juce::File getPresetFolder();

    bool savePreset(const juce::String& presetName);
    bool loadPreset(const juce::String& presetName);
    juce::StringArray getAllPresetNames() const;
    juce::String getCurrentPresetName() const { return currentPresetName; }

private:
    DrumeeAudioProcessor& processor;
    juce::String currentPresetName = "Init";
};
