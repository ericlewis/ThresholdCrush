#include "PluginProcessor.h"
#include "PluginEditor.h"

ThresholdCrushAudioProcessor::ThresholdCrushAudioProcessor()
: AudioProcessor (BusesProperties()
                 .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
, apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

const juce::String ThresholdCrushAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ThresholdCrushAudioProcessor::acceptsMidi() const { return false; }
bool ThresholdCrushAudioProcessor::producesMidi() const { return false; }
bool ThresholdCrushAudioProcessor::isMidiEffect() const { return false; }
double ThresholdCrushAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int ThresholdCrushAudioProcessor::getNumPrograms() { return (int) getFactoryPresets().size(); }
int ThresholdCrushAudioProcessor::getCurrentProgram() { return currentProgram; }
void ThresholdCrushAudioProcessor::setCurrentProgram (int index)
{
    const auto& presets = getFactoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return;

    currentProgram = index;
    applyPreset (index);
}

const juce::String ThresholdCrushAudioProcessor::getProgramName (int index)
{
    const auto& presets = getFactoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return {};
    return presets[(size_t) index].name;
}

void ThresholdCrushAudioProcessor::changeProgramName (int, const juce::String&) {}

void ThresholdCrushAudioProcessor::prepareToPlay (double sampleRate, int)
{
    dsp.setSampleRate (sampleRate);
}

void ThresholdCrushAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ThresholdCrushAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainOut == mainIn;
}
#endif

void ThresholdCrushAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);

    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const float thresholdDb = apvts.getRawParameterValue ("threshold_db")->load();
    const float attackMs = apvts.getRawParameterValue ("attack_ms")->load();
    const float releaseMs = apvts.getRawParameterValue ("release_ms")->load();
    const float crushRangeDb = apvts.getRawParameterValue ("crush_range_db")->load();
    const int minBitDepth = (int) std::round (apvts.getRawParameterValue ("min_bit_depth")->load());
    const int downsampleMax = (int) std::round (apvts.getRawParameterValue ("downsample_max")->load());
    const float mixPct = apvts.getRawParameterValue ("mix")->load();
    const float mix01 = juce::jlimit (0.0f, 1.0f, mixPct / 100.0f);
    const bool clipEnabled = apvts.getRawParameterValue ("clip_enabled")->load() > 0.5f;
    const float clipDriveDb = apvts.getRawParameterValue ("clip_drive_db")->load();
    const float clipStyle01 = juce::jlimit (0.0f, 1.0f, apvts.getRawParameterValue ("clip_style")->load() / 100.0f);

    dsp.setThresholdDb (thresholdDb);
    dsp.setAttackMs (attackMs);
    dsp.setReleaseMs (releaseMs);
    dsp.setCrushRangeDb (crushRangeDb);
    dsp.setBitDepthRange (minBitDepth, 24);
    dsp.setDownsampleMax (downsampleMax);
    dsp.setMix (mix01);
    dsp.setClipEnabled (clipEnabled);
    dsp.setClipDriveDb (clipDriveDb);
    dsp.setClipStyle01 (clipStyle01);

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();

    if (numCh <= 0)
        return;

    float maxIn = 0.0f;

    auto* ch0 = buffer.getWritePointer (0);

    if (numCh == 1)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            const float in = ch0[n];
            maxIn = juce::jmax (maxIn, std::abs (in));

            float l = in;
            float r = l;
            dsp.processSample (l, r);
            ch0[n] = l;
        }

        const float inDb = juce::Decibels::gainToDecibels (maxIn + 1.0e-9f, -60.0f);
        inputMeter01.store (juce::jlimit (0.0f, 1.0f, (inDb + 60.0f) / 60.0f), std::memory_order_relaxed);

        const float envDb = dsp.getEnvelopeDb();
        const float overshootDb = juce::jmax (0.0f, envDb - thresholdDb);
        crushMeter01.store (juce::jlimit (0.0f, 1.0f, overshootDb / juce::jmax (1.0f, crushRangeDb)), std::memory_order_relaxed);

        return;
    }

    auto* ch1 = buffer.getWritePointer (1);

    for (int n = 0; n < numSamples; ++n)
    {
        const float inL = ch0[n];
        const float inR = ch1[n];
        maxIn = juce::jmax (maxIn, juce::jmax (std::abs (inL), std::abs (inR)));

        float l = inL;
        float r = inR;
        dsp.processSample (l, r);
        ch0[n] = l;
        ch1[n] = r;
    }

    const float inDb = juce::Decibels::gainToDecibels (maxIn + 1.0e-9f, -60.0f);
    inputMeter01.store (juce::jlimit (0.0f, 1.0f, (inDb + 60.0f) / 60.0f), std::memory_order_relaxed);

    const float envDb = dsp.getEnvelopeDb();
    const float overshootDb = juce::jmax (0.0f, envDb - thresholdDb);
    crushMeter01.store (juce::jlimit (0.0f, 1.0f, overshootDb / juce::jmax (1.0f, crushRangeDb)), std::memory_order_relaxed);
}

