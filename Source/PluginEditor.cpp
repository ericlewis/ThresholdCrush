#include "PluginEditor.h"

namespace
{
void setupKnob (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 22);
}

void setupIntKnob (juce::Slider& s)
{
    setupKnob (s);
    s.setNumDecimalPlacesToDisplay (0);
}

void setupLabel (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
}
}

ThresholdCrushAudioProcessorEditor::ThresholdCrushAudioProcessorEditor (ThresholdCrushAudioProcessor& p)
: AudioProcessorEditor (&p)
, audioProcessor (p)
{
    // Detector
    setupKnob (threshold);
    threshold.setSkewFactorFromMidPoint (-18.0);

    setupKnob (attack);
    setupKnob (release);

    // Crush
    setupKnob (crushRange);
    setupIntKnob (minBitDepth);
    setupIntKnob (downsampleMax);

    // Clip
    clipEnabled.setButtonText ("Clip");
    clipEnabled.setClickingTogglesState (true);

    setupKnob (clipDrive);
    setupKnob (clipStyle);

    // Mix
    mix.setSliderStyle (juce::Slider::LinearHorizontal);
    mix.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);

    setupLabel (thresholdLabel, "Threshold");
    setupLabel (attackLabel, "Attack");
    setupLabel (releaseLabel, "Release");

    setupLabel (crushRangeLabel, "Crush Range");
    setupLabel (minBitDepthLabel, "Min Bits");
    setupLabel (downsampleMaxLabel, "Downsample Max");

    setupLabel (clipDriveLabel, "Drive");
    setupLabel (clipStyleLabel, "Soft <-> Hard");

    mixLabel.setText ("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (threshold);
    addAndMakeVisible (attack);
    addAndMakeVisible (release);
    addAndMakeVisible (crushRange);
    addAndMakeVisible (minBitDepth);
    addAndMakeVisible (downsampleMax);
    addAndMakeVisible (clipEnabled);
    addAndMakeVisible (clipDrive);
    addAndMakeVisible (clipStyle);
    addAndMakeVisible (mix);

    addAndMakeVisible (thresholdLabel);
    addAndMakeVisible (attackLabel);
    addAndMakeVisible (releaseLabel);
    addAndMakeVisible (crushRangeLabel);
    addAndMakeVisible (minBitDepthLabel);
    addAndMakeVisible (downsampleMaxLabel);
    addAndMakeVisible (clipDriveLabel);
    addAndMakeVisible (clipStyleLabel);
    addAndMakeVisible (mixLabel);

    thresholdAttachment = std::make_unique<Attachment> (audioProcessor.apvts, "threshold_db", threshold);
    attackAttachment = std::make_unique<Attachment> (audioProcessor.apvts, "attack_ms", attack);
    releaseAttachment = std::make_unique<Attachment> (audioProcessor.apvts, "release_ms", release);

    crushRangeAttachment = std::make_unique<Attachment> (audioProcessor.apvts, "crush_range_db", crushRange);
    minBitDepthAttachment = std::make_unique<Attachment> (audioProcessor.apvts, "min_bit_depth", minBitDepth);
    downsampleMaxAttachment = std::make_unique<Attachment> (audioProcessor.apvts, "downsample_max", downsampleMax);

    clipEnabledAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "clip_enabled", clipEnabled);
    clipDriveAttachment = std::make_unique<Attachment> (audioProcessor.apvts, "clip_drive_db", clipDrive);
    clipStyleAttachment = std::make_unique<Attachment> (audioProcessor.apvts, "clip_style", clipStyle);

    mixAttachment = std::make_unique<Attachment> (audioProcessor.apvts, "mix", mix);

    clipEnabledParam = audioProcessor.apvts.getRawParameterValue ("clip_enabled");

    startTimerHz (15);

    setSize (560, 500);
}

void ThresholdCrushAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background: subtle vignette + top glow.
    {
        juce::ColourGradient grad (juce::Colour::fromRGB (8, 10, 12),
                                   0.0f, 0.0f,
                                   juce::Colour::fromRGB (0, 0, 0),
                                   0.0f, (float) bounds.getBottom(),
                                   false);
        g.setGradientFill (grad);
        g.fillAll();

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRect (bounds.removeFromBottom (80));
    }

    auto header = getLocalBounds().removeFromTop (28);

    // Clip row state (visual only): indicate bypass/active without disabling Drive/Style knobs.
    if (! clipRowArea.isEmpty())
    {
        const bool clipOn = (clipEnabledParam != nullptr && clipEnabledParam->load() > 0.5f);
        auto r = clipRowArea.toFloat().reduced (4.0f);

        const auto accent = juce::Colour::fromRGB (255, 120, 90);
        const float pulse = 0.5f + 0.5f * std::sin ((float) juce::Time::getMillisecondCounterHiRes() * 0.0045f);

        g.setColour (accent.withAlpha (clipOn ? (0.08f + 0.03f * pulse) : 0.025f));
        g.fillRoundedRectangle (r, 14.0f);

        g.setColour (juce::Colours::white.withAlpha (clipOn ? 0.18f : 0.10f));
        g.drawRoundedRectangle (r, 14.0f, 1.0f);

        // Small badge near the toggle.
        auto b = clipEnabled.getBounds().toFloat();
        auto badge = juce::Rectangle<float> (b.getX(), b.getY() - 18.0f, 74.0f, 14.0f);

        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (badge, 7.0f);

        g.setColour ((clipOn ? accent : juce::Colours::white).withAlpha (clipOn ? 0.85f : 0.35f));
        g.setFont (juce::Font (juce::FontOptions (10.0f)));
        g.drawFittedText (clipOn ? "CLIP ON" : "BYPASS",
                          badge.toNearestInt(),
                          juce::Justification::centred,
                          1);
    }

    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    g.drawFittedText ("ThresholdCrush", header, juce::Justification::centred, 1);

    // Activity meters (right side of header): IN + CRUSH
    {
        const float in01 = audioProcessor.getInputMeter01();
        const float crush01 = audioProcessor.getCrushMeter01();

        auto meter = header.reduced (10, 6).removeFromRight (150);
        meter = meter.withHeight (16).withY (header.getY() + 6);

        const float pulse = 0.5f + 0.5f * std::sin ((float) juce::Time::getMillisecondCounterHiRes() * 0.006f);

        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillRoundedRectangle (meter.toFloat(), 6.0f);

        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawRoundedRectangle (meter.toFloat(), 6.0f, 1.0f);

        auto inLane = meter.removeFromTop (8).reduced (6, 1);
        auto crushLane = meter.reduced (6, 1);

        auto fillLane = [&](juce::Rectangle<int> lane, float v, juce::Colour c)
        {
            auto f = lane.toFloat();
            const float w = f.getWidth() * juce::jlimit (0.0f, 1.0f, v);
            auto filled = f.withWidth (w);

            // Glow pass
            g.setColour (c.withAlpha (0.25f + 0.15f * pulse));
            g.fillRoundedRectangle (filled.expanded (0.5f, 0.6f), 3.0f);

            // Core pass
            g.setColour (c.withAlpha (0.75f));
            g.fillRoundedRectangle (filled, 3.0f);
        };

        fillLane (inLane, in01, juce::Colour::fromRGB (70, 200, 255));
        fillLane (crushLane, crush01, juce::Colour::fromRGB (255, 120, 90));
    }

    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawFittedText ("Clean under threshold. Overshoot adds downsample + bitcrush (+ optional clip).",
                      getLocalBounds().removeFromBottom (20),
                      juce::Justification::centred,
                      1);
}

void ThresholdCrushAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    area.removeFromTop (28);
    area.removeFromBottom (20);

    const int mixRowHeight = 50;
    auto mixRow = area.removeFromBottom (mixRowHeight);

    const int rowHeight = 120;
    auto row1 = area.removeFromTop (rowHeight);
    auto row2 = area.removeFromTop (rowHeight);
    auto row3 = area.removeFromTop (rowHeight);
    clipRowArea = row3;

    auto layout3 = [](juce::Rectangle<int> r,
                      juce::Label& l1, juce::Slider& s1,
                      juce::Label& l2, juce::Slider& s2,
                      juce::Label& l3, juce::Slider& s3)
    {
        auto c1 = r.removeFromLeft (r.getWidth() / 3);
        auto c2 = r.removeFromLeft (r.getWidth() / 2);
        auto c3 = r;

        l1.setBounds (c1.removeFromTop (18));
        s1.setBounds (c1.reduced (6));

        l2.setBounds (c2.removeFromTop (18));
        s2.setBounds (c2.reduced (6));

        l3.setBounds (c3.removeFromTop (18));
        s3.setBounds (c3.reduced (6));
    };

    layout3 (row1, thresholdLabel, threshold, attackLabel, attack, releaseLabel, release);
    layout3 (row2, crushRangeLabel, crushRange, minBitDepthLabel, minBitDepth, downsampleMaxLabel, downsampleMax);

    // Clip row: [toggle] [drive knob] [style knob]
    {
        auto c1 = row3.removeFromLeft (row3.getWidth() / 3);
        auto c2 = row3.removeFromLeft (row3.getWidth() / 2);
        auto c3 = row3;

        // Center the toggle in its lane.
        auto toggleArea = c1.reduced (10);
        clipEnabled.setBounds (toggleArea.withHeight (30).withY (toggleArea.getCentreY() - 15));

        clipDriveLabel.setBounds (c2.removeFromTop (18));
        clipDrive.setBounds (c2.reduced (6));

        clipStyleLabel.setBounds (c3.removeFromTop (18));
        clipStyle.setBounds (c3.reduced (6));
    }

    mixLabel.setBounds (mixRow.removeFromLeft (60));
    mix.setBounds (mixRow.reduced (0, 6));
}

void ThresholdCrushAudioProcessorEditor::timerCallback()
{
    repaint();
}
