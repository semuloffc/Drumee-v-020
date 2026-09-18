#include "DSP.h"

void SamplePlayerVoice::start(const juce::AudioBuffer<float>* buf, double sourceSampleRate,
                               double outputSampleRate, float pitchSemitones, float gain,
                               float attackMs, float decayMs, float sustainLevel, float releaseMs,
                               float attackCurve, float decayCurve, float releaseCurve,
                               int startDelaySamples)
{
    sourceBuffer = buf;
    position = 0.0;
    ratio = std::pow(2.0, pitchSemitones / 12.0) * (sourceSampleRate / outputSampleRate);
    gainLevel = gain;
    delaySamples = juce::jmax(0, startDelaySamples);
    envElapsedSamples = 0.0;

    envAttackSamples  = juce::jmax(0.0, (double) attackMs / 1000.0 * outputSampleRate);
    envDecaySamples   = juce::jmax(0.0, (double) decayMs / 1000.0 * outputSampleRate);
    envReleaseSamples = juce::jmax(1.0, (double) releaseMs / 1000.0 * outputSampleRate);
    envSustainLevel   = juce::jlimit(0.0, 1.0, (double) sustainLevel);
    envAttackCurve  = attackCurve;
    envDecayCurve   = decayCurve;
    envReleaseCurve = releaseCurve;

    int srcLength = sourceBuffer != nullptr ? sourceBuffer->getNumSamples() : 0;
    double playbackDurationSamples = (ratio > 0.0 && srcLength > 1)
        ? (double) (srcLength - 1) / ratio
        : 0.0;

    // Release lands at the natural end of the sample by default; if the
    // sample is too short to fit Attack+Decay+Release, Release simply
    // starts as soon as Decay ends instead (its tail gets cut short by the
    // sample running out, same as any other stage would).
    envReleaseStartSample = juce::jmax(envAttackSamples + envDecaySamples,
                                        playbackDurationSamples - envReleaseSamples);

    isActive = sourceBuffer != nullptr && srcLength > 0;
}

// Attack ramps 0 -> 1, Decay shapes 1 -> sustain level, the level then
// holds at Sustain until Release shapes it back down to 0. Each stage's
// shape is controlled by its own tension value (Serum-style: 0 linear,
// positive convex, negative concave - see envelopeTensionCurve()), so the
// audible envelope always matches the curve dragged on screen. Elapsed
// time is measured in real output samples (not resampled playback
// position) so the envelope timing stays correct regardless of pitch shifting.
double SamplePlayerVoice::envelopeGainAt(double elapsedSamples) const
{
    if (elapsedSamples < envAttackSamples)
    {
        double frac = envAttackSamples > 0.0 ? elapsedSamples / envAttackSamples : 1.0;
        return (double) envelopeTensionCurve((float) frac, envAttackCurve);
    }

    if (elapsedSamples < envAttackSamples + envDecaySamples)
    {
        double local = elapsedSamples - envAttackSamples;
        double frac = envDecaySamples > 0.0 ? local / envDecaySamples : 1.0;
        // Tension warps the progress first, then the existing smoothstep
        // ease is applied on top - at curve == 0 this reduces exactly to
        // the pre-0.2.7 smoothstep shape, so old presets sound unchanged.
        double warped = (double) envelopeTensionCurve((float) frac, envDecayCurve);
        double shaped = warped * warped * (3.0 - 2.0 * warped);
        return 1.0 + (envSustainLevel - 1.0) * shaped;
    }

    if (elapsedSamples < envReleaseStartSample)
        return envSustainLevel;

    double local = elapsedSamples - envReleaseStartSample;
    double frac = juce::jlimit(0.0, 1.0, envReleaseSamples > 0.0 ? local / envReleaseSamples : 1.0);
    double warped = (double) envelopeTensionCurve((float) frac, envReleaseCurve);
    double shaped = warped * warped * (3.0 - 2.0 * warped);
    return envSustainLevel * (1.0 - shaped);
}

