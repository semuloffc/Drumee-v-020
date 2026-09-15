#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI.h"

class DrumeeAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit DrumeeAudioProcessorEditor(DrumeeAudioProcessor&);
    ~DrumeeAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void refreshPresetList();
    void savePresetDialog();
    void loadSample(int trackIndex);
    void refreshAllSampleSlots();
    void selectPresetInBox(const juce::String& name);

    DrumeeAudioProcessor& processor;
    DrumeeLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label versionLabel;
    juce::ComboBox presetBox;
    juce::TextButton saveButton { "Save" };
    juce::TextButton newButton { "New" };

    juce::Label sectionTiming { {}, "TIMING & GROOVE" };
    juce::Label sectionPitch { {}, "PITCH & SOUND" };
    juce::Label sectionChaos { {}, "RATCHET & CHAOS" };

    std::vector<std::unique_ptr<Encoder>> encoders;
    std::unique_ptr<StepSequencerVisualizer> visualizer;
    std::vector<std::unique_ptr<SampleSlotComponent>> sampleSlots;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumeeAudioProcessorEditor)
};
