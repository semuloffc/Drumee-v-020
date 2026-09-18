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

void DrumeeLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                              bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    juce::Colour base = backgroundColour;
    if (isDown)
        base = base.darker(0.15f);
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

void Encoder::setAccentColour(juce::Colour newColour)
{
    accentColour = newColour;
    slider.setColour(juce::Slider::rotarySliderFillColourId, accentColour);
    repaint();
}

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
    setInterceptsMouseClicks(true, false);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    nameLabel.setJustificationType(juce::Justification::centredLeft);
    nameLabel.setFont(juce::Font(compact ? 14.0f : 16.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, DrumeeColours::textPrimary);
    nameLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(nameLabel);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setFont(juce::Font(11.0f, juce::Font::plain));
    statusLabel.setColour(juce::Label::textColourId, DrumeeColours::textMuted);
    statusLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(statusLabel);

    setTooltip(isCompact ? "Click to edit this sample \xc2\xb7 drop an audio file to load"
                          : "Click or drop an audio file to load");

    refresh();
}

void SampleSlotComponent::resized()
{
    // Buttonless card: accent tab (painted) on the left, name on top,
    // status/filename below it - the whole card is the click target.
    auto bounds = getLocalBounds().reduced(10, 8);
    bounds.removeFromLeft(6);
    nameLabel.setBounds(bounds.removeFromTop(bounds.getHeight() / 2));
    statusLabel.setBounds(bounds);
}

void SampleSlotComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::Colour accent = DrumeeColours::forTrack(index);

    juce::Colour fill = DrumeeColours::panel;
    if (isDragHover)
        fill = DrumeeColours::panelAlt.brighter(0.08f);
    else if (isSelected)
        fill = DrumeeColours::panelAlt;
    else if (isMouseOver)
        fill = DrumeeColours::panel.brighter(0.05f);

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 5.0f);

    bool showAccentOutline = isDragHover || isSelected;
    g.setColour(showAccentOutline ? accent : DrumeeColours::outline);
    g.drawRoundedRectangle(bounds.reduced(0.75f), 5.0f, showAccentOutline ? 2.0f : 1.0f);

    // Flat accent tab on the left edge identifies the track colour even
    // when the slot is empty, matching the step-grid colour for that row.
    // Widens slightly when this is the sample currently being edited.
    g.setColour(accent);
    float tabWidth = isSelected ? 5.0f : 4.0f;
    g.fillRoundedRectangle(bounds.removeFromLeft(tabWidth).reduced(0.0f, 6.0f), 2.0f);
}

void SampleSlotComponent::refresh()
{
    nameLabel.setText(track.name, juce::dontSendNotification);
    statusLabel.setText(track.loaded ? track.sourceFile.getFileName() : "Empty slot",
                         juce::dontSendNotification);
}