bool ThresholdCrushAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ThresholdCrushAudioProcessor::createEditor()
{
    return new ThresholdCrushAudioProcessorEditor (*this);
}

void ThresholdCrushAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ThresholdCrushAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml == nullptr)
        return;

    if (! xml->hasTagName (apvts.state.getType()))
        return;

    apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout ThresholdCrushAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "threshold_db", 1 },
        "Threshold",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -18.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attack_ms", 1 },
        "Attack",
        juce::NormalisableRange<float> (0.5f, 200.0f, 0.1f, 0.35f),
        10.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "release_ms", 1 },
        "Release",
        juce::NormalisableRange<float> (5.0f, 1000.0f, 0.1f, 0.35f),
        120.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "crush_range_db", 1 },
        "Crush Range",
        juce::NormalisableRange<float> (6.0f, 48.0f, 0.1f, 0.5f),
        24.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")
    ));

    // Stored as float for simplicity in APVTS; treated as integer in DSP.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "min_bit_depth", 1 },
        "Min Bit Depth",
        juce::NormalisableRange<float> (2.0f, 16.0f, 1.0f),
        4.0f,
        juce::AudioParameterFloatAttributes().withLabel ("bits")
    ));

    // Stored as float for simplicity in APVTS; treated as integer in DSP.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "downsample_max", 1 },
        "Downsample Max",
        juce::NormalisableRange<float> (1.0f, 64.0f, 1.0f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel ("x")
    ));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "clip_enabled", 1 },
        "Clip",
        false
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "clip_drive_db", 1 },
        "Clip Drive",
        juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f),
        12.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "clip_style", 1 },
        "Clip Style",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
    ));

    return { params.begin(), params.end() };
}

const std::vector<ThresholdCrushAudioProcessor::Preset>& ThresholdCrushAudioProcessor::getFactoryPresets()
{
    static const std::vector<Preset> presets =
    {
        {
            "Clean Glue",
            {
                { "threshold_db", -18.0f },
                { "attack_ms", 15.0f },
                { "release_ms", 150.0f },
                { "crush_range_db", 30.0f },
                { "min_bit_depth", 10.0f },
                { "downsample_max", 1.0f },
                { "clip_enabled", 0.0f },
                { "clip_drive_db", 6.0f },
                { "clip_style", 40.0f },
                { "mix", 60.0f }
            }
        },
        {
            "Warm Crush",
            {
                { "threshold_db", -24.0f },
                { "attack_ms", 8.0f },
                { "release_ms", 200.0f },
                { "crush_range_db", 24.0f },
                { "min_bit_depth", 6.0f },
                { "downsample_max", 2.0f },
                { "clip_enabled", 1.0f },
                { "clip_drive_db", 8.0f },
                { "clip_style", 30.0f },
                { "mix", 75.0f }
            }
        },
        {
            "Lo-Fi Steps",
            {
                { "threshold_db", -30.0f },
                { "attack_ms", 3.0f },
                { "release_ms", 250.0f },
                { "crush_range_db", 18.0f },
                { "min_bit_depth", 4.0f },
                { "downsample_max", 8.0f },
                { "clip_enabled", 0.0f },
                { "clip_drive_db", 12.0f },
                { "clip_style", 50.0f },
                { "mix", 100.0f }
            }
        },
        {
            "Bit Smash",
            {
                { "threshold_db", -36.0f },
                { "attack_ms", 1.0f },
                { "release_ms", 120.0f },
                { "crush_range_db", 12.0f },
                { "min_bit_depth", 3.0f },
                { "downsample_max", 6.0f },
                { "clip_enabled", 1.0f },
                { "clip_drive_db", 18.0f },
                { "clip_style", 70.0f },
                { "mix", 100.0f }
            }
        },
        {
            "Clipper Bite",
            {
                { "threshold_db", -20.0f },
                { "attack_ms", 4.0f },
                { "release_ms", 90.0f },
                { "crush_range_db", 20.0f },
                { "min_bit_depth", 8.0f },
                { "downsample_max", 1.0f },
                { "clip_enabled", 1.0f },
                { "clip_drive_db", 20.0f },
                { "clip_style", 85.0f },
                { "mix", 65.0f }
            }
        }
    };

    return presets;
}

void ThresholdCrushAudioProcessor::applyPreset (int index)
{
    const auto& presets = getFactoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return;

    const auto& preset = presets[(size_t) index];
    for (const auto& entry : preset.values)
    {
        if (auto* param = apvts.getParameter (entry.first))
        {
            const float normalized = param->convertTo0to1 (entry.second);
            param->setValueNotifyingHost (normalized);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ThresholdCrushAudioProcessor();
}
