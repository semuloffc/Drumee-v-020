#include "PresetManager.h"
#include "PluginProcessor.h"

PresetManager::PresetManager(DrumeeAudioProcessor& processorToUse) : processor(processorToUse)
{
    getPresetFolder().createDirectory();
}

juce::File PresetManager::getPresetFolder()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("Reflexed")
                   .getChildFile("Drumee")
                   .getChildFile("Presets");
    return dir;
}

bool PresetManager::savePreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return false;

    juce::XmlElement root("DrumeePreset");

    if (auto paramsXml = processor.apvts.copyState().createXml())
        root.addChildElement(paramsXml.release());

    auto* patternElement = new juce::XmlElement("Pattern");
    for (int step = 0; step < kNumSteps; ++step)
    {
        auto* stepElement = new juce::XmlElement("Step");
        stepElement->setAttribute("index", step);
        for (int track = 0; track < kNumTracks; ++track)
        {
            stepElement->setAttribute("active" + juce::String(track), processor.sequencer.pattern[step].active[track]);
            stepElement->setAttribute("velocity" + juce::String(track), processor.sequencer.pattern[step].velocity[track]);
        }
        patternElement->addChildElement(stepElement);
    }
    root.addChildElement(patternElement);

    auto* samplesElement = new juce::XmlElement("Samples");
    for (int track = 0; track < kNumTracks; ++track)
    {
        auto* sampleElement = new juce::XmlElement("Sample");
        sampleElement->setAttribute("track", track);
        sampleElement->setAttribute("path", processor.tracks[(size_t) track].sourceFile.getFullPathName());
        sampleElement->setAttribute("name", processor.tracks[(size_t) track].name);
        samplesElement->addChildElement(sampleElement);
    }
    root.addChildElement(samplesElement);

    auto file = getPresetFolder().getChildFile(presetName + ".xml");
    bool ok = root.writeTo(file);
    if (ok)
        currentPresetName = presetName;
    return ok;
}

bool PresetManager::loadPreset(const juce::String& presetName)
{
    auto file = getPresetFolder().getChildFile(presetName + ".xml");
    if (! file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return false;

    for (auto* child : xml->getChildIterator())
    {
        if (child->getTagName() != "Pattern" && child->getTagName() != "Samples")
        {
            processor.apvts.replaceState(juce::ValueTree::fromXml(*child));
            break;
        }
    }

    if (auto* patternElement = xml->getChildByName("Pattern"))
    {
        for (auto* stepElement : patternElement->getChildIterator())
        {
            int step = stepElement->getIntAttribute("index");
            if (step < 0 || step >= kNumSteps)
                continue;

            for (int track = 0; track < kNumTracks; ++track)
            {
                processor.sequencer.pattern[(size_t) step].active[track] =
                    stepElement->getBoolAttribute("active" + juce::String(track));
                processor.sequencer.pattern[(size_t) step].velocity[track] =
                    (float) stepElement->getDoubleAttribute("velocity" + juce::String(track), 1.0);
            }
        }
    }

    if (auto* samplesElement = xml->getChildByName("Samples"))
    {
        for (auto* sampleElement : samplesElement->getChildIterator())
        {
            int track = sampleElement->getIntAttribute("track");
            juce::String path = sampleElement->getStringAttribute("path");
            if (track >= 0 && track < kNumTracks && path.isNotEmpty())
            {
                juce::File file2(path);
                if (file2.existsAsFile())
                    processor.loadSampleForTrack(track, file2);
            }
        }
    }

    currentPresetName = presetName;
    return true;
}

juce::StringArray PresetManager::getAllPresetNames() const
{
    juce::StringArray names;
    for (const auto& entry : juce::RangedDirectoryIterator(getPresetFolder(), false, "*.xml"))
        names.add(entry.getFile().getFileNameWithoutExtension());
    names.sort(true);
    return names;
}