void SampleSlotComponent::setSelected(bool shouldBeSelected)
{
    if (isSelected == shouldBeSelected)
        return;
    isSelected = shouldBeSelected;
    repaint();
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

void SampleSlotComponent::mouseEnter(const juce::MouseEvent&)
{
    isMouseOver = true;
    repaint();
}

void SampleSlotComponent::mouseExit(const juce::MouseEvent&)
{
    isMouseOver = false;
    repaint();
}

void SampleSlotComponent::mouseUp(const juce::MouseEvent& event)
{
    if (! event.mouseWasClicked())
        return;

    // Non-compact = already inside the sample editor panel, already the
    // sample being edited - a click there means "load a file" rather than
    // "select me", since there is nothing left to select.
    if (isCompact)
    {
        if (onEditRequested)
            onEditRequested(index);
    }
    else
    {
        if (onLoadRequested)
            onLoadRequested(index);
    }
}

// ---------------------------------------------------------------------------
// WaveformDisplay
// ---------------------------------------------------------------------------
WaveformDisplay::WaveformDisplay(int trackIndex, SampleTrack& trackToUse)
    : track(trackToUse), accentColour(DrumeeColours::forTrack(trackIndex))
{
}

void WaveformDisplay::resized() { rebuildPeaks(); }
void WaveformDisplay::refresh() { rebuildPeaks(); repaint(); }

void WaveformDisplay::rebuildPeaks()
{
    peakMin.clear();
    peakMax.clear();

    int w = getWidth();
    int numSamples = track.buffer.getNumSamples();
    int numChannels = track.buffer.getNumChannels();
    if (w <= 0 || ! track.loaded || numSamples <= 0 || numChannels <= 0)
        return;

    peakMin.resize((size_t) w, 0.0f);
    peakMax.resize((size_t) w, 0.0f);

    for (int x = 0; x < w; ++x)
    {
        int64_t startSample = (int64_t) x * numSamples / w;
        int64_t endSample = juce::jmax(startSample + 1, (int64_t) (x + 1) * numSamples / w);
        endSample = juce::jmin(endSample, (int64_t) numSamples);

        float mn = 0.0f, mx = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto range = track.buffer.findMinMax(ch, (int) startSample, (int) (endSample - startSample));
            mn = juce::jmin(mn, range.getStart());
            mx = juce::jmax(mx, range.getEnd());
        }
        peakMin[(size_t) x] = mn;
        peakMax[(size_t) x] = mx;
    }
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(DrumeeColours::panelAlt);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(DrumeeColours::outline);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

    float midY = bounds.getCentreY();

    if (! track.loaded || peakMax.empty())
    {
        g.setColour(DrumeeColours::outline.withAlpha(0.6f));
        g.drawLine(bounds.getX() + 8.0f, midY, bounds.getRight() - 8.0f, midY, 1.0f);
        g.setColour(DrumeeColours::textMuted);
        g.setFont(juce::Font(11.0f));
        g.drawText("No sample loaded", getLocalBounds(), juce::Justification::centred);
        return;
    }

    float halfHeight = bounds.getHeight() * 0.5f - 5.0f;

    juce::Path wave;
    wave.startNewSubPath(bounds.getX(), midY - peakMax[0] * halfHeight);
    for (size_t x = 1; x < peakMax.size(); ++x)
        wave.lineTo(bounds.getX() + (float) x, midY - peakMax[x] * halfHeight);
    for (size_t x = peakMin.size(); x-- > 0;)
        wave.lineTo(bounds.getX() + (float) x, midY - peakMin[x] * halfHeight);
    wave.closeSubPath();

    g.setColour(accentColour.withAlpha(0.30f));
    g.fillPath(wave);
    g.setColour(accentColour.withAlpha(0.65f));
    g.strokePath(wave, juce::PathStrokeType(1.0f));

    g.setColour(DrumeeColours::outline.withAlpha(0.5f));
    g.drawLine(bounds.getX(), midY, bounds.getRight(), midY, 1.0f);
}

// ---------------------------------------------------------------------------
// EnvelopeVisualizer
// ---------------------------------------------------------------------------
namespace
{
    // Fixed visual widths for the four envelope zones (fractions of the
    // graph's usable width). Sustain has no time parameter of its own, so
    // its zone is just a fixed-width plateau that reads as "held" - it
    // does not scale with anything.
    constexpr float kAttackZoneFrac  = 0.20f;
    constexpr float kDecayZoneFrac   = 0.25f;
    constexpr float kSustainZoneFrac = 0.25f;
    // Release gets the remainder (~0.30f).

    // How long the animated playback marker lingers on the sustain
    // plateau before sweeping into Release - a fixed value since one-shot
    // triggers have no explicit hold length to draw from.
    constexpr double kSustainHoldMs = 220.0;

    constexpr float kPointHitRadius = 10.0f;
}

EnvelopeVisualizer::EnvelopeVisualizer(juce::AudioProcessorValueTreeState& state, int trackIndex, SampleTrack& trackToUse)
    : track(trackToUse), accentColour(DrumeeColours::forTrack(trackIndex))
{
    attackParam  = state.getParameter(perTrackParamID(EnvelopeParamIDs::attack, trackIndex));
    decayParam   = state.getParameter(perTrackParamID(PitchSoundParamIDs::decay, trackIndex));
    sustainParam = state.getParameter(perTrackParamID(EnvelopeParamIDs::sustain, trackIndex));
    releaseParam = state.getParameter(perTrackParamID(EnvelopeParamIDs::release, trackIndex));

    attackRaw  = state.getRawParameterValue(perTrackParamID(EnvelopeParamIDs::attack, trackIndex));
    decayRaw   = state.getRawParameterValue(perTrackParamID(PitchSoundParamIDs::decay, trackIndex));
    sustainRaw = state.getRawParameterValue(perTrackParamID(EnvelopeParamIDs::sustain, trackIndex));
    releaseRaw = state.getRawParameterValue(perTrackParamID(EnvelopeParamIDs::release, trackIndex));

    setInterceptsMouseClicks(true, false);
    setTooltip("Drag the points to shape Attack / Decay & Sustain / Release");
    startTimerHz(30);
}

