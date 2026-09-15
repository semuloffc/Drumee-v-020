#include "GUI.h"

DrumeeLookAndFeel::DrumeeLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, DrumeeColours::background);
    setColour(juce::Label::textColourId, DrumeeColours::textPrimary);
    setColour(juce::TextButton::buttonColourId, DrumeeColours::panel);
    setColour(juce::TextButton::textColourOffId, DrumeeColours::textPrimary);
    setColour(juce::TextButton::textColourOnId, DrumeeColours::textInverse);
    setColour(juce::ComboBox::backgroundColourId, DrumeeColours::panel);
    setColour(juce::ComboBox::textColourId, DrumeeColours::textPrimary);
    setColour(juce::ComboBox::outlineColourId, DrumeeColours::outline);
    setColour(juce::ComboBox::arrowColourId, DrumeeColours::textSecondary);
    setColour(juce::PopupMenu::backgroundColourId, DrumeeColours::panel);
    setColour(juce::PopupMenu::textColourId, DrumeeColours::textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, DrumeeColours::accent1.withAlpha(0.25f));
}

// Flat Kilohearts-style knob: solid dim track, solid accent value arc, a
// flat (non-gradient) cap and a single crisp indicator line. No glows,
// no gradients, no soft alpha halos - everything is drawn at full opacity
// so the control reads cleanly at any host scaling.
void DrumeeLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                          juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(6.0f);
    auto centre = bounds.getCentre();
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    juce::Colour fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
    const float trackThickness = 3.5f;

    juce::Path track;
    track.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(DrumeeColours::outline);
    g.strokePath(track, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour(fillColour);
    g.strokePath(valueArc, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    float knobRadius = radius * 0.6f;
    g.setColour(DrumeeColours::panelAlt);
    g.fillEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

    juce::Point<float> tip(centre.x + std::sin(angle) * knobRadius * 0.78f,
                            centre.y - std::cos(angle) * knobRadius * 0.78f);
    g.setColour(fillColour);
    g.drawLine({ centre, tip }, 2.5f);
    g.fillEllipse(centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);
}

void DrumeeLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                              bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    juce::Colour base = DrumeeColours::panel;
    if (isDown)
        base = DrumeeColours::panelAlt;
    else if (isHighlighted)
        base = base.brighter(0.10f);

    g.setColour(base);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(DrumeeColours::outline);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

juce::Font DrumeeLookAndFeel::getLabelFont(juce::Label& label)
{
    return juce::Font(label.getFont().getHeight(), juce::Font::plain);
}

Encoder::Encoder(juce::AudioProcessorValueTreeState& state, const ParamInfo& info)
{
    accentColour = DrumeeColours::forGroup(info.group);

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accentColour);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, DrumeeColours::outline);
    slider.setColour(juce::Slider::textBoxTextColourId, DrumeeColours::textSecondary);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    nameLabel.setText(info.label, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, DrumeeColours::textPrimary);
    nameLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    addAndMakeVisible(nameLabel);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state, info.id, slider);
}

void Encoder::resized()
{
    auto bounds = getLocalBounds();
    nameLabel.setBounds(bounds.removeFromTop(18));
    slider.setBounds(bounds);
}

void Encoder::paint(juce::Graphics&) {}

StepSequencerVisualizer::StepSequencerVisualizer(Sequencer& sequencerToUse) : sequencer(sequencerToUse)
{
    startTimerHz(30);
}

StepSequencerVisualizer::~StepSequencerVisualizer() { stopTimer(); }

juce::Rectangle<float> StepSequencerVisualizer::getCellBounds(int track, int step) const
{
    auto area = getLocalBounds().toFloat().reduced(14.0f);
    float rowHeight = area.getHeight() / (float) kNumTracks;
    float colWidth = area.getWidth() / (float) kNumSteps;
    return { area.getX() + (float) step * colWidth, area.getY() + (float) track * rowHeight, colWidth, rowHeight };
}

void StepSequencerVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(DrumeeColours::panel);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(DrumeeColours::outline);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

    int playStep = sequencer.currentStep.load();
    auto area = getLocalBounds().toFloat().reduced(14.0f);

    for (int step = 0; step < kNumSteps; ++step)
    {
        bool isPlayColumn = (step == playStep);

        if (isPlayColumn)
        {
            // Derived from the same `area` used by getCellBounds(), so the
            // playhead highlight always lines up exactly with the grid
            // beneath it instead of drifting with an unrelated magic number.
            auto colBounds = getCellBounds(0, step);
            colBounds.setY(area.getY());
            colBounds.setHeight(area.getHeight());
            g.setColour(DrumeeColours::accent1.withAlpha(0.12f));
            g.fillRect(colBounds);
        }

        for (int track = 0; track < kNumTracks; ++track)
        {
            auto cell = getCellBounds(track, step).reduced(3.0f);
            bool active = sequencer.pattern[(size_t) step].active[track];
            float vel = sequencer.pattern[(size_t) step].velocity[track];

            juce::Colour cellColour = active ? DrumeeColours::forTrack(track) : DrumeeColours::panelAlt;
            float alpha = active ? juce::jmap(vel, 0.5f, 1.0f) : 1.0f;
            float cornerSize = juce::jmin(cell.getWidth(), cell.getHeight()) * 0.3f;

            if (isPlayColumn && active)
            {
                g.setColour(cellColour);
                g.fillRoundedRectangle(cell.expanded(1.5f), cornerSize);
            }

            g.setColour(cellColour.withAlpha(alpha));
            g.fillRoundedRectangle(cell, cornerSize);

            if (! active)
            {
                g.setColour(DrumeeColours::outline);
                g.drawRoundedRectangle(cell, cornerSize, 1.0f);
            }
        }
    }
}

