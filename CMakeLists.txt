cmake_minimum_required(VERSION 3.22)

project(ThresholdCrush VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(THRESHOLDCRUSH_BUILD_TESTS "Build unit tests" ON)

include(FetchContent)

# Pin JUCE for reproducible builds.
FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.12
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(ThresholdCrush
  COMPANY_NAME "Eric Lewis"
  BUNDLE_ID "com.ericlewis.thresholdcrush"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE

  PLUGIN_MANUFACTURER_CODE JJay
  PLUGIN_CODE ThCr

  FORMATS VST3 AU
  PRODUCT_NAME "ThresholdCrush"
)

juce_generate_juce_header(ThresholdCrush)

target_sources(ThresholdCrush
  PRIVATE
    Source/ThresholdCrushDSP.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_definitions(ThresholdCrush
  PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ThresholdCrush
  PRIVATE
    juce::juce_audio_utils
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if (THRESHOLDCRUSH_BUILD_TESTS)
  juce_add_console_app(ThresholdCrushTests
    PRODUCT_NAME "ThresholdCrushTests")

  juce_generate_juce_header(ThresholdCrushTests)

  target_sources(ThresholdCrushTests
    PRIVATE
      Source/ThresholdCrushDSP.h
      Source/PluginProcessor.cpp
      Source/PluginProcessor.h
      Source/PluginEditor.cpp
      Source/PluginEditor.h
      Tests/ThresholdCrushDSPTests.cpp
      Tests/ThresholdCrushLayoutTests.cpp
  )

  target_compile_definitions(ThresholdCrushTests
    PRIVATE
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JucePlugin_Name="ThresholdCrush"
  )

  target_link_libraries(ThresholdCrushTests
    PRIVATE
      juce::juce_core
      juce::juce_audio_basics
      juce::juce_gui_basics
      juce::juce_audio_processors
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )
endif()