EnvelopeVisualizer::~EnvelopeVisualizer() { stopTimer(); }

EnvelopeVisualizer::Geometry EnvelopeVisualizer::buildGeometry() const
{
    Geometry geo;
    geo.area = getLocalBounds().toFloat().reduced(16.0f, 14.0f);

    float w = geo.area.getWidth();
    auto zones = geo.area;
    geo.zoneAttack  = zones.removeFromLeft(w * kAttackZoneFrac);
    geo.zoneDecay   = zones.removeFromLeft(w * kDecayZoneFrac);
    geo.zoneSustain = zones.removeFromLeft(w * kSustainZoneFrac);
    geo.zoneRelease = zones;

    float attackMs  = juce::jlimit(0.0f, EnvelopeRanges::attackMaxMs, getAttack());
    float decayMs   = juce::jlimit(0.0f, EnvelopeRanges::decayMaxMs, getDecay());
    float sustainLv = juce::jlimit(0.0f, 1.0f, getSustain01());
    float releaseMs = juce::jlimit(0.0f, EnvelopeRanges::releaseMaxMs, getRelease());

    float sustainY = juce::jmap(sustainLv, 0.0f, 1.0f, geo.area.getBottom(), geo.area.getY());

    geo.p0 = { geo.area.getX(), geo.area.getBottom() };
    geo.p1 = { geo.zoneAttack.getX() + (attackMs / EnvelopeRanges::attackMaxMs) * geo.zoneAttack.getWidth(),
               geo.area.getY() };
    geo.p2 = { geo.zoneDecay.getX() + (decayMs / EnvelopeRanges::decayMaxMs) * geo.zoneDecay.getWidth(),
               sustainY };
    geo.p3 = { geo.zoneSustain.getRight(), sustainY };
    geo.p4 = { geo.zoneRelease.getX() + (releaseMs / EnvelopeRanges::releaseMaxMs) * geo.zoneRelease.getWidth(),
               geo.area.getBottom() };

    return geo;
}

EnvelopeVisualizer::DragTarget EnvelopeVisualizer::hitTest(juce::Point<float> position, const Geometry& geo) const
{
    struct Candidate { DragTarget target; juce::Point<float> point; };
    const Candidate candidates[] = {
        { DragTarget::attackPoint,  geo.p1 },
        { DragTarget::decayPoint,   geo.p2 },
        { DragTarget::releasePoint, geo.p4 }
    };

    DragTarget best = DragTarget::none;
    float bestDistance = kPointHitRadius;

    for (auto& c : candidates)
    {
        float distance = position.getDistanceFrom(c.point);
        if (distance <= bestDistance)
        {
            bestDistance = distance;
            best = c.target;
        }
    }
    return best;
}

void EnvelopeVisualizer::setNormalisedValue(juce::RangedAudioParameter* param, float realValue) const
{
    if (param != nullptr)
        param->setValueNotifyingHost(param->convertTo0to1(realValue));
}

void EnvelopeVisualizer::mouseDown(const juce::MouseEvent& event)
{
    auto geo = buildGeometry();
    dragging = hitTest(event.position, geo);

    if (dragging == DragTarget::attackPoint && attackParam != nullptr)
        attackParam->beginChangeGesture();
    else if (dragging == DragTarget::decayPoint)
    {
        if (decayParam != nullptr) decayParam->beginChangeGesture();
        if (sustainParam != nullptr) sustainParam->beginChangeGesture();
    }
    else if (dragging == DragTarget::releasePoint && releaseParam != nullptr)
        releaseParam->beginChangeGesture();

    if (dragging != DragTarget::none)
        repaint();
}

