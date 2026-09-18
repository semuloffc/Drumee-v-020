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
            // Envelope encoders are recoloured per-track right after
            // construction (see forTrack()) so each sample's envelope
            // reads in its own colour - this is just the switch's fallback.
            case AccentGroup::envelope:     return accent3;
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

    // Per-track PITCH & SOUND knob accent, used inside the sample editor
    // panel. Reuses forTrack() for Kick/Clap/Perc (blue-ish teal / pink),
    // but Snare and FX keep the plain, un-darkened accent3 amber that every
    // sample's knobs used before this became per-track - i.e. "unchanged".
    inline juce::Colour forSampleKnob(int trackIndex)
    {
        if (trackIndex == 1 || trackIndex == 4)
            return accent3;
        return forTrack(trackIndex);
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

// Graphical (icon-only) button used where the old build had plain text
// buttons ("Save" / "New"). Drawn entirely in code so it stays on the same
// flat, glow-free palette as the rest of the UI - no new image assets.
class IconButton : public juce::Button
{
public:
    enum class Icon { Save, New, Back };

    IconButton(const juce::String& tooltipText, Icon iconToUse);

    void paintButton(juce::Graphics&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    Icon icon;
};

class Encoder : public juce::Component
{
public:
    Encoder(juce::AudioProcessorValueTreeState& state, const ParamInfo& info);

    void resized() override;
    void paint(juce::Graphics&) override;
    void setAccentColour(juce::Colour newColour);

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

class SampleSlotComponent : public juce::Component, public juce::FileDragAndDropTarget, public juce::SettableTooltipClient
{
public:
    // compact = the small card used in the main window's bottom row.
    // The larger (non-compact) layout is used inside the sample editor
    // panel, where there is room to show the file status on its own line.
    SampleSlotComponent(int trackIndex, SampleTrack& trackToUse, bool compact = true);

    void resized() override;
    void paint(juce::Graphics&) override;
    void refresh();
    void setSelected(bool shouldBeSelected);

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

    // compact card: click selects this sample for editing (opens/focuses
    // the sample editor panel on it). Non-compact (already inside that
    // panel, already the sample being edited): click loads a file - there
    // is no "select" ambiguity to resolve there.
    std::function<void(int)> onLoadRequested;
    std::function<void(int, const juce::File&)> onFileDropped;
    std::function<void(int)> onEditRequested;   // main-screen card only

private:
    bool isAcceptableFile(const juce::File& file) const;

    int index;
    SampleTrack& track;
    bool isCompact;
    juce::Label nameLabel;
    juce::Label statusLabel;
    bool isDragHover = false;
    bool isMouseOver = false;
    bool isSelected = false;
};

// ---------------------------------------------------------------------------
// Static peak waveform for one sample, drawn directly above that sample's
// envelope graph in its own track colour so the two form one connected
// visual block. Rebuilt whenever a new file is loaded into the track.
// ---------------------------------------------------------------------------
class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    WaveformDisplay(int trackIndex, SampleTrack& trackToUse);
    ~WaveformDisplay() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void refresh();

private:
    void timerCallback() override;
    void rebuildPeaks();

    SampleTrack& track;
    juce::Colour accentColour;
    std::vector<float> peakMin, peakMax;

    // Same trigger-detection/sweep approach as EnvelopeVisualizer: poll the
    // track's atomic hit counter and, on a new hit, sweep a playhead across
    // the full sample duration (not just the envelope's time) so the marker
    // tracks what is actually audible in the waveform above.
    int lastSeenTriggerCount = 0;
    double animationStartMs = 0.0;
    bool animating = false;
};

// ---------------------------------------------------------------------------
// Interactive ADSR envelope graph for one sample's page in the editor panel.
// Draws the Attack/Decay/Sustain/Release shape in that sample's own track
// colour, lets the person drag its three breakpoints directly (Serum-style)
// to reshape the envelope, and animates a small marker sweeping along the
// curve whenever the sequencer actually triggers that sample.
// ---------------------------------------------------------------------------
class EnvelopeVisualizer : public juce::Component, private juce::Timer, public juce::SettableTooltipClient
{
public:
    EnvelopeVisualizer(juce::AudioProcessorValueTreeState& state, int trackIndex, SampleTrack& trackToUse);
    ~EnvelopeVisualizer() override;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

private:
    // *Point targets drag the breakpoints (time/level). *Curve targets grab
    // the segment between two breakpoints anywhere along its length and bend
    // it, Serum-style - the classic "second layer" of ADSR editing on top of
    // the point drag that was already here.
    enum class DragTarget { none, attackPoint, decayPoint, releasePoint, attackCurve, decayCurve, releaseCurve };

    // Zone rectangles depend only on the component's bounds; the points
    // (p1..p4) depend on the current parameter values too, so they are
    // rebuilt together every time geometry is needed (paint, hit-testing).
    struct Geometry
    {
        juce::Rectangle<float> area, zoneAttack, zoneDecay, zoneSustain, zoneRelease;
        juce::Point<float> p0, p1, p2, p3, p4;
    };

    void timerCallback() override;
    Geometry buildGeometry() const;
    DragTarget hitTest(juce::Point<float> position, const Geometry&) const;
    DragTarget hitTestCurve(juce::Point<float> position, const Geometry&) const;
    juce::Point<float> pointAtElapsed(const Geometry&, double elapsedMs) const;
    void drawPoint(juce::Graphics&, juce::Point<float>, bool active) const;
    void drawValueChip(juce::Graphics&, const Geometry&) const;
    void setNormalisedValue(juce::RangedAudioParameter* param, float realValue) const;
    juce::Path buildCurvedShape(const Geometry&) const;

    float getAttack() const  { return attackRaw != nullptr ? attackRaw->load() : 0.0f; }
    float getDecay() const   { return decayRaw != nullptr ? decayRaw->load() : 0.0f; }
    float getSustain01() const { return sustainRaw != nullptr ? sustainRaw->load() / 100.0f : 0.0f; }
    float getRelease() const { return releaseRaw != nullptr ? releaseRaw->load() : 0.0f; }
    float getAttackCurve() const  { return attackCurveRaw != nullptr ? attackCurveRaw->load() : 0.0f; }
    float getDecayCurve() const   { return decayCurveRaw != nullptr ? decayCurveRaw->load() : 0.0f; }
    float getReleaseCurve() const { return releaseCurveRaw != nullptr ? releaseCurveRaw->load() : 0.0f; }

    SampleTrack& track;
    juce::Colour accentColour;

    juce::RangedAudioParameter* attackParam = nullptr;
    juce::RangedAudioParameter* decayParam = nullptr;
    juce::RangedAudioParameter* sustainParam = nullptr;
    juce::RangedAudioParameter* releaseParam = nullptr;
    juce::RangedAudioParameter* attackCurveParam = nullptr;
    juce::RangedAudioParameter* decayCurveParam = nullptr;
    juce::RangedAudioParameter* releaseCurveParam = nullptr;

    std::atomic<float>* attackRaw = nullptr;
    std::atomic<float>* decayRaw = nullptr;
    std::atomic<float>* sustainRaw = nullptr;
    std::atomic<float>* releaseRaw = nullptr;
    std::atomic<float>* attackCurveRaw = nullptr;
    std::atomic<float>* decayCurveRaw = nullptr;
    std::atomic<float>* releaseCurveRaw = nullptr;

    DragTarget dragging = DragTarget::none;
    DragTarget hovered = DragTarget::none;

    // Curve drags are relative: how far the mouse has moved from where the
    // drag started, added to whatever the curve value already was - not an
    // absolute mapping of mouse position - so grabbing a segment never
    // snaps it to a new shape the instant you click.
    juce::Point<float> curveDragStart;
    float curveDragStartValue = 0.0f;

    int lastSeenTriggerCount = 0;
    double animationStartMs = 0.0;
    bool animating = false;
};

// ---------------------------------------------------------------------------
// Sample editor page: one sample's file slot, its PITCH & SOUND controls
// (Pitch Rand, Velocity) and its ADSR ENVELOPE (Attack/Decay/Sustain/
// Release/Volume) with the interactive graph above. As of 0.2.3 this no
// longer lives in its own OS-level window - it is one page inside
// SampleEditorPanel, an internal view swapped in full-width over the step
// sequencer and side knob columns.
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
    juce::Label sectionPitch { {}, "PITCH & SOUND" };
    juce::Label sectionEnvelope { {}, "ENVELOPE" };
    std::unique_ptr<SampleSlotComponent> slot;
    std::vector<std::unique_ptr<Encoder>> pitchEncoders;
    std::vector<std::unique_ptr<Encoder>> envelopeEncoders;
    std::unique_ptr<WaveformDisplay> waveformDisplay;
    std::unique_ptr<EnvelopeVisualizer> envelopeVisualizer;

    // Recessed "well" backgrounds behind the two knob rows, matching the
    // panelAlt treatment used elsewhere - captured in resized() so paint()
    // can draw them without recomputing the whole layout.
    juce::Rectangle<int> pitchWellBounds;
    juce::Rectangle<int> envelopeKnobWellBounds;
};

// ---------------------------------------------------------------------------
// SampleEditorPanel: internal view that takes over the plugin's full width
// (step sequencer plus both side knob columns) while the user is editing
// samples, instead of opening a separate physical window. Holds one tab per
// track plus that track's SampleEditorContent page, and a Back button that
// returns to the sequencer.
// ---------------------------------------------------------------------------
class SampleEditorPanel : public juce::Component
{
public:
    SampleEditorPanel(juce::AudioProcessorValueTreeState& state, std::array<SampleTrack, kNumTracks>& tracksToUse);

    void resized() override;
    void paint(juce::Graphics&) override;
    void refresh();
    void showTrack(int trackIndex);
    int getCurrentTrack() const { return currentTrack; }

    std::function<void(int)> onLoadRequested;
    std::function<void(int, const juce::File&)> onFileDropped;
    std::function<void()> onCloseRequested;
    std::function<void(int)> onTrackChanged;   // fired whenever the selected tab changes

private:
    void updateTabColours();
    void updateTitle();

    int currentTrack = 0;
    std::array<juce::String, kNumTracks> trackNames;
    juce::Label titlePrefixLabel;
    juce::Label titleLabel;
    IconButton closeButton { "Back to sequencer", IconButton::Icon::Back };
    std::array<std::unique_ptr<juce::TextButton>, kNumTracks> tabButtons;
    std::array<std::unique_ptr<SampleEditorContent>, kNumTracks> pages;
};
