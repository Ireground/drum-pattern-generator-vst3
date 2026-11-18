#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Hand-drawn "studio hardware" interface: dark panels, silver knobs with blue value rings,
// coloured tabs and a tall bar-graph step grid. Everything is laid out on a fixed 1240 x 620
// canvas and scaled as a whole, so the window can be resized freely (size is remembered).
class DrumEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DrumEditor (DrumProcessor&);
    ~DrumEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    static constexpr int baseW = 1240, baseH = 620;

private:
    enum Kind
    {
        None,
        // title bar
        StylePrev, StyleName, StyleNext,
        // tabs
        TabRandomize, TabMutate, TabClear, TabUndo,
        // knobs
        KnobComplexity, KnobSwing, KnobVel, KnobNoteLen,
        // menus / toggles
        BoxRate, BoxMap, BoxChannel, BoxExportBars, TogglePlay,
        // export
        DragMidi, BtnSave,
        // per lane
        LaneMute, LaneSolo, LaneLock, LaneRnd, LaneNote, LaneLen, LaneChance,
        Grid
    };

    struct Target { Kind kind = None; int lane = -1; int step = -1; };

    void timerCallback() override;

    // layout (all in base canvas coordinates)
    juce::Rectangle<int> rectFor (Kind, int lane = -1) const;
    juce::Point<float>  knobCentre (Kind) const;
    Target targetAt (juce::Point<int>) const;
    juce::Point<int> toBase (juce::Point<float>) const;
    juce::Point<int> mouseBase (const juce::MouseEvent& e) const { return toBase (e.position); }
    float scale() const { return (float) getWidth() / (float) baseW; }

    // values
    bool  isKnob (Kind) const;
    bool  isValueBox (Kind) const;
    int   getValue (Kind, int lane) const;
    void  setValue (Kind, int lane, int v);
    int   minValue (Kind) const;
    int   maxValue (Kind) const;
    int   defaultValue (Kind) const;
    float dragScale (Kind) const;

    void click (const Target&, const juce::MouseEvent&);
    void showMenu (Kind);
    void startMidiDrag();
    void saveMidi();

    // drawing
    void drawTitleBar (juce::Graphics&);
    void drawPanel (juce::Graphics&, juce::Rectangle<int>);
    void drawSectionHeader (juce::Graphics&, juce::Rectangle<int>, const juce::String&, juce::Colour);
    void drawKnob (juce::Graphics&, Kind, const juce::String& label, const juce::String& valueText, float radius);
    void drawTab (juce::Graphics&, Kind, const juce::String&, juce::Colour, bool highlighted);
    void drawMenuBox (juce::Graphics&, Kind, const juce::String& label, const juce::String& value);
    void drawSmallButton (juce::Graphics&, juce::Rectangle<int>, const juce::String&, bool on, juce::Colour onCol, bool hot);
    void drawLaneValue (juce::Graphics&, Kind, int lane, const juce::String&, float fill);
    void drawGrid (juce::Graphics&);
    void drawExport (juce::Graphics&);
    bool isHot (Kind, int lane = -1) const;

    DrumProcessor& processor;
    juce::LookAndFeel_V4 menuLaf;

    // mouse state
    Target hover, active;
    int dragStartY = 0, dragStartValue = 0;
    bool painting = false; int paintValue = 0;
    bool velDragging = false; int velLane = 0, velStep = 0;
    bool midiDragging = false;
    std::unique_ptr<juce::FileChooser> chooser;

    // grid geometry
    static constexpr int laneX = 216, gx = 486, gy = 112, rh = 54, cw = 17;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumEditor)
};