void EnvelopeVisualizer::mouseDrag(const juce::MouseEvent& event)
{
    if (dragging == DragTarget::none)
        return;

    auto geo = buildGeometry();

    auto mapXToMs = [] (float x, juce::Rectangle<float> zone, float maxMs)
    {
        float frac = zone.getWidth() > 0.0f ? (x - zone.getX()) / zone.getWidth() : 0.0f;
        return juce::jlimit(0.0f, maxMs, frac * maxMs);
    };

    switch (dragging)
    {
        case DragTarget::attackPoint:
            setNormalisedValue(attackParam, mapXToMs(event.position.x, geo.zoneAttack, EnvelopeRanges::attackMaxMs));
            break;

        case DragTarget::decayPoint:
        {
            float newDecay = mapXToMs(event.position.x, geo.zoneDecay, EnvelopeRanges::decayMaxMs);
            float levelFrac = geo.area.getHeight() > 0.0f
                ? (geo.area.getBottom() - event.position.y) / geo.area.getHeight() : 0.0f;
            float newSustain = juce::jlimit(0.0f, 100.0f, levelFrac * 100.0f);
            setNormalisedValue(decayParam, newDecay);
            setNormalisedValue(sustainParam, newSustain);
            break;
        }

        case DragTarget::releasePoint:
            setNormalisedValue(releaseParam, mapXToMs(event.position.x, geo.zoneRelease, EnvelopeRanges::releaseMaxMs));
            break;

        case DragTarget::none:
        default:
            break;
    }

    repaint();
}

void EnvelopeVisualizer::mouseUp(const juce::MouseEvent&)
{
    if (dragging == DragTarget::attackPoint && attackParam != nullptr)
        attackParam->endChangeGesture();
    else if (dragging == DragTarget::decayPoint)
    {
        if (decayParam != nullptr) decayParam->endChangeGesture();
        if (sustainParam != nullptr) sustainParam->endChangeGesture();
    }
    else if (dragging == DragTarget::releasePoint && releaseParam != nullptr)
        releaseParam->endChangeGesture();

    dragging = DragTarget::none;
    repaint();
}

void EnvelopeVisualizer::mouseMove(const juce::MouseEvent& event)
{
    auto geo = buildGeometry();
    auto newHover = hitTest(event.position, geo);
    if (newHover != hovered)
    {
        hovered = newHover;
        setMouseCursor(hovered == DragTarget::none ? juce::MouseCursor::NormalCursor
                                                    : juce::MouseCursor::PointingHandCursor);
        repaint();
    }
}

void EnvelopeVisualizer::mouseExit(const juce::MouseEvent&)
{
    if (hovered != DragTarget::none)
    {
        hovered = DragTarget::none;
        repaint();
    }
}

juce::Point<float> EnvelopeVisualizer::pointAtElapsed(const Geometry& geo, double elapsedMs) const
{
    double attackMs  = juce::jmax(0.0, (double) getAttack());
    double decayMs   = juce::jmax(0.0, (double) getDecay());
    double releaseMs = juce::jmax(0.0, (double) getRelease());
    double t = juce::jmax(0.0, elapsedMs);

    if (t <= attackMs)
    {
        double frac = attackMs > 0.0 ? t / attackMs : 1.0;
        float x = geo.zoneAttack.getX() + (float) (t / (double) EnvelopeRanges::attackMaxMs) * geo.zoneAttack.getWidth();
        float y = juce::jmap((float) frac, 0.0f, 1.0f, geo.area.getBottom(), geo.area.getY());
        return { x, y };
    }
    t -= attackMs;

    if (t <= decayMs)
    {
        double frac = decayMs > 0.0 ? t / decayMs : 1.0;
        float eased = (float) (frac * frac * (3.0 - 2.0 * frac));
        float x = geo.zoneDecay.getX() + (float) (t / (double) EnvelopeRanges::decayMaxMs) * geo.zoneDecay.getWidth();
        float y = juce::jmap(eased, 0.0f, 1.0f, geo.area.getY(), geo.p2.y);
        return { x, y };
    }
    t -= decayMs;

    if (t <= kSustainHoldMs)
    {
        double frac = kSustainHoldMs > 0.0 ? t / kSustainHoldMs : 1.0;
        float x = juce::jmap((float) frac, 0.0f, 1.0f, geo.p2.x, geo.p3.x);
        return { x, geo.p2.y };
    }
    t -= kSustainHoldMs;

    double frac = juce::jlimit(0.0, 1.0, releaseMs > 0.0 ? t / releaseMs : 1.0);
    float eased = (float) (frac * frac * (3.0 - 2.0 * frac));
    float x = geo.zoneRelease.getX()
             + (float) juce::jlimit(0.0, 1.0, t / (double) EnvelopeRanges::releaseMaxMs) * geo.zoneRelease.getWidth();
    float y = juce::jmap(eased, 0.0f, 1.0f, geo.p2.y, geo.area.getBottom());
    return { x, y };
}

