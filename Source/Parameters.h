#pragma once
#include <JuceHeader.h>

namespace ParamIDs
{
    static const juce::String nudge       = "nudge";
    static const juce::String swing       = "swing";
    static const juce::String humanize    = "humanize";
    static const juce::String pitchRand   = "pitchRand";
    static const juce::String decay       = "decay";
    static const juce::String velocity    = "velocity";
    static const juce::String ratchet     = "ratchet";
    static const juce::String probability = "probability";
    static const juce::String masterVol   = "masterVolume";
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

inline const std::array<ParamInfo, 9>& getAllParamInfo()
{
    static const std::array<ParamInfo, 9> info = { {
        { ParamIDs::nudge,       "Nudge",       AccentGroup::timing },
        { ParamIDs::swing,       "Swing",       AccentGroup::timing },
        { ParamIDs::humanize,    "Humanize",    AccentGroup::timing },
        { ParamIDs::pitchRand,   "Pitch Rand",  AccentGroup::pitchSound },
        { ParamIDs::decay,       "Decay",       AccentGroup::pitchSound },
        { ParamIDs::velocity,    "Velocity",    AccentGroup::pitchSound },
        { ParamIDs::ratchet,     "Ratchet",     AccentGroup::ratchetChaos },
        { ParamIDs::probability, "Probability", AccentGroup::ratchetChaos },
        { ParamIDs::masterVol,   "Volume",      AccentGroup::pitchSound }
    } };
    return info;
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
        ParamIDs::pitchRand, "Pitch Rand",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.01f), 0.0f, "st"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::decay, "Decay",
        juce::NormalisableRange<float>(10.0f, 300.0f, 0.1f), 300.0f, "ms"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::velocity, "Velocity",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::ratchet, "Ratchet",
        juce::NormalisableRange<float>(1.0f, 4.0f, 1.0f), 1.0f, "x"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::probability, "Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::masterVol, "Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.8f, ""));

    return { params.begin(), params.end() };
}