void SamplePlayerVoice::renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples)
{
    if (! isActive || sourceBuffer == nullptr)
        return;

    const int srcChannels = sourceBuffer->getNumChannels();
    const int srcLength = sourceBuffer->getNumSamples();
    const int outChannels = output.getNumChannels();

    if (srcChannels <= 0 || srcLength <= 0 || outChannels <= 0)
    {
        isActive = false;
        return;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        if (delaySamples > 0)
        {
            --delaySamples;
            continue;
        }

        if (position >= (double) (srcLength - 1))
        {
            isActive = false;
            break;
        }

        if (envElapsedSamples >= envReleaseStartSample + envReleaseSamples)
        {
            isActive = false;
            break;
        }

        int idx0 = (int) position;
        int idx1 = juce::jmin(idx0 + 1, srcLength - 1);
        float frac = (float) (position - (double) idx0);

        double envGain = envelopeGainAt(envElapsedSamples);

        for (int ch = 0; ch < outChannels; ++ch)
        {
            int srcCh = juce::jmin(ch, srcChannels - 1);
            float s0 = sourceBuffer->getSample(srcCh, idx0);
            float s1 = sourceBuffer->getSample(srcCh, idx1);
            float sample = s0 + (s1 - s0) * frac;
            output.addSample(ch, startSample + i, sample * gainLevel * (float) envGain);
        }

        position += ratio;
        envElapsedSamples += 1.0;
    }
}

bool SampleTrack::loadFile(const juce::File& file, juce::AudioFormatManager& formatManager)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return false;

    buffer.setSize((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read(&buffer, 0, (int) reader->lengthInSamples, 0, true, true);
    sourceSampleRate = reader->sampleRate;
    sourceFile = file;
    loaded = true;
    return true;
}

void SampleTrack::trigger(double outputSampleRate, float pitchSemitones, float velocityGain,
                           float attackMs, float decayMs, float sustainLevel, float releaseMs,
                           float attackCurve, float decayCurve, float releaseCurve,
                           int startDelaySamples)
{
    if (! loaded)
        return;

    voices[nextVoice].start(&buffer, sourceSampleRate, outputSampleRate, pitchSemitones, velocityGain,
                             attackMs, decayMs, sustainLevel, releaseMs,
                             attackCurve, decayCurve, releaseCurve, startDelaySamples);
    nextVoice = (nextVoice + 1) % kMaxVoicesPerTrack;
    triggerCount.fetch_add(1, std::memory_order_relaxed);
}

void SampleTrack::renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples)
{
    for (auto& voice : voices)
        if (voice.active())
            voice.renderNextBlock(output, startSample, numSamples);
}

void Sequencer::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    phase = 0.0;
    currentStep = 0;
    wasPlaying = false;
}

void Sequencer::reset()
{
    phase = 0.0;
    currentStep = 0;
    wasPlaying = false;
}

void Sequencer::process(int numSamples, double bpm, bool isPlaying,
                         float nudgeMs, float swingPct, float humanizePct,
                         const std::array<float, kNumTracks>& pitchRandSt,
                         const std::array<float, kNumTracks>& velocityDepth,
                         float ratchetAmount, float probabilityPct, const TriggerCallback& callback)
{
    if (! isPlaying || bpm <= 0.0)
    {
        wasPlaying = false;
        return;
    }

    double samplesPerStep = (60.0 / bpm / 4.0) * sampleRate;
    if (samplesPerStep < 1.0)
        return;

    int processed = 0;

    // Transport just started (or restarted): snap to step 0 and fire it
    // immediately at sample 0, instead of waiting a full step length before
    // the first hit is ever heard.
    if (! wasPlaying)
    {
        wasPlaying = true;
        phase = 0.0;
        currentStep.store(0);
        fireStep(0, 0, samplesPerStep, nudgeMs, swingPct, humanizePct,
                 pitchRandSt, velocityDepth, ratchetAmount, probabilityPct, numSamples, callback);
    }

    while (processed < numSamples)
    {
        double remaining = samplesPerStep - phase;
        int samplesLeftInBlock = numSamples - processed;

        if (remaining > (double) samplesLeftInBlock)
        {
            phase += (double) samplesLeftInBlock;
            processed = numSamples;
            break;
        }

        int advance = (int) std::ceil(remaining);
        advance = juce::jmin(advance, samplesLeftInBlock);
        processed += advance;
        phase = 0.0;

        // Fire the step that is *starting* right here (not the one that just
        // ended), so the audible hit lines up with the step that lights up
        // in the UI and with the host's beat grid.
        int nextStep = (currentStep.load() + 1) % kNumSteps;
        currentStep.store(nextStep);
        fireStep(nextStep, processed, samplesPerStep, nudgeMs, swingPct, humanizePct,
                 pitchRandSt, velocityDepth, ratchetAmount, probabilityPct, numSamples, callback);
    }
}

