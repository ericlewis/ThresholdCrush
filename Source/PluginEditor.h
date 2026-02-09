#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ThresholdCrushAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    explicit ThresholdCrushAudioProcessorEditor (ThresholdCrushAudioProcessor&);
    ~ThresholdCrushAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Lightweight test seams (avoid brittle child-walking in unit tests).
    const juce::Slider& debugThresholdSlider() const noexcept { return threshold; }
    const juce::Slider& debugClipDriveSlider() const noexcept { return clipDrive; }
    const juce::Slider& debugClipStyleSlider() const noexcept { return clipStyle; }

private:
    void timerCallback() override;

    ThresholdCrushAudioProcessor& audioProcessor;

    std::atomic<float>* clipEnabledParam = nullptr; // read-only on message thread for visuals
    juce::Rectangle<int> clipRowArea;

    juce::Slider threshold;
    juce::Slider attack;
    juce::Slider release;

    juce::Slider crushRange;
    juce::Slider minBitDepth;
    juce::Slider downsampleMax;

    juce::ToggleButton clipEnabled;
    juce::Slider clipDrive;
    juce::Slider clipStyle;

    juce::Slider mix;

    juce::Label thresholdLabel;
    juce::Label attackLabel;
    juce::Label releaseLabel;

    juce::Label crushRangeLabel;
    juce::Label minBitDepthLabel;
    juce::Label downsampleMaxLabel;

    juce::Label clipDriveLabel;
    juce::Label clipStyleLabel;

    juce::Label mixLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<Attachment> thresholdAttachment;
    std::unique_ptr<Attachment> attackAttachment;
    std::unique_ptr<Attachment> releaseAttachment;

    std::unique_ptr<Attachment> crushRangeAttachment;
    std::unique_ptr<Attachment> minBitDepthAttachment;
    std::unique_ptr<Attachment> downsampleMaxAttachment;

    std::unique_ptr<ButtonAttachment> clipEnabledAttachment;
    std::unique_ptr<Attachment> clipDriveAttachment;
    std::unique_ptr<Attachment> clipStyleAttachment;

    std::unique_ptr<Attachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThresholdCrushAudioProcessorEditor)
};
