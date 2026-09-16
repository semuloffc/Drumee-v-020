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
    void openSampleWindow(int trackIndex);

    DrumeeAudioProcessor& processor;
    DrumeeLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label versionLabel;
    juce::ComboBox presetBox;
    juce::TextButton saveButton { "Save" };
    juce::TextButton newButton { "New" };

    juce::Label sectionTiming { {}, "TIMING & GROOVE" };
    juce::Label sectionChaos { {}, "RATCHET & CHAOS" };
    juce::Label sectionSamples { {}, "SAMPLES" };

    // Card panels drawn behind the Timing/Chaos knob groups, giving those
    // columns the same panel treatment as the step grid and sample cards
    // now that they have the extra vertical room PITCH & SOUND left behind.
    juce::Rectangle<int> timingCardBounds;
    juce::Rectangle<int> chaosCardBounds;

    std::vector<std::unique_ptr<Encoder>> encoders;
    std::unique_ptr<StepSequencerVisualizer> visualizer;
    std::vector<std::unique_ptr<SampleSlotComponent>> sampleSlots;

    // One independent, persistent settings window per sample - each holds
    // that sample's own unique PITCH & SOUND controls.
    std::array<std::unique_ptr<SampleEditorWindow>, kNumTracks> sampleWindows;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumeeAudioProcessorEditor)
};