void Sequencer::fireStep(int step, int offsetInBlock, double samplesPerStep,
                          float nudgeMs, float swingPct, float humanizePct,
                          const std::array<float, kNumTracks>& pitchRandSt,
                          const std::array<float, kNumTracks>& velocityDepth,
                          float ratchetAmount, float probabilityPct,
                          int blockSize, const TriggerCallback& callback)
{
    stepFlash[step] = true;

    for (int track = 0; track < kNumTracks; ++track)
    {
        if (! pattern[step].active[track])
            continue;

        float roll = random.nextFloat() * 100.0f;
        if (roll > probabilityPct)
            continue;

        double nudgeSamples = (nudgeMs / 1000.0) * sampleRate;
        double swingSamples = (step % 2 == 1) ? (swingPct / 100.0) * (samplesPerStep * 0.5) : 0.0;
        double humanizeSamples = (humanizePct / 100.0) * (random.nextDouble() * 2.0 - 1.0) * (samplesPerStep * 0.25);
        double totalOffset = nudgeSamples + swingSamples + humanizeSamples;

        int ratchetCount = juce::jlimit(1, 4, (int) std::round(ratchetAmount));
        double subStep = samplesPerStep / (double) ratchetCount;

        float trackPitchRand = pitchRandSt[(size_t) track];
        float trackVelocityDepth = velocityDepth[(size_t) track];

        for (int r = 0; r < ratchetCount; ++r)
        {
            double pos = (double) offsetInBlock + totalOffset + subStep * (double) r;
            int samplePos = juce::jlimit(0, blockSize - 1, (int) std::round(pos));

            float pitchOffset = trackPitchRand > 0.0f
                ? (random.nextFloat() * 2.0f - 1.0f) * trackPitchRand
                : 0.0f;

            float baseVelocity = pattern[step].velocity[track];
            float velocity = juce::jmap(trackVelocityDepth / 100.0f, 0.0f, 1.0f, 1.0f, baseVelocity);
            velocity = juce::jlimit(0.05f, 1.0f, velocity - (float) r * 0.08f);

            callback(track, samplePos, velocity, pitchOffset);
        }
    }
}

void MasterBus::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void MasterBus::reset()
{
    previousVolumeGain = 1.0f;
    limiterEnvelope = 1.0f;
}

// Volume is applied first (ramped across the block to avoid zipper noise
// when the knob moves), then the limiter looks at the true post-volume
// peak on every sample and reins it in if it exceeds the ceiling - fast
// enough attack to catch one-shot transients, slow enough release to
// avoid audible pumping between hits.
void MasterBus::process(juce::AudioBuffer<float>& buffer, float volumeGain, float limiterCeilingDb)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
        return;

    for (int ch = 0; ch < numChannels; ++ch)
        buffer.applyGainRamp(ch, 0, numSamples, previousVolumeGain, volumeGain);
    previousVolumeGain = volumeGain;

    const float ceilingLin = juce::Decibels::decibelsToGain(limiterCeilingDb);
    const float attackCoeff  = (float) std::exp(-1.0 / (0.001 * sampleRate));
    const float releaseCoeff = (float) std::exp(-1.0 / (0.100 * sampleRate));

    for (int i = 0; i < numSamples; ++i)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            peak = juce::jmax(peak, std::abs(buffer.getSample(ch, i)));

        float targetGain = (peak > ceilingLin && peak > 0.0f) ? (ceilingLin / peak) : 1.0f;

        limiterEnvelope = targetGain < limiterEnvelope
            ? targetGain + (limiterEnvelope - targetGain) * attackCoeff
            : targetGain + (limiterEnvelope - targetGain) * releaseCoeff;
        limiterEnvelope = juce::jlimit(0.0f, 1.0f, limiterEnvelope);

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample(ch, i, buffer.getSample(ch, i) * limiterEnvelope);
    }
}
