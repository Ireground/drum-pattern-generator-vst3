#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// The whole interface is drawn by hand (like the JSFX version) and handles its own mouse input.
class DrumEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DrumEditor (DrumProcessor&);
    ~DrumEditor() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    enum Kind
    {
        None, BtnRandomize, BtnMutate, BtnClear, BtnUndo, BtnStyle, BtnRate, BtnMap,
        BtnExportBars, BtnSave, BtnPlay, DragMidi,
        ValComplexity, ValSwing, ValVel, ValNoteLen, ValChannel,
        LaneMute, LaneSolo, LaneLock, LaneRnd, LaneNote, LaneLen, LaneChance,
        Grid
    };

    struct Target { Kind kind = None; int lane = -1; int step = -1; };

    void timerCallback() override;
    Target targetAt (juce::Point<int>) const;
    juce::Rectangle<int> rectFor (Kind, int lane = -1) const;
    bool isValueBox (Kind k) const;
    int  getValue (Kind, int lane) const;
    void setValue (Kind, int lane, int v);
    int  minValue (Kind) const;
    int  maxValue (Kind) const;
    float dragScale (Kind) const;
    void click (const Target&, const juce::MouseEvent&);
    void startMidiDrag();
    void saveMidi();

    void drawButton (juce::Graphics&, juce::Rectangle<int>, const juce::String&, bool on, juce::Colour onCol, bool bold = false);
    void drawValueBox (juce::Graphics&, Kind, int lane, const juce::String&, bool fill);

    DrumProcessor& processor;

    // mouse state
    Target hover, active;
    int dragStartY = 0, dragStartValue = 0;
    bool painting = false; int paintValue = 0;
    bool velDragging = false; int velLane = 0, velStep = 0;
    bool midiDragging = false;
    std::unique_ptr<juce::FileChooser> chooser;

    static constexpr int gx = 302, gy = 104, rh = 38, cw = 20;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumEditor)
};
