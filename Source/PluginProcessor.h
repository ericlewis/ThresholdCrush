#pragma once

#include <JuceHeader.h>
#include "ThresholdCrushDSP.h"

class ThresholdCrushAudioProcessor final : public juce::AudioProcessor
{
public:
    ThresholdCrushAudioProcessor();
    ~ThresholdCrushAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // UI meters (0..1). Read on message thread.
    float getInputMeter01() const noexcept { return inputMeter01.load (std::memory_order_relaxed); }
    float getCrushMeter01() const noexcept { return crushMeter01.load (std::memory_order_relaxed); }

private:
    ThresholdCrushDSP dsp;

    std::atomic<float> inputMeter01 { 0.0f };
    std::atomic<float> crushMeter01 { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThresholdCrushAudioProcessor)
};
