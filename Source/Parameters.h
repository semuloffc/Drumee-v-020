#pragma once
#include <JuceHeader.h>
#include <array>
#include "DSP.h"

// ---------------------------------------------------------------------------
// Global parameters (shown on the main screen: TIMING & GROOVE, RATCHET &
// CHAOS). PITCH & SOUND used to live here too, but as of 0.2.2 those four
// controls became per-sample and moved into the internal sample editor panel -
// see PitchSoundParamIDs / getPitchSoundParamInfoForTrack below.
// ---------------------------------------------------------------------------
namespace ParamIDs
{
    static const juce::String nudge       = "nudge";
    static const juce::String swing       = "swing";
    static const juce::String humanize    = "humanize";
    static const juce::String ratchet     = "ratchet";
    static const juce::String probability = "probability";
}

// ---------------------------------------------------------------------------
// Per-sample parameters (shown only on that sample's page in the editor panel).
// Every track gets its own independent Pitch Rand / Decay / Velocity /
// Volume, addressed as base id + track index, e.g. "decay0" .. "decay4".
// ---------------------------------------------------------------------------
namespace PitchSoundParamIDs
{
    static const juce::String pitchRand = "pitchRand";
    static const juce::String decay     = "decay";
    static const juce::String velocity  = "velocity";
    static const juce::String volume    = "volume";
}

inline juce::String perTrackParamID(const juce::String& base, int trackIndex)
{
    return base + juce::String(trackIndex);
}

enum class AccentGroup
{
    timing,
    pitchSound,
    ratchetChaos
};

struct ParamInfo
{
    juce::String id;
    juce::String label;
    AccentGroup group;
};

// Main-screen parameters only (Timing/Groove + Ratchet/Chaos).
inline const std::array<ParamInfo, 5>& getAllParamInfo()
{
    static const std::array<ParamInfo, 5> info = { {
        { ParamIDs::nudge,       "Nudge",       AccentGroup::timing },
        { ParamIDs::swing,       "Swing",       AccentGroup::timing },
        { ParamIDs::humanize,    "Humanize",    AccentGroup::timing },
        { ParamIDs::ratchet,     "Ratchet",     AccentGroup::ratchetChaos },
        { ParamIDs::probability, "Probability", AccentGroup::ratchetChaos }
    } };
    return info;
}

// The four Pitch & Sound controls, expanded to their concrete per-track
// parameter IDs for a given sample/track index. Used to build the encoders
// on that sample's page in the editor panel.
inline std::array<ParamInfo, 4> getPitchSoundParamInfoForTrack(int trackIndex)
{
    return { {
        { perTrackParamID(PitchSoundParamIDs::pitchRand, trackIndex), "Pitch Rand", AccentGroup::pitchSound },
        { perTrackParamID(PitchSoundParamIDs::decay,     trackIndex), "Decay",      AccentGroup::pitchSound },
        { perTrackParamID(PitchSoundParamIDs::velocity,  trackIndex), "Velocity",   AccentGroup::pitchSound },
        { perTrackParamID(PitchSoundParamIDs::volume,    trackIndex), "Volume",     AccentGroup::pitchSound }
    } };
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::nudge, "Nudge",
        juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f), 0.0f, "ms"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::swing, "Swing",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::humanize, "Humanize",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::ratchet, "Ratchet",
        juce::NormalisableRange<float>(1.0f, 4.0f, 1.0f), 1.0f, "x"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::probability, "Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f, "%"));

    // Per-track Pitch & Sound parameters: 4 controls x kNumTracks samples.
    for (int t = 0; t < kNumTracks; ++t)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(PitchSoundParamIDs::pitchRand, t), "Pitch Rand " + juce::String(t + 1),
            juce::NormalisableRange<float>(0.0f, 12.0f, 0.01f), 0.0f, "st"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(PitchSoundParamIDs::decay, t), "Decay " + juce::String(t + 1),
            juce::NormalisableRange<float>(10.0f, 300.0f, 0.1f), 300.0f, "ms"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(PitchSoundParamIDs::velocity, t), "Velocity " + juce::String(t + 1),
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f, "%"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(PitchSoundParamIDs::volume, t), "Volume " + juce::String(t + 1),
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.8f, ""));
    }

    return { params.begin(), params.end() };
}