void StepSequencerVisualizer::mouseDown(const juce::MouseEvent& event)
{
    auto area = getLocalBounds().toFloat().reduced(14.0f);
    if (! area.contains(event.position))
        return;

    float rowHeight = area.getHeight() / (float) kNumTracks;
    float colWidth = area.getWidth() / (float) kNumSteps;

    int track = (int) ((event.position.y - area.getY()) / rowHeight);
    int step = (int) ((event.position.x - area.getX()) / colWidth);

    track = juce::jlimit(0, kNumTracks - 1, track);
    step = juce::jlimit(0, kNumSteps - 1, step);

    bool& active = sequencer.pattern[(size_t) step].active[track];
    active = ! active;
    if (active)
        sequencer.pattern[(size_t) step].velocity[track] = 0.85f;

    repaint();
}

void StepSequencerVisualizer::timerCallback()
{
    int step = sequencer.currentStep.load();
    if (step != lastPaintedStep)
    {
        lastPaintedStep = step;
        repaint();
    }
}

SampleSlotComponent::SampleSlotComponent(int trackIndex, SampleTrack& trackToUse)
    : index(trackIndex), track(trackToUse)
{
    nameLabel.setJustificationType(juce::Justification::centredLeft);
    nameLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, DrumeeColours::textPrimary);
    addAndMakeVisible(nameLabel);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setFont(juce::Font(11.0f, juce::Font::plain));
    statusLabel.setColour(juce::Label::textColourId, DrumeeColours::textMuted);
    addAndMakeVisible(statusLabel);

    loadButton.setButtonText("Load");
    loadButton.onClick = [this] { if (onLoadRequested) onLoadRequested(index); };
    addAndMakeVisible(loadButton);

    refresh();
}

void SampleSlotComponent::resized()
{
    auto bounds = getLocalBounds().reduced(8);
    loadButton.setBounds(bounds.removeFromRight(64).reduced(0, 8));
    bounds.removeFromRight(6);
    nameLabel.setBounds(bounds.removeFromTop(bounds.getHeight() / 2));
    statusLabel.setBounds(bounds);
}

void SampleSlotComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::Colour accent = DrumeeColours::forTrack(index);

    g.setColour(isDragHover ? DrumeeColours::panelAlt.brighter(0.08f) : DrumeeColours::panel);
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(isDragHover ? accent : DrumeeColours::outline);
    g.drawRoundedRectangle(bounds.reduced(0.75f), 5.0f, isDragHover ? 2.0f : 1.0f);

    // Flat accent tab on the left edge identifies the track colour even
    // when the slot is empty, matching the step-grid colour for that row.
    g.setColour(accent);
    g.fillRoundedRectangle(bounds.removeFromLeft(4.0f).reduced(0.0f, 6.0f), 2.0f);
}

void SampleSlotComponent::refresh()
{
    nameLabel.setText(track.name, juce::dontSendNotification);
    statusLabel.setText(track.loaded ? track.sourceFile.getFileName() : "Empty slot",
                         juce::dontSendNotification);
}

bool SampleSlotComponent::isAcceptableFile(const juce::File& file) const
{
    static const juce::StringArray extensions { ".wav", ".wave", ".aif", ".aiff", ".flac", ".ogg", ".mp3" };
    return extensions.contains(file.getFileExtension().toLowerCase());
}

bool SampleSlotComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& path : files)
        if (isAcceptableFile(juce::File(path)))
            return true;
    return false;
}

void SampleSlotComponent::fileDragEnter(const juce::StringArray&, int, int)
{
    isDragHover = true;
    repaint();
}

void SampleSlotComponent::fileDragExit(const juce::StringArray&)
{
    isDragHover = false;
    repaint();
}

void SampleSlotComponent::filesDropped(const juce::StringArray& files, int, int)
{
    isDragHover = false;

    for (auto& path : files)
    {
        juce::File file(path);
        if (isAcceptableFile(file) && file.existsAsFile())
        {
            if (onFileDropped)
                onFileDropped(index, file);
            break;
        }
    }

    repaint();
}
