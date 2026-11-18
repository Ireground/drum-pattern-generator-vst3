#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>

class DrumProcessor : public juce::AudioProcessor
{
public:
    static constexpr int NL = 8;   // lanes
    static constexpr int NS = 32;  // max steps per lane

    DrumProcessor();
    ~DrumProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // ---- pattern data (shared between editor and audio) ----
    std::array<std::atomic<int>, NL * NS> steps;  // velocity, 0 = off
    std::array<std::atomic<int>, NL> laneNote, laneLen, laneMute, laneSolo, laneChance, laneLock;
    std::array<std::atomic<int>, NL> currentStep;  // for the playhead display
    std::atomic<int> mapSel { 0 };
    std::atomic<int> learnLane { -1 };
    std::atomic<bool> isPlayingNow { false };
    std::atomic<float> uiScale { 1.0f };      // editor window size, remembered per instance

    int  getStep (int lane, int s) const   { return steps[(size_t) (lane * NS + s)].load(); }
    void setStep (int lane, int s, int v)  { steps[(size_t) (lane * NS + s)].store (v); }

    // ---- parameter helpers ----
    int  param (const char* id) const;
    void setParam (const char* id, float value);
    double stepBeats() const;               // length of one step in quarter notes
    static const char* laneName (int i);
    static const char* styleName (int i);
    static const char* rateName (int i);

    // ---- pattern actions (message thread) ----
    void randomizeAll();
    void mutateAll();
    void clearAll();
    void undo();
    void randomizeLane (int lane);
    void chokeHats();
    void saveUndo();
    void applyMap (int m);

    juce::File writeMidiFile();             // pattern as a .mid file in the temp folder

private:
    void defaultPattern();
    float styleProbability (int style, int lane, int pos, float c) const;
    int styleVelocity (int lane, int pos);
    void allNotesOff (juce::MidiBuffer& midi);
    void fireStep (juce::int64 k, int offset, juce::MidiBuffer& midi, int channel, int gateSamples);

    std::array<int, NL * NS> undoBuf {};
    bool hasUndo = false;
    juce::Random rng, audioRng;

    // playback state (audio thread)
    double sampleRate = 44100.0;
    bool wasPlaying = false;
    double expectedPos = 0.0;
    juce::int64 lastFired = -1;
    std::array<int, NL> offRemaining {}, offNote {}, offChannel {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumProcessor)
};