void EnvelopeVisualizer::timerCallback()
{
    int count = track.triggerCount.load(std::memory_order_relaxed);
    if (count != lastSeenTriggerCount)
    {
        lastSeenTriggerCount = count;
        animating = true;
        animationStartMs = juce::Time::getMillisecondCounterHiRes();
    }

    if (animating)
    {
        double elapsed = juce::Time::getMillisecondCounterHiRes() - animationStartMs;
        double totalMs = (double) getAttack() + (double) getDecay() + kSustainHoldMs + (double) getRelease();
        if (elapsed >= totalMs)
            animating = false;
        repaint();
    }
}

void EnvelopeVisualizer::drawPoint(juce::Graphics& g, juce::Point<float> p, bool active) const
{
    float r = active ? 5.5f : 4.0f;
    auto dot = juce::Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(p);

    g.setColour(DrumeeColours::panelAlt);
    g.fillEllipse(dot);
    g.setColour(accentColour);
    g.drawEllipse(dot, active ? 2.2f : 1.5f);

    if (active)
    {
        g.setColour(accentColour);
        g.fillEllipse(juce::Rectangle<float>(3.0f, 3.0f).withCentre(p));
    }
}

void EnvelopeVisualizer::drawValueChip(juce::Graphics& g, const Geometry& geo) const
{
    DragTarget target = dragging != DragTarget::none ? dragging : hovered;
    if (target == DragTarget::none)
        return;

    juce::String text;
    juce::Point<float> anchor;

    if (target == DragTarget::attackPoint)
    {
        text = "Attack " + juce::String(getAttack(), 0) + " ms";
        anchor = geo.p1;
    }
    else if (target == DragTarget::decayPoint)
    {
        text = "Decay " + juce::String(getDecay(), 0) + " ms  \xc2\xb7  Sustain " + juce::String(getSustain01() * 100.0f, 0) + "%";
        anchor = geo.p2;
    }
    else
    {
        text = "Release " + juce::String(getRelease(), 0) + " ms";
        anchor = geo.p4;
    }

    juce::Font font(11.5f, juce::Font::bold);
    g.setFont(font);
    float textWidth = font.getStringWidthFloat(text) + 14.0f;
    float textHeight = 19.0f;

    float x = juce::jlimit(geo.area.getX(), juce::jmax(geo.area.getX(), geo.area.getRight() - textWidth),
                            anchor.x - textWidth * 0.5f);
    float y = anchor.y - textHeight - 9.0f;
    if (y < geo.area.getY())
        y = juce::jmin(anchor.y + 9.0f, geo.area.getBottom() - textHeight);

    juce::Rectangle<float> chip(x, y, textWidth, textHeight);
    g.setColour(DrumeeColours::panel);
    g.fillRoundedRectangle(chip, 4.0f);
    g.setColour(accentColour);
    g.drawRoundedRectangle(chip.reduced(0.5f), 4.0f, 1.0f);
    g.setColour(DrumeeColours::textPrimary);
    g.drawText(text, chip, juce::Justification::centred);
}

void EnvelopeVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(DrumeeColours::panelAlt);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(DrumeeColours::outline);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

    auto geo = buildGeometry();

    float midY = (geo.area.getY() + geo.area.getBottom()) * 0.5f;
    g.setColour(DrumeeColours::outline.withAlpha(0.6f));
    g.drawLine(geo.area.getX(), midY, geo.area.getRight(), midY, 1.0f);
    g.drawLine(geo.area.getX(), geo.area.getBottom(), geo.area.getRight(), geo.area.getBottom(), 1.0f);

    juce::Path shape;
    shape.startNewSubPath(geo.p0);
    shape.lineTo(geo.p1);
    shape.quadraticTo({ (geo.p1.x + geo.p2.x) * 0.5f, geo.p1.y }, geo.p2);
    shape.lineTo(geo.p3);
    shape.quadraticTo({ (geo.p3.x + geo.p4.x) * 0.5f, geo.p3.y }, geo.p4);

    juce::Path fill = shape;
    fill.lineTo(geo.p4.x, geo.area.getBottom());
    fill.lineTo(geo.p0.x, geo.area.getBottom());
    fill.closeSubPath();

    g.setColour(accentColour.withAlpha(0.16f));
    g.fillPath(fill);

    g.setColour(accentColour);
    g.strokePath(shape, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    drawPoint(g, geo.p1, dragging == DragTarget::attackPoint || hovered == DragTarget::attackPoint);
    drawPoint(g, geo.p2, dragging == DragTarget::decayPoint || hovered == DragTarget::decayPoint);
    drawPoint(g, geo.p4, dragging == DragTarget::releasePoint || hovered == DragTarget::releasePoint);

    if (animating)
    {
        double elapsed = juce::Time::getMillisecondCounterHiRes() - animationStartMs;
        auto marker = pointAtElapsed(geo, elapsed);
        g.setColour(accentColour.withAlpha(0.45f));
        g.drawLine(marker.x, geo.area.getBottom(), marker.x, geo.area.getY(), 1.0f);
        g.setColour(DrumeeColours::textPrimary);
        g.fillEllipse(juce::Rectangle<float>(5.0f, 5.0f).withCentre(marker));
    }

    drawValueChip(g, geo);
}

