#pragma once
#include <JuceHeader.h>
#include <array>
#include "DSP.h"

// ---------------------------------------------------------------------------
// Global parameters (shown on the main screen: TIMING & GROOVE, RATCHET &
// CHAOS). PITCH & SOUND used to live here too, but as of 0.2.2 those
// controls became per-sample and moved into the internal sample editor panel -
// see PitchSoundParamIDs/EnvelopeParamIDs and getPitchSoundParamInfoForTrack/
// getEnvelopeParamInfoForTrack below.
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
// Every track gets its own independent Pitch Rand / Velocity plus its own
// ADSR envelope (Attack / Decay / Sustain / Release / Volume), addressed as
// base id + track index, e.g. "decay0" .. "decay4".
// ---------------------------------------------------------------------------
namespace PitchSoundParamIDs
{
    static const juce::String pitchRand = "pitchRand";
    static const juce::String decay     = "decay";
    static const juce::String velocity  = "velocity";
    static const juce::String volume    = "volume";
}

// 0.2.5: per-sample amplitude envelope. Decay and Volume above are reused
// as the ADSR's Decay stage and output level - only Attack/Sustain/Release
// are new IDs - so existing "decayN"/"volumeN" automation keeps working.
namespace EnvelopeParamIDs
{
    static const juce::String attack  = "attack";
    static const juce::String sustain = "sustain";
    static const juce::String release = "release";
}

// 0.2.7: Serum-style segment shaping. Not shown as knobs - dragged directly
// on the Attack/Decay/Release segments of the envelope graph, same as the
// breakpoints. -1..1, 0 = linear (identical to the pre-0.2.7 shape).
namespace EnvelopeCurveParamIDs
{
    static const juce::String attackCurve  = "attackCurve";
    static const juce::String decayCurve   = "decayCurve";
    static const juce::String releaseCurve = "releaseCurve";
}

// 0.2.8: per-track Mute/Solo, toggled from the compact sample cards on the
// main screen. Plain bools rather than knobs - no ParamInfo/AccentGroup
// entry, since they are never shown as an Encoder.
namespace MuteSoloParamIDs
{
    static const juce::String mute = "mute";
    static const juce::String solo = "solo";
}

// 0.2.8: master bus, applied once after all tracks are summed (see
// MasterBus in DSP.h/.cpp) - overall output Volume plus a Limiter ceiling.
namespace MasterParamIDs
{
    static const juce::String volume  = "masterVolume";
    static const juce::String limiter = "masterLimiter";
}

// 0.2.9: per-track Reverse, toggled from that sample's page in the editor
// panel. A plain bool, like Mute/Solo - no ParamInfo/AccentGroup entry,
// since it is shown as a toggle rather than an Encoder.
namespace ReverseParamIDs
{
    static const juce::String reverse = "reverse";
}

// Shared with the interactive envelope graph in GUI.cpp so the knob ranges
// and the on-screen point positions can never drift apart.
namespace EnvelopeRanges
{
    static constexpr float attackMinMs  = 0.0f;
    static constexpr float attackMaxMs  = 500.0f;
    static constexpr float decayMinMs   = 1.0f;
    static constexpr float decayMaxMs   = 2000.0f;
    static constexpr float releaseMinMs = 1.0f;
    static constexpr float releaseMaxMs = 2000.0f;
}

inline juce::String perTrackParamID(const juce::String& base, int trackIndex)
{
    return base + juce::String(trackIndex);
}

