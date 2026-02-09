#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

namespace
{
struct ThresholdCrushLayoutUnitTest final : public juce::UnitTest
{
    ThresholdCrushLayoutUnitTest() : juce::UnitTest ("ThresholdCrushLayout", "ui") {}

    void runTest() override
    {
        beginTest ("Clip knobs are same size as other knobs; no overlaps; within bounds");

        juce::ScopedJuceInitialiser_GUI gui;

        ThresholdCrushAudioProcessor proc;
        std::unique_ptr<juce::AudioProcessorEditor> editorBase (proc.createEditor());
        auto* editor = dynamic_cast<ThresholdCrushAudioProcessorEditor*> (editorBase.get());
        expect (editor != nullptr);
        if (editor == nullptr)
            return;

        editor->setSize (560, 500);
        editor->resized();

        const auto& thresholdKnob = editor->debugThresholdSlider();
        const auto& clipDriveKnob = editor->debugClipDriveSlider();
        const auto& clipStyleKnob = editor->debugClipStyleSlider();

        expect (thresholdKnob.getBounds().getWidth() > 0);
        expect (clipDriveKnob.getBounds().getWidth() > 0);
        expect (clipStyleKnob.getBounds().getWidth() > 0);

        expectEquals (clipDriveKnob.getHeight(), thresholdKnob.getHeight());
        expectEquals (clipDriveKnob.getWidth(), thresholdKnob.getWidth());
        expectEquals (clipStyleKnob.getHeight(), thresholdKnob.getHeight());
        expectEquals (clipStyleKnob.getWidth(), thresholdKnob.getWidth());

        auto within = editor->getLocalBounds();
        expect (within.contains (thresholdKnob.getBounds()));
        expect (within.contains (clipDriveKnob.getBounds()));
        expect (within.contains (clipStyleKnob.getBounds()));

        // Basic non-overlap sanity for the clip knobs.
        expect (! clipDriveKnob.getBounds().intersects (clipStyleKnob.getBounds()));

        // Ensure editor is destroyed before JUCE GUI teardown (debug leak detector).
        editorBase.reset();
    }
};

static ThresholdCrushLayoutUnitTest thresholdCrushLayoutUnitTestInstance;
}