// ---------------------------------------------------------------------------
// SampleEditorContent
// ---------------------------------------------------------------------------
SampleEditorContent::SampleEditorContent(int trackIndex, juce::AudioProcessorValueTreeState& state, SampleTrack& trackToUse)
{
    juce::Colour trackAccent = DrumeeColours::forTrack(trackIndex);

    sampleNameLabel.setText(trackToUse.name, juce::dontSendNotification);
    sampleNameLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    sampleNameLabel.setColour(juce::Label::textColourId, trackAccent);
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

    juce::Colour knobAccent = DrumeeColours::forSampleKnob(trackIndex);
    for (auto& info : getPitchSoundParamInfoForTrack(trackIndex))
    {
        auto encoder = std::make_unique<Encoder>(state, info);
        encoder->setAccentColour(knobAccent);
        addAndMakeVisible(*encoder);
        pitchEncoders.push_back(std::move(encoder));
    }

    sectionEnvelope.setJustificationType(juce::Justification::centredLeft);
    sectionEnvelope.setFont(juce::Font(12.0f, juce::Font::bold));
    sectionEnvelope.setColour(juce::Label::textColourId, DrumeeColours::textSecondary);
    addAndMakeVisible(sectionEnvelope);

    waveformDisplay = std::make_unique<WaveformDisplay>(trackIndex, trackToUse);
    addAndMakeVisible(*waveformDisplay);

    envelopeVisualizer = std::make_unique<EnvelopeVisualizer>(state, trackIndex, trackToUse);
    addAndMakeVisible(*envelopeVisualizer);

    juce::Colour envelopeAccent = DrumeeColours::forTrack(trackIndex);
    for (auto& info : getEnvelopeParamInfoForTrack(trackIndex))
    {
        auto encoder = std::make_unique<Encoder>(state, info);
        encoder->setAccentColour(envelopeAccent);
        addAndMakeVisible(*encoder);
        envelopeEncoders.push_back(std::move(encoder));
    }
}

void SampleEditorContent::refresh()
{
    if (slot != nullptr)
        slot->refresh();
    if (waveformDisplay != nullptr)
        waveformDisplay->refresh();
}

