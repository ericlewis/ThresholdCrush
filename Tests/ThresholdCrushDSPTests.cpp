#include <JuceHeader.h>
#include "../Source/ThresholdCrushDSP.h"

namespace
{
struct ThresholdCrushDSPUnitTest final : public juce::UnitTest
{
    ThresholdCrushDSPUnitTest() : juce::UnitTest ("ThresholdCrushDSP", "dsp") {}

    void runTest() override
    {
        testCleanUnderThreshold();
        testCrushIncreasesWithOvershoot();
        testStereoLinkingCrushesQuietChannel();
        testAttackSmoothingDelaysCrush();
        testMinBitDepthMatters();
        testMixWorks();
        testReleaseAffectsRecovery();
        testDownsampleHoldsValues();
        testClipToggleChangesOutput();
        testSoftVsHardStyleDiffers();
    }

    void testCleanUnderThreshold()
    {
        beginTest ("Clean under threshold is bit-identical");

        ThresholdCrushDSP dsp;
        dsp.setSampleRate (48000.0);
        dsp.setThresholdDb (-12.0f);
        dsp.setAttackMs (0.5f);
        dsp.setCrushRangeDb (24.0f);
        dsp.setBitDepthRange (4, 24);

        // -20 dBFS sine sample should be below threshold.
        const float x = juce::Decibels::decibelsToGain (-20.0f) * 0.7071f;
        float l = x;
        float r = -x;
        dsp.processSample (l, r);

        expectEquals (l, x);
        expectEquals (r, -x);
    }

    void testCrushIncreasesWithOvershoot()
    {
        beginTest ("More overshoot -> coarser quantization");

        ThresholdCrushDSP dsp;
        dsp.setSampleRate (48000.0);
        dsp.setThresholdDb (-30.0f);
        dsp.setAttackMs (0.5f);
        dsp.setCrushRangeDb (24.0f);
        dsp.setBitDepthRange (4, 24);

        // Use a signal that is likely between quantization steps.
        const float in = 0.1234567f;

        // Drive the envelope with two steady levels, compare output delta.
        // First: mild overshoot (about -20 dBFS).
        dsp.reset();
        {
            const float drive = juce::Decibels::decibelsToGain (-20.0f);
            float l = drive;
            float r = drive;
            for (int i = 0; i < 2000; ++i) dsp.processSample (l, r);

            float a = in;
            float b = -in;
            dsp.processSample (a, b);
            const float errMild = std::abs (a - in);

            // Second: larger overshoot (about -6 dBFS).
            dsp.reset();
            l = juce::Decibels::decibelsToGain (-6.0f);
            r = l;
            for (int i = 0; i < 2000; ++i) dsp.processSample (l, r);

            float c = in;
            float d = -in;
            dsp.processSample (c, d);
            const float errHot = std::abs (c - in);

            expect (errHot >= errMild);
            expect (errHot > 0.0f);
        }
    }

    void testStereoLinkingCrushesQuietChannel()
    {
        beginTest ("Stereo linking: loud L crushes quiet R");

        ThresholdCrushDSP dsp;
        dsp.setSampleRate (48000.0);
        dsp.setThresholdDb (-24.0f);
        dsp.setAttackMs (0.5f);
        dsp.setCrushRangeDb (24.0f);
        dsp.setBitDepthRange (4, 24);

        // Drive envelope with loud left only.
        dsp.reset();
        float l = juce::Decibels::decibelsToGain (-3.0f);
        float r = 0.01f;
        for (int i = 0; i < 2000; ++i) dsp.processSample (l, r);

        // Now process a quiet value: it should get quantized (changed) due to linked detector.
        float ql = 0.0012345f;
        float qr = 0.0012345f;
        dsp.processSample (ql, qr);

        expect (std::abs (qr - 0.0012345f) > 0.0f);
    }

    void testAttackSmoothingDelaysCrush()
    {
        beginTest ("Attack smoothing delays onset of crush");

        ThresholdCrushDSP slow;
        ThresholdCrushDSP fast;

        slow.setSampleRate (48000.0);
        fast.setSampleRate (48000.0);

        slow.setThresholdDb (-24.0f);
        fast.setThresholdDb (-24.0f);

        slow.setAttackMs (100.0f);
        fast.setAttackMs (0.5f);

        slow.setCrushRangeDb (24.0f);
        fast.setCrushRangeDb (24.0f);

        // Step up to a loud level just above threshold.
        const float step = juce::Decibels::decibelsToGain (-6.0f);

        auto measureFirstSampleChange = [&](ThresholdCrushDSP& dsp)
        {
            dsp.reset();
            float l = 0.1f;
            float r = 0.1f;
            dsp.processSample (l, r); // settle below threshold

            float x = step * 0.1234567f;
            float y = x;
            dsp.processSample (x, y);
            return std::abs (x - (step * 0.1234567f));
        };

        const float fastErr = measureFirstSampleChange (fast);
        const float slowErr = measureFirstSampleChange (slow);

        // Fast attack should crush more immediately.
        expect (fastErr >= slowErr);
    }

