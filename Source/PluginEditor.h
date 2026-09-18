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
    void toggleSampleEditor(bool show, int trackIndexToShow = -1);
    void highlightEditedSample(int trackIndex);

    DrumeeAudioProcessor& processor;
    DrumeeLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label versionLabel;
    juce::ComboBox presetBox;
    IconButton saveButton { "Save preset", IconButton::Icon::Save };
    IconButton newButton { "New pattern", IconButton::Icon::New };
    juce::TextButton editSamplesButton { "Edit Samples" };

    juce::Label sectionTiming { {}, "TIMING & GROOVE" };
    juce::Label sectionChaos { {}, "RATCHET & CHAOS" };
    juce::Label sectionSamples { {}, "SAMPLES" };
    juce::Label sectionMaster { {}, "MASTER" };

    // Card panels drawn behind the Timing/Chaos knob groups, giving those
    // columns the same panel treatment as the step grid and sample cards
    // now that they have the extra vertical room PITCH & SOUND left behind.
    juce::Rectangle<int> timingCardBounds;
    juce::Rectangle<int> chaosCardBounds;

    std::vector<std::unique_ptr<Encoder>> encoders;
    std::vector<std::unique_ptr<Encoder>> masterEncoders;
    std::unique_ptr<StepSequencerVisualizer> visualizer;
    std::vector<std::unique_ptr<SampleSlotComponent>> sampleSlots;

    // Internal view swapped in over the step sequencer's own bounds while
    // editing samples - replaces the old per-sample OS-level windows.
    std::unique_ptr<SampleEditorPanel> sampleEditorPanel;
    bool isEditingSamples = false;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumeeAudioProcessorEditor)
};