void SampleEditorContent::resized()
{
    constexpr int margin = 20;
    auto bounds = getLocalBounds().reduced(margin);

    sampleNameLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(6);

    // Top row: file slot (left) and the compact Pitch & Sound knobs (right)
    // share one row, leaving most of the page's height for the waveform +
    // envelope block below - the main showcase of this page.
    auto topRow = bounds.removeFromTop(62);
    bounds.removeFromTop(10);

    auto slotArea = topRow.removeFromLeft((int) (topRow.getWidth() * 0.56f));
    slot->setBounds(slotArea);

    topRow.removeFromLeft(16);
    sectionPitch.setBounds(topRow.removeFromTop(16));
    topRow.removeFromTop(4);
    pitchWellBounds = topRow;
    auto pitchRow = topRow;
    int pitchCellWidth = pitchEncoders.empty() ? pitchRow.getWidth() : pitchRow.getWidth() / (int) pitchEncoders.size();
    for (size_t i = 0; i < pitchEncoders.size(); ++i)
    {
        juce::Rectangle<int> cell(pitchRow.getX() + (int) i * pitchCellWidth, pitchRow.getY(), pitchCellWidth, pitchRow.getHeight());
        pitchEncoders[i]->setBounds(cell.reduced(10, 0));
    }

    sectionEnvelope.setBounds(bounds.removeFromTop(16));
    bounds.removeFromTop(6);

    // Waveform strip shares the envelope graph's exact x-position and width
    // (computed once here) so the two form one aligned block - the sample's
    // content sits directly above the envelope shaping it.
    int graphWidth = (int) (bounds.getWidth() * 0.56f);
    auto waveformArea = bounds.removeFromTop(46);
    waveformDisplay->setBounds(waveformArea.getX(), waveformArea.getY(), graphWidth, waveformArea.getHeight());
    bounds.removeFromTop(8);

    // Envelope section: the interactive graph takes the same left column as
    // the waveform above it, its five knobs (Attack/Decay/Sustain/Release/
    // Volume) fill the rest as a single row so nothing has to wrap or overlap.
    auto graphArea = bounds.removeFromLeft(graphWidth);
    envelopeVisualizer->setBounds(graphArea.reduced(0, 2));

    bounds.removeFromLeft(16);
    envelopeKnobWellBounds = bounds;
    auto knobArea = bounds;
    int knobCellWidth = envelopeEncoders.empty() ? knobArea.getWidth() : knobArea.getWidth() / (int) envelopeEncoders.size();
    int knobHeight = juce::jmin(130, knobArea.getHeight());
    int knobTopPad = juce::jmax(0, (knobArea.getHeight() - knobHeight) / 2);
    for (size_t i = 0; i < envelopeEncoders.size(); ++i)
    {
        juce::Rectangle<int> cell(knobArea.getX() + (int) i * knobCellWidth, knobArea.getY() + knobTopPad, knobCellWidth, knobHeight);
        envelopeEncoders[i]->setBounds(cell.reduced(8, 0));
    }
}

void SampleEditorContent::paint(juce::Graphics& g)
{
    // This page now lives nested inside SampleEditorPanel's own panel
    // background (rather than filling a standalone window), so it only
    // draws the recessed "well" behind each knob row - the envelope graph
    // and sample slot already paint their own panelAlt background.
    for (auto bounds : { pitchWellBounds, envelopeKnobWellBounds })
    {
        if (bounds.isEmpty())
            continue;

        auto wellBounds = bounds.toFloat();
        g.setColour(DrumeeColours::panelAlt);
        g.fillRoundedRectangle(wellBounds, 6.0f);
        g.setColour(DrumeeColours::outline);
        g.drawRoundedRectangle(wellBounds.reduced(0.5f), 6.0f, 1.0f);
    }
}

// ---------------------------------------------------------------------------
// SampleEditorPanel
// ---------------------------------------------------------------------------
SampleEditorPanel::SampleEditorPanel(juce::AudioProcessorValueTreeState& state, std::array<SampleTrack, kNumTracks>& tracksToUse)
{
    for (int i = 0; i < kNumTracks; ++i)
        trackNames[(size_t) i] = tracksToUse[(size_t) i].name;

    titlePrefixLabel.setText("EDIT SAMPLES \xe2\x80\x94", juce::dontSendNotification);
    titlePrefixLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titlePrefixLabel.setJustificationType(juce::Justification::centredLeft);
    titlePrefixLabel.setColour(juce::Label::textColourId, DrumeeColours::textSecondary);
    addAndMakeVisible(titlePrefixLabel);

    titleLabel.setFont(juce::Font(15.0f, juce::Font::bold));
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
    updateTitle();
    resized();

    if (onTrackChanged)
        onTrackChanged(currentTrack);
}

void SampleEditorPanel::updateTitle()
{
    // "EDIT SAMPLES —" stays a constant, neutral-coloured label; only the
    // sample name itself is set in that track's accent colour, so the
    // colour change actually reads as "this is the sample being edited"
    // instead of tinting the whole static heading.
    titleLabel.setText(trackNames[(size_t) currentTrack].toUpperCase(), juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, DrumeeColours::forTrack(currentTrack));
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

    int prefixWidth = (int) titlePrefixLabel.getFont().getStringWidthFloat(titlePrefixLabel.getText()) + 7;
    titlePrefixLabel.setBounds(headerRow.removeFromLeft(prefixWidth));
    headerRow.removeFromLeft(6);
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
