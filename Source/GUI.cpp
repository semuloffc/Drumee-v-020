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

IconButton::IconButton(const juce::String& tooltipText, Icon iconToUse) : juce::Button(tooltipText), icon(iconToUse)
{
    setTooltip(tooltipText);
}

void IconButton::paintButton(juce::Graphics& g, bool isHighlighted, bool isDown)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    juce::Colour base = DrumeeColours::panel;
    if (isDown)
        base = DrumeeColours::panelAlt;
    else if (isHighlighted)
        base = base.brighter(0.10f);

    g.setColour(base);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(DrumeeColours::outline);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

    auto glyph = bounds.reduced(bounds.getWidth() * 0.28f, bounds.getHeight() * 0.28f);
    juce::Colour ink = isHighlighted || isDown ? DrumeeColours::textPrimary : DrumeeColours::textSecondary;
    g.setColour(ink);

    if (icon == Icon::Save)
    {
        // Flat floppy-disk glyph: outer body, a write-protect tab top-left
        // and a label strip - reads clearly at small toolbar sizes.
        juce::Path body;
        body.addRoundedRectangle(glyph, 1.5f);
        g.strokePath(body, juce::PathStrokeType(1.6f));

        auto tab = glyph.removeFromTop(glyph.getHeight() * 0.42f).removeFromRight(glyph.getWidth() * 0.55f);
        g.fillRoundedRectangle(tab.reduced(1.5f, 0.0f), 1.0f);

        auto label = juce::Rectangle<float>(glyph.getX() + glyph.getWidth() * 0.18f,
                                             glyph.getBottom() - glyph.getHeight() * 0.02f,
                                             glyph.getWidth() * 0.64f, glyph.getHeight() * 0.55f);
        g.drawRoundedRectangle(label, 1.0f, 1.4f);
    }
    else if (icon == Icon::New)
    {
        // Blank page with a folded corner and a "+" - a pattern reset reads
        // as "start a fresh page" rather than a destructive action.
        juce::Path page;
        float fold = glyph.getWidth() * 0.32f;
        page.startNewSubPath(glyph.getX(), glyph.getY());
        page.lineTo(glyph.getRight() - fold, glyph.getY());
        page.lineTo(glyph.getRight(), glyph.getY() + fold);
        page.lineTo(glyph.getRight(), glyph.getBottom());
        page.lineTo(glyph.getX(), glyph.getBottom());
        page.closeSubPath();
        g.strokePath(page, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path corner;
        corner.startNewSubPath(glyph.getRight() - fold, glyph.getY());
        corner.lineTo(glyph.getRight() - fold, glyph.getY() + fold);
        corner.lineTo(glyph.getRight(), glyph.getY() + fold);
        g.strokePath(corner, juce::PathStrokeType(1.2f));

        auto plusArea = glyph.withTrimmedTop(glyph.getHeight() * 0.38f).reduced(glyph.getWidth() * 0.2f, 0.0f);
        auto centre = plusArea.getCentre();
        float armLength = juce::jmin(plusArea.getWidth(), plusArea.getHeight()) * 0.5f;
        g.drawLine(centre.x - armLength, centre.y, centre.x + armLength, centre.y, 1.6f);
        g.drawLine(centre.x, centre.y - armLength, centre.x, centre.y + armLength, 1.6f);
    }
    else // Icon::Back
    {
        auto centre = glyph.getCentre();
        float armLength = juce::jmin(glyph.getWidth(), glyph.getHeight()) * 0.5f;
        juce::Path arrow;
        arrow.startNewSubPath(centre.x + armLength, centre.y);
        arrow.lineTo(centre.x - armLength, centre.y);
        arrow.startNewSubPath(centre.x - armLength * 0.35f, centre.y - armLength * 0.65f);
        arrow.lineTo(centre.x - armLength, centre.y);
        arrow.lineTo(centre.x - armLength * 0.35f, centre.y + armLength * 0.65f);
        g.strokePath(arrow, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
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

SampleSlotComponent::SampleSlotComponent(int trackIndex, SampleTrack& trackToUse, bool compact)
    : index(trackIndex), track(trackToUse), isCompact(compact)
{
    nameLabel.setJustificationType(juce::Justification::centredLeft);
    nameLabel.setFont(juce::Font(compact ? 14.0f : 16.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, DrumeeColours::textPrimary);
    addAndMakeVisible(nameLabel);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setFont(juce::Font(11.0f, juce::Font::plain));
    statusLabel.setColour(juce::Label::textColourId, DrumeeColours::textMuted);
    addAndMakeVisible(statusLabel);

    loadButton.setButtonText("Load");
    loadButton.onClick = [this] { if (onLoadRequested) onLoadRequested(index); };
    addAndMakeVisible(loadButton);

    if (isCompact)
    {
        editButton.setButtonText("Edit");
        editButton.onClick = [this] { if (onEditRequested) onEditRequested(index); };
        addAndMakeVisible(editButton);
    }

    refresh();
}

void SampleSlotComponent::resized()
{
    if (isCompact)
    {
        // Card layout for the main screen: accent tab (painted) + name/status
        // stacked on top, a Load/Edit button row along the bottom.
        auto bounds = getLocalBounds().reduced(10, 8);
        bounds.removeFromLeft(6); // room for the painted accent tab

        auto buttonRow = bounds.removeFromBottom(26);
        loadButton.setBounds(buttonRow.removeFromLeft((buttonRow.getWidth() - 6) / 2));
        buttonRow.removeFromLeft(6);
        editButton.setBounds(buttonRow);

        bounds.removeFromBottom(6);
        nameLabel.setBounds(bounds.removeFromTop(bounds.getHeight() / 2));
        statusLabel.setBounds(bounds);
    }
    else
    {
        // Wide, single-row layout for use inside the sample editor panel.
        auto bounds = getLocalBounds().reduced(10, 8);
        bounds.removeFromLeft(6);
        loadButton.setBounds(bounds.removeFromRight(84).reduced(0, 10));
        bounds.removeFromRight(8);
        nameLabel.setBounds(bounds.removeFromTop(bounds.getHeight() / 2));
        statusLabel.setBounds(bounds);
    }
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

// ---------------------------------------------------------------------------
// SampleEditorContent
// ---------------------------------------------------------------------------
SampleEditorContent::SampleEditorContent(int trackIndex, juce::AudioProcessorValueTreeState& state, SampleTrack& trackToUse)
{
    sampleNameLabel.setText(trackToUse.name, juce::dontSendNotification);
    sampleNameLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    sampleNameLabel.setColour(juce::Label::textColourId, DrumeeColours::textPrimary);
    sampleNameLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(sampleNameLabel);

    slot = std::make_unique<SampleSlotComponent>(trackIndex, trackToUse, false);
    slot->onLoadRequested = [this](int t) { if (onLoadRequested) onLoadRequested(t); };
    slot->onFileDropped = [this](int t, const juce::File& f) { if (onFileDropped) onFileDropped(t, f); };
    addAndMakeVisible(*slot);

    sectionPitch.setJustificationType(juce::Justification::centredLeft);
    sectionPitch.setFont(juce::Font(12.0f, juce::Font::bold));
    sectionPitch.setColour(juce::Label::textColourId, DrumeeColours::textSecondary);
    addAndMakeVisible(sectionPitch);

    for (auto& info : getPitchSoundParamInfoForTrack(trackIndex))
    {
        auto encoder = std::make_unique<Encoder>(state, info);
        addAndMakeVisible(*encoder);
        encoders.push_back(std::move(encoder));
    }
}

void SampleEditorContent::refresh()
{
    if (slot != nullptr)
        slot->refresh();
}

void SampleEditorContent::resized()
{
    constexpr int margin = 20;
    auto bounds = getLocalBounds().reduced(margin);

    sampleNameLabel.setBounds(bounds.removeFromTop(28));
    bounds.removeFromTop(10);

    slot->setBounds(bounds.removeFromTop(66));
    bounds.removeFromTop(18);

    sectionPitch.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(8);

    // 2x2 grid of Pitch & Sound encoders, filling the remaining space.
    auto grid = bounds;
    int colWidth = grid.getWidth() / 2;
    int rowHeight = grid.getHeight() / 2;
    for (size_t i = 0; i < encoders.size(); ++i)
    {
        int row = (int) i / 2;
        int col = (int) i % 2;
        juce::Rectangle<int> cell(grid.getX() + col * colWidth, grid.getY() + row * rowHeight, colWidth, rowHeight);
        encoders[i]->setBounds(cell.reduced(10));
    }
}

void SampleEditorContent::paint(juce::Graphics& g)
{
    // This page now lives nested inside SampleEditorPanel's own panel
    // background (rather than filling a standalone window), so it stays
    // transparent and only draws the recessed well behind its encoder grid,
    // matching the "panelAlt" treatment used for other sunken fields.
    constexpr int margin = 20;
    auto bounds = getLocalBounds().reduced(margin);
    bounds.removeFromTop(28 + 10 + 66 + 18 + 20 + 8);

    g.setColour(DrumeeColours::panelAlt);
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(DrumeeColours::outline);
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// SampleEditorPanel
// ---------------------------------------------------------------------------
SampleEditorPanel::SampleEditorPanel(juce::AudioProcessorValueTreeState& state, std::array<SampleTrack, kNumTracks>& tracksToUse)
{
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, DrumeeColours::textSecondary);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    closeButton.onClick = [this] { if (onCloseRequested) onCloseRequested(); };
    addAndMakeVisible(closeButton);

    for (int i = 0; i < kNumTracks; ++i)
    {
        auto tab = std::make_unique<juce::TextButton>(tracksToUse[(size_t) i].name);
        tab->onClick = [this, i] { showTrack(i); };
        addAndMakeVisible(*tab);
        tabButtons[(size_t) i] = std::move(tab);

        auto page = std::make_unique<SampleEditorContent>(i, state, tracksToUse[(size_t) i]);
        page->onLoadRequested = [this](int t) { if (onLoadRequested) onLoadRequested(t); };
        page->onFileDropped = [this](int t, const juce::File& f) { if (onFileDropped) onFileDropped(t, f); };
        addChildComponent(*page); // hidden until selected by showTrack()
        pages[(size_t) i] = std::move(page);
    }

    showTrack(0);
}

void SampleEditorPanel::showTrack(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= kNumTracks)
        return;

    currentTrack = trackIndex;

    for (int i = 0; i < kNumTracks; ++i)
        pages[(size_t) i]->setVisible(i == currentTrack);

    updateTabColours();
    resized();
}

void SampleEditorPanel::updateTabColours()
{
    for (int i = 0; i < kNumTracks; ++i)
    {
        auto* tab = tabButtons[(size_t) i].get();
        juce::Colour accent = DrumeeColours::forTrack(i);
        bool isSelected = (i == currentTrack);

        tab->setColour(juce::TextButton::buttonColourId, isSelected ? accent : DrumeeColours::panelAlt);
        tab->setColour(juce::TextButton::textColourOffId, isSelected ? DrumeeColours::textInverse : DrumeeColours::textSecondary);
    }
}

void SampleEditorPanel::refresh()
{
    for (auto& page : pages)
        if (page != nullptr)
            page->refresh();
}

void SampleEditorPanel::resized()
{
    auto bounds = getLocalBounds().reduced(14);

    auto headerRow = bounds.removeFromTop(24);
    closeButton.setBounds(headerRow.removeFromLeft(28));
    headerRow.removeFromLeft(8);
    titleLabel.setBounds(headerRow);

    bounds.removeFromTop(10);

    auto tabRow = bounds.removeFromTop(28);
    int tabGap = 6;
    int tabWidth = (tabRow.getWidth() - tabGap * (kNumTracks - 1)) / kNumTracks;
    for (auto& tab : tabButtons)
    {
        tab->setBounds(tabRow.removeFromLeft(tabWidth));
        tabRow.removeFromLeft(tabGap);
    }

    bounds.removeFromTop(10);

    for (auto& page : pages)
        page->setBounds(bounds);
}

void SampleEditorPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(DrumeeColours::panel);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(DrumeeColours::outline);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
}
