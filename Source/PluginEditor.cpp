#include "PluginEditor.h"

DrumeeAudioProcessorEditor::DrumeeAudioProcessorEditor(DrumeeAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lookAndFeel);

    setResizable(false, false);

    titleLabel.setText("DRUMEE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, DrumeeColours::textPrimary);
    addAndMakeVisible(titleLabel);

    versionLabel.setText("v" + juce::String(JucePlugin_VersionString), juce::dontSendNotification);
    versionLabel.setFont(juce::Font(11.0f, juce::Font::plain));
    versionLabel.setColour(juce::Label::textColourId, DrumeeColours::textMuted);
    addAndMakeVisible(versionLabel);

    presetBox.setTextWhenNothingSelected("Init Preset");
    presetBox.onChange = [this]
    {
        auto name = presetBox.getText();
        if (name.isNotEmpty() && processor.presetManager.loadPreset(name))
        {
            refreshAllSampleSlots();
            if (visualizer != nullptr)
                visualizer->repaint();
        }
    };
    addAndMakeVisible(presetBox);

    saveButton.onClick = [this] { savePresetDialog(); };
    addAndMakeVisible(saveButton);

    newButton.onClick = [this]
    {
        for (auto& step : processor.sequencer.pattern)
            for (int t = 0; t < kNumTracks; ++t)
                step.active[t] = false;

        if (visualizer != nullptr)
            visualizer->repaint();
    };
    addAndMakeVisible(newButton);

    editSamplesButton.onClick = [this] { toggleSampleEditor(! isEditingSamples); };
    addAndMakeVisible(editSamplesButton);

    // PITCH & SOUND no longer has a section on the main screen - it moved
    // into the internal sample editor panel. Only Timing/Groove and
    // Ratchet/Chaos remain here, plus a small header over the sample row.
    for (auto* label : { &sectionTiming, &sectionChaos, &sectionSamples })
    {
        label->setJustificationType(juce::Justification::centredLeft);
        label->setFont(juce::Font(12.0f, juce::Font::bold));
        label->setColour(juce::Label::textColourId, DrumeeColours::textSecondary);
        addAndMakeVisible(label);
    }

    for (auto& info : getAllParamInfo())
    {
        auto encoder = std::make_unique<Encoder>(processor.apvts, info);
        addAndMakeVisible(*encoder);
        encoders.push_back(std::move(encoder));
    }

    visualizer = std::make_unique<StepSequencerVisualizer>(processor.sequencer);
    addAndMakeVisible(*visualizer);

    for (int i = 0; i < kNumTracks; ++i)
    {
        auto slot = std::make_unique<SampleSlotComponent>(i, processor.tracks[(size_t) i], true);
        slot->onLoadRequested = [this](int trackIndex) { loadSample(trackIndex); };
        slot->onFileDropped = [this](int trackIndex, const juce::File& file)
        {
            if (processor.loadSampleForTrack(trackIndex, file)
                && trackIndex >= 0 && trackIndex < (int) sampleSlots.size())
            {
                sampleSlots[(size_t) trackIndex]->refresh();
                if (sampleEditorPanel != nullptr)
                    sampleEditorPanel->refresh();
            }
        };
        slot->onEditRequested = [this](int trackIndex) { toggleSampleEditor(true, trackIndex); };
        addAndMakeVisible(*slot);
        sampleSlots.push_back(std::move(slot));
    }

    // Internal panel that takes over the sequencer's own bounds while
    // editing samples - one per-track tab, each holding that sample's file
    // slot and its own unique PITCH & SOUND controls.
    sampleEditorPanel = std::make_unique<SampleEditorPanel>(processor.apvts, processor.tracks);
    sampleEditorPanel->onLoadRequested = [this](int trackIndex) { loadSample(trackIndex); };
    sampleEditorPanel->onFileDropped = [this](int trackIndex, const juce::File& file)
    {
        if (processor.loadSampleForTrack(trackIndex, file) && trackIndex >= 0 && trackIndex < (int) sampleSlots.size())
        {
            sampleSlots[(size_t) trackIndex]->refresh();
            if (sampleEditorPanel != nullptr)
                sampleEditorPanel->refresh();
        }
    };
    sampleEditorPanel->onCloseRequested = [this] { toggleSampleEditor(false); };
    sampleEditorPanel->onTrackChanged = [this](int trackIndex)
    {
        if (isEditingSamples)
            highlightEditedSample(trackIndex);
    };
    addChildComponent(*sampleEditorPanel); // hidden until Edit Samples is pressed

    refreshPresetList();

    // setSize() triggers resized() synchronously, so it must come after every
    // child component (encoders, visualizer, sample slots) has been created -
    // otherwise resized() dereferences still-null pointers and crashes.
    setSize(1280, 720);
}

DrumeeAudioProcessorEditor::~DrumeeAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

