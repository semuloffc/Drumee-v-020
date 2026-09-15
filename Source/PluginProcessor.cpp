#include "PluginProcessor.h"
#include "PluginEditor.h"

DrumeeAudioProcessor::DrumeeAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      presetManager(*this)
{
    formatManager.registerBasicFormats();

    static const char* defaultNames[kNumTracks] = { "Kick", "Snare", "Clap", "Perc", "FX" };
    for (int i = 0; i < kNumTracks; ++i)
        tracks[(size_t) i].name = defaultNames[i];
}

DrumeeAudioProcessor::~DrumeeAudioProcessor() {}

void DrumeeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sequencer.prepare(sampleRate);
    scratchBuffer.setSize(2, samplesPerBlock);
}

void DrumeeAudioProcessor::releaseResources() {}

bool DrumeeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

bool DrumeeAudioProcessor::loadSampleForTrack(int trackIndex, const juce::File& file)
{
    if (trackIndex < 0 || trackIndex >= kNumTracks)
        return false;

    return tracks[(size_t) trackIndex].loadFile(file, formatManager);
}

void DrumeeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    hostIsPlaying = false;
    hostBpm = 120.0;

    if (auto* playHead = getPlayHead())
    {
        if (auto positionInfo = playHead->getPosition())
        {
            hostIsPlaying = positionInfo->getIsPlaying();
            if (auto bpm = positionInfo->getBpm())
                hostBpm = *bpm;
        }
    }

    float nudge = apvts.getRawParameterValue(ParamIDs::nudge)->load();
    float swing = apvts.getRawParameterValue(ParamIDs::swing)->load();
    float humanize = apvts.getRawParameterValue(ParamIDs::humanize)->load();
    float pitchRand = apvts.getRawParameterValue(ParamIDs::pitchRand)->load();
    float decay = apvts.getRawParameterValue(ParamIDs::decay)->load();
    float velocity = apvts.getRawParameterValue(ParamIDs::velocity)->load();
    float ratchet = apvts.getRawParameterValue(ParamIDs::ratchet)->load();
    float probability = apvts.getRawParameterValue(ParamIDs::probability)->load();
    float masterVolume = apvts.getRawParameterValue(ParamIDs::masterVol)->load();

    double outputSampleRate = getSampleRate();
    auto& tracksRef = tracks;

    sequencer.process(buffer.getNumSamples(), hostBpm, hostIsPlaying,
                       nudge, swing, humanize, pitchRand, velocity, ratchet, probability,
                       [&tracksRef, outputSampleRate, decay](int track, int sampleOffset, float vel, float pitchOffset)
                       {
                           tracksRef[(size_t) track].trigger(outputSampleRate, pitchOffset, vel, decay, sampleOffset);
                       });

    for (auto& track : tracks)
        track.renderNextBlock(buffer, 0, buffer.getNumSamples());

    buffer.applyGain(masterVolume);
}

juce::AudioProcessorEditor* DrumeeAudioProcessor::createEditor()
{
    return new DrumeeAudioProcessorEditor(*this);
}

void DrumeeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    auto* patternElement = new juce::XmlElement("Pattern");
    for (int step = 0; step < kNumSteps; ++step)
    {
        auto* stepElement = new juce::XmlElement("Step");
        stepElement->setAttribute("index", step);
        for (int track = 0; track < kNumTracks; ++track)
        {
            stepElement->setAttribute("active" + juce::String(track), sequencer.pattern[(size_t) step].active[track]);
            stepElement->setAttribute("velocity" + juce::String(track), sequencer.pattern[(size_t) step].velocity[track]);
        }
        patternElement->addChildElement(stepElement);
    }
    xml->addChildElement(patternElement);

    auto* samplesElement = new juce::XmlElement("Samples");
    for (int track = 0; track < kNumTracks; ++track)
    {
        auto* sampleElement = new juce::XmlElement("Sample");
        sampleElement->setAttribute("track", track);
        sampleElement->setAttribute("path", tracks[(size_t) track].sourceFile.getFullPathName());
        samplesElement->addChildElement(sampleElement);
    }
    xml->addChildElement(samplesElement);

    copyXmlToBinary(*xml, destData);
}

void DrumeeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml == nullptr)
        return;

    if (auto* paramsElement = xml->getChildByName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*paramsElement));

    if (auto* patternElement = xml->getChildByName("Pattern"))
    {
        for (auto* stepElement : patternElement->getChildIterator())
        {
            int step = stepElement->getIntAttribute("index");
            if (step < 0 || step >= kNumSteps)
                continue;

            for (int track = 0; track < kNumTracks; ++track)
            {
                sequencer.pattern[(size_t) step].active[track] =
                    stepElement->getBoolAttribute("active" + juce::String(track));
                sequencer.pattern[(size_t) step].velocity[track] =
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
                juce::File file(path);
                if (file.existsAsFile())
                    loadSampleForTrack(track, file);
            }
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumeeAudioProcessor();
}
