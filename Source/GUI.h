#pragma once
#include <JuceHeader.h>
#include "DSP.h"
#include "Parameters.h"

// Flat, Kilohearts-inspired palette: one calm neutral base with three
// deliberately harmonised accents (same saturation/lightness band, spaced
// evenly around the colour wheel) so any combination of them reads as one
// coherent family rather than clashing hues.
namespace DrumeeColours
{
    static const juce::Colour background   { 0xFF1B1C20 };   // app background, flat
    static const juce::Colour panel        { 0xFF212228 };   // cards / bars
    static const juce::Colour panelAlt     { 0xFF282A31 };   // recessed wells (empty cells, track fields)
    static const juce::Colour outline      { 0xFF383A44 };   // hairline borders
    static const juce::Colour secondary    { 0xFF9DA3AF };   // kept for outline/legacy use

    static const juce::Colour accent1      { 0xFF49C2B4 };   // teal   — Timing / Groove
    static const juce::Colour accent2      { 0xFFE0748A };   // coral  — Ratchet / Chaos
    static const juce::Colour accent3      { 0xFFE8B24F };   // amber  — Pitch / Sound

    static const juce::Colour textPrimary  { 0xFFF2F3F5 };
    static const juce::Colour textSecondary{ 0xFF9DA3AF };
    static const juce::Colour textMuted    { 0xFF636873 };
    static const juce::Colour textInverse  { 0xFF14151A };

    inline juce::Colour forGroup(AccentGroup group)
    {
        switch (group)
        {
            case AccentGroup::timing:       return accent1;
            case AccentGroup::pitchSound:   return accent3;
            case AccentGroup::ratchetChaos: return accent2;
        }
        return accent1;
    }

    // Five track colours built from the same three-hue family so the step
    // grid stays legible without introducing a fourth/fifth clashing hue.
    inline juce::Colour forTrack(int trackIndex)
    {
        static const juce::Colour trackColours[5] = {
            accent1, accent3, accent2, accent1.withRotatedHue(0.03f).brighter(0.12f), accent3.darker(0.18f)
        };
        return trackColours[juce::jlimit(0, 4, trackIndex)];
    }
}

class DrumeeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DrumeeLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont(juce::Label&) override;
};

class Encoder : public juce::Component
{
public:
    Encoder(juce::AudioProcessorValueTreeState& state, const ParamInfo& info);

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    juce::Slider slider;
    juce::Label nameLabel;
    juce::Colour accentColour;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class StepSequencerVisualizer : public juce::Component, private juce::Timer
{
public:
    explicit StepSequencerVisualizer(Sequencer& sequencerToUse);
    ~StepSequencerVisualizer() override;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<float> getCellBounds(int track, int step) const;

    Sequencer& sequencer;
    int lastPaintedStep = -1;
};

class SampleSlotComponent : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    // compact = the small card used in the main window's bottom row.
    // The larger (non-compact) layout is used inside a sample's own window,
    // where there is room to show the file status on its own line.
    SampleSlotComponent(int trackIndex, SampleTrack& trackToUse, bool compact = true);

    void resized() override;
    void paint(juce::Graphics&) override;
    void refresh();

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    std::function<void(int)> onLoadRequested;
    std::function<void(int, const juce::File&)> onFileDropped;
    std::function<void(int)> onEditRequested;   // main-screen card only

private:
    bool isAcceptableFile(const juce::File& file) const;

    int index;
    SampleTrack& track;
    bool isCompact;
    juce::TextButton loadButton;
    juce::TextButton editButton;
    juce::Label nameLabel;
    juce::Label statusLabel;
    bool isDragHover = false;
};

// ---------------------------------------------------------------------------
// Sample window: one independent, per-sample settings window holding that
// sample's file slot plus its own unique PITCH & SOUND controls (Pitch Rand,
// Decay, Velocity, Volume). Opened from the main screen's sample card.
// ---------------------------------------------------------------------------
class SampleEditorContent : public juce::Component
{
public:
    SampleEditorContent(int trackIndex, juce::AudioProcessorValueTreeState& state, SampleTrack& trackToUse);

    void resized() override;
    void paint(juce::Graphics&) override;
    void refresh();

    std::function<void(int)> onLoadRequested;
    std::function<void(int, const juce::File&)> onFileDropped;

private:
    juce::Label sampleNameLabel;
    juce::Label sectionPitch { {}, "PITCH & SOUND" };
    std::unique_ptr<SampleSlotComponent> slot;
    std::vector<std::unique_ptr<Encoder>> encoders;
};

class SampleEditorWindow : public juce::DocumentWindow
{
public:
    // lookAndFeel must outlive this window (owned by the main editor) - a
    // DocumentWindow is a separate top-level window and does not inherit
    // the editor's LookAndFeel automatically, so it is applied explicitly
    // here to keep the sample window on the same palette/typography.
    SampleEditorWindow(int trackIndex, juce::AudioProcessorValueTreeState& state,
                        SampleTrack& trackToUse, juce::LookAndFeel& lookAndFeelToUse);
    ~SampleEditorWindow() override;

    void closeButtonPressed() override;
    SampleEditorContent& getContent() { return *content; }

private:
    SampleEditorContent* content = nullptr;
};
