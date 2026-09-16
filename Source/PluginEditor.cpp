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

    for (auto* label : { &sectionTiming, &sectionPitch, &sectionChaos })
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
        auto slot = std::make_unique<SampleSlotComponent>(i, processor.tracks[(size_t) i]);
        slot->onLoadRequested = [this](int trackIndex) { loadSample(trackIndex); };
        slot->onFileDropped = [this](int trackIndex, const juce::File& file)
        {
            if (processor.loadSampleForTrack(trackIndex, file)
                && trackIndex >= 0 && trackIndex < (int) sampleSlots.size())
                sampleSlots[(size_t) trackIndex]->refresh();
        };
        addAndMakeVisible(*slot);
        sampleSlots.push_back(std::move(slot));
    }

    refreshPresetList();

    // setSize() triggers resized() synchronously, so it must come after every
    // child component (encoders, visualizer, sample slots) has been created -
    // otherwise resized() dereferences still-null pointers and crashes.
    setSize(960, 540);
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

    auto bottomBar = bounds.removeFromBottom(74);
    int slotWidth = bottomBar.getWidth() / kNumTracks;
    for (auto& slot : sampleSlots)
    {
        slot->setBounds(bottomBar.removeFromLeft(slotWidth).reduced(4));
    }

    bounds.removeFromBottom(10);

    auto leftColumn = bounds.removeFromLeft(170);
    auto rightColumn = bounds.removeFromRight(220);
    bounds.removeFromLeft(10);
    bounds.removeFromRight(10);

    std::vector<Encoder*> timingEncoders, pitchEncoders, chaosEncoders;
    auto& infoArray = getAllParamInfo();
    for (size_t i = 0; i < encoders.size(); ++i)
    {
        switch (infoArray[i].group)
        {
            case AccentGroup::timing:       timingEncoders.push_back(encoders[i].get()); break;
            case AccentGroup::pitchSound:   pitchEncoders.push_back(encoders[i].get()); break;
            case AccentGroup::ratchetChaos: chaosEncoders.push_back(encoders[i].get()); break;
        }
    }

    sectionTiming.setBounds(leftColumn.removeFromTop(20));
    leftColumn.removeFromTop(4);
    for (auto* enc : timingEncoders)
    {
        enc->setBounds(leftColumn.removeFromTop(110));
        leftColumn.removeFromTop(8);
    }

    sectionPitch.setBounds(rightColumn.removeFromTop(20));
    rightColumn.removeFromTop(4);
    auto pitchGrid = rightColumn.removeFromTop(220);
    int pitchColWidth = pitchGrid.getWidth() / 2;
    int pitchRowHeight = pitchGrid.getHeight() / 2;
    for (size_t i = 0; i < pitchEncoders.size(); ++i)
    {
        int row = (int) i / 2;
        int col = (int) i % 2;
        juce::Rectangle<int> cell(pitchGrid.getX() + col * pitchColWidth,
                                   pitchGrid.getY() + row * pitchRowHeight,
                                   pitchColWidth, pitchRowHeight);
        pitchEncoders[i]->setBounds(cell.reduced(4));
    }

    rightColumn.removeFromTop(6);
    sectionChaos.setBounds(rightColumn.removeFromTop(20));
    rightColumn.removeFromTop(4);
    int chaosColWidth = rightColumn.getWidth() / 2;
    for (auto* enc : chaosEncoders)
        enc->setBounds(rightColumn.removeFromLeft(chaosColWidth).reduced(4));

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
        }
    });
}