namespace
{
    // Single source of truth for the top bar geometry, shared by paint()
    // and resized() so the flat header panel always lines up exactly with
    // the controls sitting on it (this was the cause of the header panel
    // drifting out from under the title/buttons in the old build).
    constexpr int kMargin = 16;
    constexpr int kTopBarHeight = 48;
    constexpr int kSideColumnWidth = 230;
    constexpr int kSampleRowHeight = 130;
}

void DrumeeAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Flat fill - no gradients, no soft "glass" noise. Kilohearts-style
    // plugin backgrounds are a single flat colour so every panel and
    // control reads at full, consistent contrast.
    g.fillAll(DrumeeColours::background);

    auto headerBounds = getLocalBounds().reduced(kMargin).removeFromTop(kTopBarHeight);
    g.setColour(DrumeeColours::panel);
    g.fillRoundedRectangle(headerBounds.toFloat(), 6.0f);

    // Card panels behind the Timing/Groove and Ratchet/Chaos knob groups,
    // matching the panel treatment used by the step grid and sample cards.
    // Skipped while the sample editor panel is open and full-width - it
    // paints its own opaque panel over this same area anyway.
    if (! isEditingSamples)
    {
        g.setColour(DrumeeColours::panel);
        g.fillRoundedRectangle(timingCardBounds.toFloat(), 6.0f);
        g.fillRoundedRectangle(chaosCardBounds.toFloat(), 6.0f);
        g.setColour(DrumeeColours::outline);
        g.drawRoundedRectangle(timingCardBounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
        g.drawRoundedRectangle(chaosCardBounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
    }
}

void DrumeeAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(kMargin);

    auto topBar = bounds.removeFromTop(kTopBarHeight).reduced(6, 0);
    auto titleArea = topBar.removeFromLeft(160);
    titleLabel.setBounds(titleArea.removeFromTop(26));
    versionLabel.setBounds(titleArea);

    editSamplesButton.setBounds(topBar.removeFromRight(130).reduced(0, 6));
    topBar.removeFromRight(6);

    // Preset selector + its graphical New/Save buttons, grouped and
    // centred in whatever top-bar width remains between the title and the
    // Edit Samples button.
    constexpr int presetBoxWidth = 180;
    constexpr int iconButtonSize = 32;
    constexpr int presetGroupGap = 6;
    int presetGroupWidth = presetBoxWidth + presetGroupGap + iconButtonSize + presetGroupGap + iconButtonSize;

    auto presetGroup = topBar.withSizeKeepingCentre(presetGroupWidth, topBar.getHeight());
    presetBox.setBounds(presetGroup.removeFromLeft(presetBoxWidth).reduced(0, 6));
    presetGroup.removeFromLeft(presetGroupGap);
    newButton.setBounds(presetGroup.removeFromLeft(iconButtonSize).reduced(0, 6));
    presetGroup.removeFromLeft(presetGroupGap);
    saveButton.setBounds(presetGroup.removeFromLeft(iconButtonSize).reduced(0, 6));

    bounds.removeFromTop(10);

    // Bottom row: SAMPLES header + the five sample cards. Each card is a
    // single click target that opens the internal sample editor panel,
    // focused on that track (no separate Edit button any more).
    auto samplesArea = bounds.removeFromBottom(kSampleRowHeight);
    sectionSamples.setBounds(samplesArea.removeFromTop(20));
    samplesArea.removeFromTop(6);

    int slotGap = 10;
    int slotWidth = (samplesArea.getWidth() - slotGap * (kNumTracks - 1)) / kNumTracks;
    for (auto& slot : sampleSlots)
    {
        slot->setBounds(samplesArea.removeFromLeft(slotWidth));
        samplesArea.removeFromLeft(slotGap);
    }

    bounds.removeFromBottom(14);

    // Remaining middle area: Timing/Groove card (left), step sequencer
    // (centre, gets all the freed-up width and height), Ratchet/Chaos
    // card (right). Captured whole (before the left/right split) too, so
    // the sample editor panel can take over the full plugin width when
    // it's open - no side knob columns next to it.
    auto fullMiddleArea = bounds;

    auto leftColumn = bounds.removeFromLeft(kSideColumnWidth);
    auto rightColumn = bounds.removeFromRight(kSideColumnWidth);
    bounds.removeFromLeft(14);
    bounds.removeFromRight(14);

    std::vector<Encoder*> timingEncoders, chaosEncoders;
    auto& infoArray = getAllParamInfo();
    for (size_t i = 0; i < encoders.size(); ++i)
    {
        switch (infoArray[i].group)
        {
            case AccentGroup::timing:       timingEncoders.push_back(encoders[i].get()); break;
            case AccentGroup::ratchetChaos: chaosEncoders.push_back(encoders[i].get()); break;
            case AccentGroup::pitchSound:   break; // lives in the sample editor panel now
        }
    }

    timingCardBounds = leftColumn;
    chaosCardBounds = rightColumn;

    constexpr int encoderHeight = 120;
    constexpr int encoderGap = 14;

    // Section label stays pinned near the top of its card; the knob group
    // below it is centred in the remaining height so a card with fewer
    // knobs (Ratchet/Chaos) doesn't read as top-heavy with empty space
    // dangling underneath.
    auto timingContent = leftColumn.reduced(14);
    sectionTiming.setBounds(timingContent.removeFromTop(20));
    timingContent.removeFromTop(6);
    {
        int used = (int) timingEncoders.size() * encoderHeight + ((int) timingEncoders.size() - 1) * encoderGap;
        int topPad = juce::jmax(0, (timingContent.getHeight() - used) / 2);
        timingContent.removeFromTop(topPad);
        for (auto* enc : timingEncoders)
        {
            enc->setBounds(timingContent.removeFromTop(encoderHeight));
            timingContent.removeFromTop(encoderGap);
        }
    }

    auto chaosContent = rightColumn.reduced(14);
    sectionChaos.setBounds(chaosContent.removeFromTop(20));
    chaosContent.removeFromTop(6);
    {
        int used = (int) chaosEncoders.size() * encoderHeight + ((int) chaosEncoders.size() - 1) * encoderGap;
        int topPad = juce::jmax(0, (chaosContent.getHeight() - used) / 2);
        chaosContent.removeFromTop(topPad);
        for (auto* enc : chaosEncoders)
        {
            enc->setBounds(chaosContent.removeFromTop(encoderHeight));
            chaosContent.removeFromTop(encoderGap);
        }
    }

    // The sample editor panel takes over the full plugin width (the area
    // the sequencer, Timing/Groove and Ratchet/Chaos columns normally
    // share) while it's open, instead of sitting squeezed between the two
    // side knob columns.
    if (visualizer != nullptr)
        visualizer->setBounds(bounds);
    if (sampleEditorPanel != nullptr)
        sampleEditorPanel->setBounds(fullMiddleArea);
}

