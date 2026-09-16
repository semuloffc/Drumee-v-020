#include "DSP.h"

void SamplePlayerVoice::start(const juce::AudioBuffer<float>* buf, double sourceSampleRate,
                               double outputSampleRate, float pitchSemitones, float gain, float decayMs,
                               int startDelaySamples)
{
    sourceBuffer = buf;
    position = 0.0;
    ratio = std::pow(2.0, pitchSemitones / 12.0) * (sourceSampleRate / outputSampleRate);
    gainLevel = gain;
    envelope = 1.0;
    delaySamples = juce::jmax(0, startDelaySamples);

    double decaySamples = juce::jmax(1.0, (decayMs / 1000.0) * outputSampleRate);
    envelopeDecayPerSample = std::pow(0.0005, 1.0 / decaySamples);
    isActive = sourceBuffer != nullptr && sourceBuffer->getNumSamples() > 0;
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

        int idx0 = (int) position;
        int idx1 = juce::jmin(idx0 + 1, srcLength - 1);
        float frac = (float) (position - (double) idx0);

        for (int ch = 0; ch < outChannels; ++ch)
        {
            int srcCh = juce::jmin(ch, srcChannels - 1);
            float s0 = sourceBuffer->getSample(srcCh, idx0);
            float s1 = sourceBuffer->getSample(srcCh, idx1);
            float sample = s0 + (s1 - s0) * frac;
            output.addSample(ch, startSample + i, sample * gainLevel * (float) envelope);
        }

        position += ratio;
        envelope *= envelopeDecayPerSample;

        if (envelope < 0.0002)
        {
            isActive = false;
            break;
        }
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

void SampleTrack::trigger(double outputSampleRate, float pitchSemitones, float velocityGain, float decayMs, int startDelaySamples)
{
    if (! loaded)
        return;

    voices[nextVoice].start(&buffer, sourceSampleRate, outputSampleRate, pitchSemitones, velocityGain, decayMs, startDelaySamples);
    nextVoice = (nextVoice + 1) % kMaxVoicesPerTrack;
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