    void testMinBitDepthMatters()
    {
        beginTest ("Min bit depth changes quantization severity");

        ThresholdCrushDSP lowBits;
        ThresholdCrushDSP highBits;

        lowBits.setSampleRate (48000.0);
        highBits.setSampleRate (48000.0);

        lowBits.setThresholdDb (-60.0f);
        highBits.setThresholdDb (-60.0f);

        lowBits.setAttackMs (0.5f);
        highBits.setAttackMs (0.5f);

        lowBits.setCrushRangeDb (6.0f);  // force near-max crush easily
        highBits.setCrushRangeDb (6.0f);

        lowBits.setBitDepthRange (2, 24);
        highBits.setBitDepthRange (12, 24);

        lowBits.setMix (1.0f);
        highBits.setMix (1.0f);

        // Drive envelope very hot so we're at max crush.
        float driveL = 0.99f;
        float driveR = 0.99f;
        for (int i = 0; i < 4000; ++i)
        {
            lowBits.processSample (driveL, driveR);
            highBits.processSample (driveL, driveR);
        }

        const float in = 0.1234567f;
        float aL = in, aR = in;
        float bL = in, bR = in;
        lowBits.processSample (aL, aR);
        highBits.processSample (bL, bR);

        const float errLow = std::abs (aL - in);
        const float errHigh = std::abs (bL - in);
        expect (errLow >= errHigh);
        expect (errLow > 0.0f);
    }

    void testMixWorks()
    {
        beginTest ("Mix blends dry and crushed");

        ThresholdCrushDSP dsp;
        dsp.setSampleRate (48000.0);
        dsp.setThresholdDb (-60.0f);
        dsp.setAttackMs (0.5f);
        dsp.setCrushRangeDb (6.0f);
        dsp.setBitDepthRange (2, 24);

        // Drive envelope hot.
        float l = 0.99f, r = 0.99f;
        for (int i = 0; i < 2000; ++i) dsp.processSample (l, r);

        const float in = 0.1234567f;

        dsp.setMix (1.0f);
        float wetL = in, wetR = in;
        dsp.processSample (wetL, wetR);

        dsp.setMix (0.0f);
        float dryL = in, dryR = in;
        dsp.processSample (dryL, dryR);

        expectEquals (dryL, in);
        expectEquals (dryR, in);
        expect (std::abs (wetL - in) > 0.0f);
    }

    void testReleaseAffectsRecovery()
    {
        beginTest ("Release affects recovery time");

        ThresholdCrushDSP fast;
        ThresholdCrushDSP slow;

        fast.setSampleRate (48000.0);
        slow.setSampleRate (48000.0);

        fast.setThresholdDb (-24.0f);
        slow.setThresholdDb (-24.0f);

        fast.setAttackMs (0.5f);
        slow.setAttackMs (0.5f);

        fast.setReleaseMs (20.0f);
        slow.setReleaseMs (500.0f);

        fast.setCrushRangeDb (24.0f);
        slow.setCrushRangeDb (24.0f);

        fast.setBitDepthRange (4, 24);
        slow.setBitDepthRange (4, 24);

        fast.setDownsampleMax (1);
        slow.setDownsampleMax (1);

        fast.setClipEnabled (false);
        slow.setClipEnabled (false);

        fast.setMix (1.0f);
        slow.setMix (1.0f);

        // Drive above threshold to charge the envelope.
        float driveL = juce::Decibels::decibelsToGain (-6.0f);
        float driveR = driveL;
        for (int i = 0; i < 2000; ++i)
        {
            fast.processSample (driveL, driveR);
            slow.processSample (driveL, driveR);
        }

        // Now feed a quiet sample that should be clean once envelope has released.
        const float quiet = 0.0012345f;
        int fastCleanAt = -1;
        int slowCleanAt = -1;

        for (int i = 0; i < 6000; ++i)
        {
            float l1 = quiet, r1 = quiet;
            float l2 = quiet, r2 = quiet;

            fast.processSample (l1, r1);
            slow.processSample (l2, r2);

            const bool fastClean = (std::abs (l1 - quiet) < 1.0e-9f && std::abs (r1 - quiet) < 1.0e-9f);
            const bool slowClean = (std::abs (l2 - quiet) < 1.0e-9f && std::abs (r2 - quiet) < 1.0e-9f);

            if (fastCleanAt < 0 && fastClean) fastCleanAt = i;
            if (slowCleanAt < 0 && slowClean) slowCleanAt = i;
        }

        expect (fastCleanAt >= 0);
        expect (slowCleanAt < 0 || slowCleanAt > fastCleanAt);
    }

