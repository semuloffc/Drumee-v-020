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

    // PITCH & SOUND no longer has a section on the main screen - it moved
    // into each sample's own window. Only Timing/Groove and Ratchet/Chaos
    // remain here, plus a small header over the sample row.
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
                if (sampleWindows[(size_t) trackIndex] != nullptr)
                    sampleWindows[(size_t) trackIndex]->getContent().refresh();
            }
        };
        slot->onEditRequested = [this](int trackIndex) { openSampleWindow(trackIndex); };
        addAndMakeVisible(*slot);
        sampleSlots.push_back(std::move(slot));
    }

    // Create every sample's own window up front (hidden). Each one owns a
    // unique, independent set of PITCH & SOUND controls for that sample.
    for (int i = 0; i < kNumTracks; ++i)
    {
        auto window = std::make_unique<SampleEditorWindow>(i, processor.apvts, processor.tracks[(size_t) i], lookAndFeel);
        window->getContent().onLoadRequested = [this](int trackIndex) { loadSample(trackIndex); };
        window->getContent().onFileDropped = [this](int trackIndex, const juce::File& file)
        {
            if (processor.loadSampleForTrack(trackIndex, file) && trackIndex >= 0 && trackIndex < (int) sampleSlots.size())
            {
                sampleSlots[(size_t) trackIndex]->refresh();
                if (sampleWindows[(size_t) trackIndex] != nullptr)
                    sampleWindows[(size_t) trackIndex]->getContent().refresh();
            }
        };
        sampleWindows[(size_t) i] = std::move(window);
    }

    refreshPresetList();

    // setSize() triggers resized() synchronously, so it must come after every
    // child component (encoders, visualizer, sample slots) has been created -
    // otherwise resized() dereferences still-null pointers and crashes.
    setSize(1040, 820);
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
    constexpr int kSampleRowHeight = 148;
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
    g.setColour(DrumeeColours::panel);
    g.fillRoundedRectangle(timingCardBounds.toFloat(), 6.0f);
    g.fillRoundedRectangle(chaosCardBounds.toFloat(), 6.0f);
    g.setColour(DrumeeColours::outline);
    g.drawRoundedRectangle(timingCardBounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
    g.drawRoundedRectangle(chaosCardBounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
}

void DrumeeAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(kMargin);

    auto topBar = bounds.removeFromTop(kTopBarHeight).reduced(6, 0);
    auto titleArea = topBar.removeFromLeft(160);
    titleLabel.setBounds(titleArea.removeFromTop(26));
    versionLabel.setBounds(titleArea);
    newButton.setBounds(topBar.removeFromRight(60).reduced(0, 6));
    topBar.removeFromRight(6);
    saveButton.setBounds(topBar.removeFromRight(70).reduced(0, 6));
    topBar.removeFromRight(6);
    presetBox.setBounds(topBar.removeFromRight(180).reduced(0, 6));

    bounds.removeFromTop(10);

    // Bottom row: SAMPLES header + the five sample cards. Each card now
    // also carries an Edit button that opens that sample's own window.
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
    // card (right).
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
            case AccentGroup::pitchSound:   break; // lives in the per-sample windows now
        }
    }

    timingCardBounds = leftColumn;
    chaosCardBounds = rightColumn;

    constexpr int encoderHeight = 140;
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

    if (visualizer != nullptr)
        visualizer->setBounds(bounds);
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
    for (auto& window : sampleWindows)
        if (window != nullptr)
            window->getContent().refresh();
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
            if (trackIndex >= 0 && trackIndex < (int) sampleWindows.size() && sampleWindows[(size_t) trackIndex] != nullptr)
                sampleWindows[(size_t) trackIndex]->getContent().refresh();
        }
    });
}

void DrumeeAudioProcessorEditor::openSampleWindow(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= (int) sampleWindows.size() || sampleWindows[(size_t) trackIndex] == nullptr)
        return;

    auto* window = sampleWindows[(size_t) trackIndex].get();

    if (! window->isVisible())
    {
        // Cascade each sample's window from the main plugin window so
        // several can be open at once without landing exactly on top of
        // one another.
        auto mainBounds = getScreenBounds();
        int offset = trackIndex * 28;
        window->setTopLeftPosition(mainBounds.getRight() + 24 + offset, mainBounds.getY() + offset);
        window->setVisible(true);
    }

    window->toFront(true);
}