enum class AccentGroup
{
    timing,
    pitchSound,
    ratchetChaos,
    envelope,
    master
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

// The Pitch & Sound controls, expanded to their concrete per-track
// parameter IDs for a given sample/track index. Used to build the encoders
// on that sample's page in the editor panel. Decay and Volume moved out of
// this group in 0.2.5 - they are now part of the ADSR envelope group below.
inline std::array<ParamInfo, 2> getPitchSoundParamInfoForTrack(int trackIndex)
{
    return { {
        { perTrackParamID(PitchSoundParamIDs::pitchRand, trackIndex), "Pitch Rand", AccentGroup::pitchSound },
        { perTrackParamID(PitchSoundParamIDs::velocity,  trackIndex), "Velocity",   AccentGroup::pitchSound }
    } };
}

// The five ENVELOPE controls for a given sample/track index: the four ADSR
// stages plus overall Volume, all shown next to that sample's interactive
// envelope graph on its page in the editor panel.
inline std::array<ParamInfo, 5> getEnvelopeParamInfoForTrack(int trackIndex)
{
    return { {
        { perTrackParamID(EnvelopeParamIDs::attack,   trackIndex), "Attack",  AccentGroup::envelope },
        { perTrackParamID(PitchSoundParamIDs::decay,  trackIndex), "Decay",   AccentGroup::envelope },
        { perTrackParamID(EnvelopeParamIDs::sustain,  trackIndex), "Sustain", AccentGroup::envelope },
        { perTrackParamID(EnvelopeParamIDs::release,  trackIndex), "Release", AccentGroup::envelope },
        { perTrackParamID(PitchSoundParamIDs::volume, trackIndex), "Volume",  AccentGroup::envelope }
    } };
}

// The two MASTER controls (Volume/Limiter) shown next to the Ratchet &
// Chaos knobs on the main screen.
inline const std::array<ParamInfo, 2>& getMasterParamInfo()
{
    static const std::array<ParamInfo, 2> info = { {
        { MasterParamIDs::volume,  "Volume",  AccentGroup::master },
        { MasterParamIDs::limiter, "Limiter", AccentGroup::master }
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
        ParamIDs::ratchet, "Ratchet",
        juce::NormalisableRange<float>(1.0f, 4.0f, 1.0f), 1.0f, "x"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::probability, "Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f, "%"));

    // 0.2.8: master bus, sits after all tracks are summed (see MasterBus).
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        MasterParamIDs::volume, "Master Volume",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        MasterParamIDs::limiter, "Master Limiter",
        juce::NormalisableRange<float>(-24.0f, 0.0f, 0.1f), 0.0f, "dB"));

    // Per-track Pitch & Sound + Envelope parameters: 7 controls x kNumTracks samples.
    for (int t = 0; t < kNumTracks; ++t)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(PitchSoundParamIDs::pitchRand, t), "Pitch Rand " + juce::String(t + 1),
            juce::NormalisableRange<float>(0.0f, 12.0f, 0.01f), 0.0f, "st"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(PitchSoundParamIDs::decay, t), "Decay " + juce::String(t + 1),
            juce::NormalisableRange<float>(EnvelopeRanges::decayMinMs, EnvelopeRanges::decayMaxMs, 0.1f), 300.0f, "ms"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(PitchSoundParamIDs::velocity, t), "Velocity " + juce::String(t + 1),
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f, "%"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(PitchSoundParamIDs::volume, t), "Volume " + juce::String(t + 1),
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.8f, ""));

        // 0.2.5: Attack/Sustain/Release complete the per-sample envelope.
        // Defaults (attack 0, sustain 0) keep the pre-0.2.5 punchy one-shot
        // decay-to-silence behaviour unless the user reshapes the envelope.
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(EnvelopeParamIDs::attack, t), "Attack " + juce::String(t + 1),
            juce::NormalisableRange<float>(EnvelopeRanges::attackMinMs, EnvelopeRanges::attackMaxMs, 0.1f), 0.0f, "ms"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(EnvelopeParamIDs::sustain, t), "Sustain " + juce::String(t + 1),
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f, "%"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(EnvelopeParamIDs::release, t), "Release " + juce::String(t + 1),
            juce::NormalisableRange<float>(EnvelopeRanges::releaseMinMs, EnvelopeRanges::releaseMaxMs, 0.1f), 40.0f, "ms"));

        // 0.2.7: per-segment curve shape, dragged directly on the graph
        // (Serum-style). Default 0 = linear/smoothstep, i.e. identical to
        // the pre-0.2.7 shape - existing presets are unaffected until the
        // curve is dragged.
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(EnvelopeCurveParamIDs::attackCurve, t), "Attack Curve " + juce::String(t + 1),
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f, ""));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(EnvelopeCurveParamIDs::decayCurve, t), "Decay Curve " + juce::String(t + 1),
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f, ""));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            perTrackParamID(EnvelopeCurveParamIDs::releaseCurve, t), "Release Curve " + juce::String(t + 1),
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f, ""));

        // 0.2.8: per-track Mute/Solo, toggled from the compact sample cards.
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            perTrackParamID(MuteSoloParamIDs::mute, t), "Mute " + juce::String(t + 1), false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            perTrackParamID(MuteSoloParamIDs::solo, t), "Solo " + juce::String(t + 1), false));

        // 0.2.9: per-track Reverse, toggled from that sample's page in the
        // editor panel - flips the sample's playback direction.
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            perTrackParamID(ReverseParamIDs::reverse, t), "Reverse " + juce::String(t + 1), false));
    }

    return { params.begin(), params.end() };
}