    void testDownsampleHoldsValues()
    {
        beginTest ("Downsample holds values in blocks");

        ThresholdCrushDSP dsp;
        dsp.setSampleRate (48000.0);
        dsp.setThresholdDb (-60.0f);
        dsp.setAttackMs (0.5f);
        dsp.setReleaseMs (120.0f);
        dsp.setCrushRangeDb (6.0f);

        // Disable bitcrush influence as much as possible (but still deterministic).
        dsp.setBitDepthRange (24, 24);
        dsp.setMix (1.0f);
        dsp.setClipEnabled (false);

        dsp.setDownsampleMax (4);

        // Charge envelope so we're at max crush amount (=> holdSamples ~= downsampleMax).
        float driveL = 0.99f, driveR = 0.99f;
        for (int i = 0; i < 2000; ++i) dsp.processSample (driveL, driveR);

        // Feed a changing signal; output should repeat in 4-sample blocks.
        float out[8] = {};
        for (int i = 0; i < 8; ++i)
        {
            float l = 0.10f + 0.01f * (float) i;
            float r = l;
            dsp.processSample (l, r);
            out[i] = l;
        }

        expect (std::abs (out[0] - out[1]) < 1.0e-6f);
        expect (std::abs (out[1] - out[2]) < 1.0e-6f);
        expect (std::abs (out[2] - out[3]) < 1.0e-6f);
        expect (std::abs (out[4] - out[5]) < 1.0e-6f);
        expect (std::abs (out[5] - out[6]) < 1.0e-6f);
        expect (std::abs (out[6] - out[7]) < 1.0e-6f);
        expect (std::abs (out[0] - out[4]) > 1.0e-6f);
    }

    void testClipToggleChangesOutput()
    {
        beginTest ("Clip toggle changes output when enabled");

        ThresholdCrushDSP a; // no clip
        ThresholdCrushDSP b; // clip

        a.setSampleRate (48000.0);
        b.setSampleRate (48000.0);

        a.setThresholdDb (-60.0f);
        b.setThresholdDb (-60.0f);

        a.setAttackMs (0.5f);
        b.setAttackMs (0.5f);

        a.setReleaseMs (120.0f);
        b.setReleaseMs (120.0f);

        a.setCrushRangeDb (6.0f);
        b.setCrushRangeDb (6.0f);

        a.setBitDepthRange (24, 24);
        b.setBitDepthRange (24, 24);

        a.setDownsampleMax (1);
        b.setDownsampleMax (1);

        a.setMix (1.0f);
        b.setMix (1.0f);

        a.setClipEnabled (false);
        b.setClipEnabled (true);
        b.setClipDriveDb (12.0f);
        b.setClipStyle01 (0.5f);

        // Charge envelope to ensure overshoot > 0.
        float driveL = 0.99f, driveR = 0.99f;
        for (int i = 0; i < 2000; ++i)
        {
            a.processSample (driveL, driveR);
            b.processSample (driveL, driveR);
        }

        float l1 = 0.5f, r1 = 0.5f;
        float l2 = 0.5f, r2 = 0.5f;
        a.processSample (l1, r1);
        b.processSample (l2, r2);

        expectEquals (l1, 0.5f);
        expect (std::abs (l2 - 0.5f) > 1.0e-6f);
    }

    void testSoftVsHardStyleDiffers()
    {
        beginTest ("Soft vs hard clip style differs");

        ThresholdCrushDSP soft;
        ThresholdCrushDSP hard;

        soft.setSampleRate (48000.0);
        hard.setSampleRate (48000.0);

        soft.setThresholdDb (-60.0f);
        hard.setThresholdDb (-60.0f);

        soft.setAttackMs (0.5f);
        hard.setAttackMs (0.5f);

        soft.setReleaseMs (120.0f);
        hard.setReleaseMs (120.0f);

        soft.setCrushRangeDb (6.0f);
        hard.setCrushRangeDb (6.0f);

        soft.setBitDepthRange (24, 24);
        hard.setBitDepthRange (24, 24);

        soft.setDownsampleMax (1);
        hard.setDownsampleMax (1);

        soft.setMix (1.0f);
        hard.setMix (1.0f);

        soft.setClipEnabled (true);
        hard.setClipEnabled (true);

        soft.setClipDriveDb (12.0f);
        hard.setClipDriveDb (12.0f);

        soft.setClipStyle01 (0.0f);
        hard.setClipStyle01 (1.0f);

        // Charge envelope to ensure overshoot > 0.
        float driveL = 0.99f, driveR = 0.99f;
        for (int i = 0; i < 2000; ++i)
        {
            soft.processSample (driveL, driveR);
            hard.processSample (driveL, driveR);
        }

        float sL = 0.2f, sR = 0.2f;
        float hL = 0.2f, hR = 0.2f;

        soft.processSample (sL, sR);
        hard.processSample (hL, hR);

        expect (hL > sL);
    }
};

// UnitTests are discovered via static construction (UnitTest::getAllTests()). 
static ThresholdCrushDSPUnitTest thresholdCrushDSPUnitTestInstance;
}

int main (int, char**)
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.setPassesAreLogged (false);
    runner.runAllTests();

    int totalFailures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        totalFailures += runner.getResult (i)->failures;

    return totalFailures == 0 ? 0 : 1;
}
