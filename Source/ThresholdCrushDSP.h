#pragma once

#include <JuceHeader.h>

// DSP core:
// - Clean passthrough at/below threshold.
// - Above threshold: overshoot (dB) drives bit-depth reduction.
// - Attack time smooths the detector envelope like a compressor.
class ThresholdCrushDSP
{
public:
    void setSampleRate (double newSampleRate)
    {
        sampleRate = (newSampleRate > 0.0 ? newSampleRate : 44100.0);
        reset();
        updateTimeConstants();
    }

    void reset()
    {
        env = 0.0f;
        holdCounter = 0;
        heldL = 0.0f;
        heldR = 0.0f;
    }

    void setThresholdDb (float db) { thresholdDb = db; }
    void setAttackMs (float ms)
    {
        attackMs = ms;
        updateTimeConstants();
    }

    void setReleaseMs (float ms)
    {
        releaseMs = ms;
        updateTimeConstants();
    }

    void setCrushRangeDb (float db) { crushRangeDb = juce::jmax (1.0f, db); }
    void setBitDepthRange (int minDepth, int maxDepth)
    {
        minBitDepth = juce::jlimit (2, 24, minDepth);
        maxBitDepth = juce::jlimit (2, 24, maxDepth);
        if (minBitDepth > maxBitDepth)
            std::swap (minBitDepth, maxBitDepth);
    }

    void setDownsampleMax (int maxFactor)
    {
        downsampleMax = juce::jlimit (1, 64, maxFactor);
    }

    void setMix (float mix01) { mix = juce::jlimit (0.0f, 1.0f, mix01); }

    void setClipEnabled (bool enabled) { clipEnabled = enabled; }
    void setClipDriveDb (float db) { clipDriveDb = juce::jlimit (0.0f, 24.0f, db); }
    void setClipStyle01 (float style01) { clipStyle = juce::jlimit (0.0f, 1.0f, style01); }

    float getEnvelopeDb() const
    {
        return juce::Decibels::gainToDecibels (env + epsilon);
    }

    // Stereo-linked process: detector is max(|L|, |R|). If mono, pass same value for both.
    inline void processSample (float& l, float& r)
    {
        const float inL = l;
        const float inR = r;

        const float peak = juce::jmax (std::abs (inL), std::abs (inR));
        const float coeff = (peak > env ? attackCoeff : releaseCoeff);
        env = coeff * env + (1.0f - coeff) * peak;

        const float envDb = juce::Decibels::gainToDecibels (env + epsilon);
        const float overshootDb = juce::jmax (0.0f, envDb - thresholdDb);

        if (overshootDb <= 0.0f)
        {
            // Ensure we don't smear held samples when re-entering the crushing state.
            holdCounter = 0;
            l = inL;
            r = inR;
            return;
        }

        const float crushAmount = juce::jlimit (0.0f, 1.0f, overshootDb / crushRangeDb);

        // Downsample-hold: at full overshoot, hold up to downsampleMax samples.
        const int holdSamples = juce::jlimit (1,
                                              downsampleMax,
                                              (int) std::round (juce::jmap (crushAmount, 1.0f, (float) downsampleMax)));

        if (holdSamples <= 1)
        {
            holdCounter = 0;
            heldL = inL;
            heldR = inR;
        }
        else if (holdCounter == 0)
        {
            heldL = inL;
            heldR = inR;
            holdCounter = holdSamples - 1;
        }
        else
        {
            --holdCounter;
        }

        const float dsL = heldL;
        const float dsR = heldR;

        const float bitDepthF = juce::jmap (crushAmount, (float) maxBitDepth, (float) minBitDepth);
        const int bitDepth = (int) std::round (bitDepthF);

        float wetL = quantizeToBitDepth (dsL, bitDepth);
        float wetR = quantizeToBitDepth (dsR, bitDepth);

        // Optional clipper applied to wet chain only.
        if (clipEnabled)
        {
            const float drive = juce::Decibels::decibelsToGain (clipDriveDb * crushAmount);

            wetL = clipSample (wetL * drive, clipStyle);
            wetR = clipSample (wetR * drive, clipStyle);
        }

        // Keep bypass-like behavior under threshold (handled above). Over threshold, allow blending.
        l = inL + (wetL - inL) * mix;
        r = inR + (wetR - inR) * mix;
    }

private:
    static constexpr float epsilon = 1.0e-9f;

    double sampleRate = 44100.0;

    float thresholdDb = -18.0f;
    float attackMs = 10.0f;

    float releaseMs = 120.0f;

    float crushRangeDb = 24.0f;
    int minBitDepth = 4;
    int maxBitDepth = 24;
    float mix = 1.0f;

    int downsampleMax = 1;
    int holdCounter = 0;
    float heldL = 0.0f;
    float heldR = 0.0f;

    bool clipEnabled = false;
    float clipDriveDb = 12.0f;
    float clipStyle = 0.5f; // 0 = soft, 1 = hard

    float env = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    static inline float msToCoeff (float ms, double sr)
    {
        const auto clampedMs = juce::jmax (0.001f, ms);
        const auto tau = clampedMs * 0.001f;
        const auto a = std::exp (-1.0 / (tau * sr));
        return (float) a;
    }

    static inline float quantizeToBitDepth (float x, int bitDepth)
    {
        bitDepth = juce::jlimit (2, 24, bitDepth);

        // 2^(N-1) steps per polarity in [-1, 1).
        const float steps = (float) (1 << (bitDepth - 1));
        const float q = std::round (x * steps) / steps;
        return juce::jlimit (-1.0f, 1.0f, q);
    }

    static inline float clipSample (float xDriven, float style01)
    {
        const float soft = std::tanh (xDriven);
        const float hard = juce::jlimit (-1.0f, 1.0f, xDriven);
        return soft + (hard - soft) * style01;
    }

    void updateTimeConstants()
    {
        attackCoeff = msToCoeff (attackMs, sampleRate);
        releaseCoeff = msToCoeff (releaseMs, sampleRate);
    }
};