void DrumeeAudioProcessorEditor::refreshPresetList()
{
    presetBox.clear(juce::dontSendNotification);
    auto names = processor.presetManager.getAllPresetNames();
    int id = 1;
    for (auto& name : names)
        presetBox.addItem(name, id++);

    selectPresetInBox(processor.presetManager.getCurrentPresetName());
}

void DrumeeAudioProcessorEditor::selectPresetInBox(const juce::String& name)
{
    for (int i = 0; i < presetBox.getNumItems(); ++i)
    {
        if (presetBox.getItemText(i) == name)
        {
            presetBox.setSelectedItemIndex(i, juce::dontSendNotification);
            return;
        }
    }
    presetBox.setText(name, juce::dontSendNotification);
}

void DrumeeAudioProcessorEditor::refreshAllSampleSlots()
{
    for (auto& slot : sampleSlots)
        slot->refresh();
    if (sampleEditorPanel != nullptr)
        sampleEditorPanel->refresh();
}

void DrumeeAudioProcessorEditor::savePresetDialog()
{
    auto* alert = new juce::AlertWindow("Save Preset", "Enter preset name:", juce::AlertWindow::NoIcon);
    alert->addTextEditor("name", processor.presetManager.getCurrentPresetName());
    alert->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int result)
    {
        if (result == 1)
        {
            auto name = alert->getTextEditorContents("name");
            if (processor.presetManager.savePreset(name))
                refreshPresetList();
        }
        delete alert;
    }));
}

void DrumeeAudioProcessorEditor::loadSample(int trackIndex)
{
    fileChooser = std::make_unique<juce::FileChooser>("Select a sample",
                                                        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                                                        "*.wav;*.aiff;*.mp3;*.flac");

    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this, trackIndex](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file.existsAsFile())
        {
            processor.loadSampleForTrack(trackIndex, file);
            if (trackIndex >= 0 && trackIndex < (int) sampleSlots.size())
                sampleSlots[(size_t) trackIndex]->refresh();
            if (sampleEditorPanel != nullptr)
                sampleEditorPanel->refresh();
        }
    });
}

void DrumeeAudioProcessorEditor::toggleSampleEditor(bool show, int trackIndexToShow)
{
    if (sampleEditorPanel == nullptr || visualizer == nullptr)
        return;

    if (show && trackIndexToShow >= 0)
        sampleEditorPanel->showTrack(trackIndexToShow);

    isEditingSamples = show;
    sampleEditorPanel->setVisible(show);
    visualizer->setVisible(! show);

    // No side knob columns while the sample editor panel is open - it
    // takes over their space instead (see resized()/paint()).
    sectionTiming.setVisible(! show);
    sectionChaos.setVisible(! show);
    for (auto& encoder : encoders)
        encoder->setVisible(! show);

    editSamplesButton.setButtonText(show ? "Back to Beat" : "Edit Samples");
    highlightEditedSample(show ? sampleEditorPanel->getCurrentTrack() : -1);
    repaint();
}

void DrumeeAudioProcessorEditor::highlightEditedSample(int trackIndex)
{
    for (int i = 0; i < (int) sampleSlots.size(); ++i)
        sampleSlots[(size_t) i]->setSelected(i == trackIndex);
}
