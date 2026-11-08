cmake_minimum_required(VERSION 3.22)

if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

project(Beneath VERSION 1.0.0 LANGUAGES C CXX)

enable_testing()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BENEATH_JUCE_SOURCE_DIR "" CACHE PATH "Path to a JUCE checkout")
if(NOT BENEATH_JUCE_SOURCE_DIR)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
        set(BENEATH_JUCE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
    elseif(WIN32 AND EXISTS "F:/JUCE/CMakeLists.txt")
        set(BENEATH_JUCE_SOURCE_DIR "F:/JUCE")
    else()
        message(FATAL_ERROR "JUCE was not found. Pass -DBENEATH_JUCE_SOURCE_DIR=/path/to/JUCE")
    endif()
endif()

set(JUCE_BUILD_HELPER_TOOLS ON CACHE BOOL "Build JUCE helper tools with the main toolchain" FORCE)
add_subdirectory("${BENEATH_JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
if(TARGET juceaide AND NOT TARGET juce::juceaide)
    add_executable(juce::juceaide ALIAS juceaide)
endif()

set(BENEATH_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND BENEATH_FORMATS AU)
endif()

juce_add_plugin(Beneath
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "MonkeMountainStudios"
    COMPANY_WEBSITE "https://monkemountainstudios.neocities.org"
    BUNDLE_ID "com.monkemountainstudios.beneath"
    PLUGIN_MANUFACTURER_CODE Mnke
    PLUGIN_CODE BnTh
    FORMATS ${BENEATH_FORMATS}
    PRODUCT_NAME "Beneath"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_AUTO_MANIFEST FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

# Keep the standalone artifact in a predictable release folder.
if(TARGET Beneath_Standalone)
    set_target_properties(Beneath_Standalone PROPERTIES
        OUTPUT_NAME "Beneath"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Beneath_artefacts/Release/Standalone")
endif()

juce_generate_juce_header(Beneath)

juce_add_binary_data(BeneathAssets SOURCES
    Assets/ocean_backdrop.png
    Assets/beneath_logo.png)

target_sources(Beneath PRIVATE
    Source/BeneathEngine.cpp
    Source/BeneathEngine.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h)

target_compile_definitions(Beneath PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

if(WIN32)
    target_compile_definitions(Beneath PUBLIC JUCE_ASIO=1)
endif()

target_link_libraries(Beneath PRIVATE
    BeneathAssets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

juce_add_console_app(BeneathDspTests PRODUCT_NAME "Beneath DSP Tests")
juce_generate_juce_header(BeneathDspTests)
target_sources(BeneathDspTests PRIVATE
    Tests/BeneathEngineTest.cpp
    Source/BeneathEngine.cpp
    Source/BeneathEngine.h)
target_include_directories(BeneathDspTests PRIVATE Source)
target_compile_definitions(BeneathDspTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(BeneathDspTests PRIVATE
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_dsp)
add_test(NAME BeneathDspSmoke COMMAND BeneathDspTests)

