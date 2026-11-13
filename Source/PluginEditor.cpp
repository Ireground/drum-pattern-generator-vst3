#include "PluginEditor.h"

namespace
{
    juce::Colour rgb (float r, float g, float b) { return juce::Colour::fromFloatRGBA (r, g, b, 1.0f); }

    juce::Colour laneColour (int l)
    {
        switch (l)
        {
            case 0:  return rgb (0.95f, 0.35f, 0.35f);
            case 1:  return rgb (0.95f, 0.6f, 0.25f);
            case 2:  return rgb (0.95f, 0.85f, 0.3f);
            case 3:  return rgb (0.45f, 0.85f, 0.45f);
            case 4:  return rgb (0.3f, 0.8f, 0.8f);
            case 5:  return rgb (0.4f, 0.6f, 0.95f);
            case 6:  return rgb (0.65f, 0.45f, 0.95f);
            default: return rgb (0.9f, 0.45f, 0.8f);
        }
    }

    const juce::Colour bg       = rgb (0.11f, 0.11f, 0.13f);
    const juce::Colour btnOff   = rgb (0.22f, 0.22f, 0.26f);
    const juce::Colour btnHover = rgb (0.3f, 0.3f, 0.36f);
    const juce::Colour textCol  = rgb (0.88f, 0.88f, 0.92f);
    const juce::Colour dimText  = rgb (0.55f, 0.55f, 0.6f);
    const juce::Colour accent   = rgb (1.0f, 0.55f, 0.2f);
}

DrumEditor::DrumEditor (DrumProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    setSize (950, 458);
    startTimerHz (30);
}

DrumEditor::~DrumEditor() { stopTimer(); }

void DrumEditor::timerCallback() { repaint(); }

// ---------------------------------------------------------------------------
juce::Rectangle<int> DrumEditor::rectFor (Kind k, int lane) const
{
    const int y = gy + lane * rh;
    switch (k)
    {
        case BtnRandomize:  return { 10, 10, 120, 34 };
        case BtnMutate:     return { 138, 10, 80, 34 };
        case BtnClear:      return { 226, 10, 64, 34 };
        case BtnUndo:       return { 298, 10, 64, 34 };
        case BtnStyle:      return { 370, 10, 150, 34 };
        case ValComplexity: return { 528, 10, 130, 34 };
        case ValSwing:      return { 666, 10, 100, 34 };
        case BtnRate:       return { 774, 10, 76, 34 };
        case BtnMap:        return { 856, 10, 86, 34 };
        case ValVel:        return { 10, 52, 150, 22 };
        case ValNoteLen:    return { 168, 52, 140, 22 };
        case ValChannel:    return { 316, 52, 90, 22 };
        case BtnPlay:       return { 414, 52, 160, 22 };
        case BtnExportBars: return { 594, 50, 104, 26 };
        case BtnSave:       return { 704, 50, 100, 26 };
        case DragMidi:      return { 810, 48, 132, 30 };
        case LaneMute:      return { 66, y + 8, 20, 22 };
        case LaneSolo:      return { 88, y + 8, 20, 22 };
        case LaneLock:      return { 110, y + 8, 20, 22 };
        case LaneRnd:       return { 132, y + 8, 30, 22 };
        case LaneNote:      return { 166, y + 8, 42, 22 };
        case LaneLen:       return { 212, y + 8, 34, 22 };
        case LaneChance:    return { 250, y + 8, 42, 22 };
        default:            return {};
    }
}

DrumEditor::Target DrumEditor::targetAt (juce::Point<int> pt) const
{
    for (auto k : { BtnRandomize, BtnMutate, BtnClear, BtnUndo, BtnStyle, ValComplexity, ValSwing, BtnRate, BtnMap,
                    ValVel, ValNoteLen, ValChannel, BtnPlay, BtnExportBars, BtnSave, DragMidi })
        if (rectFor (k).contains (pt))
            return { k, -1, -1 };

    const int lane = (pt.y - gy) / rh;
    if (pt.y >= gy && lane >= 0 && lane < DrumProcessor::NL)
    {
        for (auto k : { LaneMute, LaneSolo, LaneLock, LaneRnd, LaneNote, LaneLen, LaneChance })
            if (rectFor (k, lane).contains (pt))
                return { k, lane, -1 };

        const int step = (pt.x - gx) / cw;
        if (pt.x >= gx && step >= 0 && step < DrumProcessor::NS)
            return { Grid, lane, step };
    }
    return {};
}

bool DrumEditor::isValueBox (Kind k) const
{
    return k == ValComplexity || k == ValSwing || k == ValVel || k == ValNoteLen || k == ValChannel
        || k == LaneNote || k == LaneLen || k == LaneChance;
}

int DrumEditor::getValue (Kind k, int lane) const
{
    switch (k)
    {
        case ValComplexity: return processor.param ("complexity");
        case ValSwing:      return processor.param ("swing");
        case ValVel:        return processor.param ("velhumanize");
        case ValNoteLen:    return processor.param ("notelength");
        case ValChannel:    return processor.param ("channel");
        case LaneNote:      return processor.laneNote[(size_t) lane];
        case LaneLen:       return processor.laneLen[(size_t) lane];
        case LaneChance:    return processor.laneChance[(size_t) lane];
        default:            return 0;
    }
}

void DrumEditor::setValue (Kind k, int lane, int v)
{
    v = juce::jlimit (minValue (k), maxValue (k), v);
    if (v == getValue (k, lane)) return;
    switch (k)
    {
        case ValComplexity: processor.setParam ("complexity", (float) v); break;
        case ValSwing:      processor.setParam ("swing", (float) v); break;
        case ValVel:        processor.setParam ("velhumanize", (float) v); break;
        case ValNoteLen:    processor.setParam ("notelength", (float) v); break;
        case ValChannel:    processor.setParam ("channel", (float) v); break;
        case LaneNote:      processor.laneNote[(size_t) lane] = v; break;
        case LaneLen:       processor.laneLen[(size_t) lane] = v; break;
        case LaneChance:    processor.laneChance[(size_t) lane] = v; break;
        default: break;
    }
}

int DrumEditor::minValue (Kind k) const
{
    return k == ValNoteLen ? 5 : k == LaneLen ? 1 : 0;
}

int DrumEditor::maxValue (Kind k) const
{
    switch (k)
    {
        case ValSwing:   return 75;
        case ValVel:     return 40;
        case ValChannel: return 15;
        case LaneNote:   return 127;
        case LaneLen:    return DrumProcessor::NS;
        default:         return 100;
    }
}

float DrumEditor::dragScale (Kind k) const
{
    switch (k)
    {
        case ValSwing:   return 1.5f;
        case ValVel:     return 2.0f;
        case ValChannel: return 6.0f;
        case LaneNote:   return 3.0f;
        case LaneLen:    return 4.0f;
        default:         return 1.0f;
    }
}

// ---------------------------------------------------------------------------
void DrumEditor::click (const Target& t, const juce::MouseEvent& e)
{
    auto& p = processor;
    const size_t l = (size_t) juce::jmax (0, t.lane);

    switch (t.kind)
    {
        case BtnRandomize: p.randomizeAll(); break;
        case BtnMutate:    p.mutateAll(); break;
        case BtnClear:     p.clearAll(); break;
        case BtnUndo:      p.undo(); break;
        case BtnStyle:
        {
            const int s = p.param ("style");
            const bool left = e.x < rectFor (BtnStyle).getCentreX();
            p.setParam ("style", (float) (left ? (s + 6) % 7 : (s + 1) % 7));
            break;
        }
        case BtnRate:       p.setParam ("rate", (float) ((p.param ("rate") + 1) % 5)); break;
        case BtnMap:        p.applyMap ((p.mapSel + 1) % 3); break;
        case BtnExportBars: p.setParam ("exportbars", (float) ((p.param ("exportbars") + 1) % 4)); break;
        case BtnPlay:       p.setParam ("play", p.param ("play") ? 0.0f : 1.0f); break;
        case BtnSave:       saveMidi(); break;
        case LaneMute:      p.laneMute[l] = p.laneMute[l] ? 0 : 1; break;
        case LaneSolo:      p.laneSolo[l] = p.laneSolo[l] ? 0 : 1; break;
        case LaneLock:      p.laneLock[l] = p.laneLock[l] ? 0 : 1; break;
        case LaneRnd:
            p.saveUndo();
            p.randomizeLane (t.lane);
            if (t.lane == 3 || t.lane == 4) p.chokeHats();
            break;
        default: break;
    }
}

void DrumEditor::mouseDown (const juce::MouseEvent& e)
{
    const auto t = targetAt (e.getPosition());
    active = t;

    if (e.mods.isPopupMenu())
    {
        if (t.kind == LaneNote)
        {
            processor.learnLane = processor.learnLane.load() == t.lane ? -1 : t.lane;
        }
        else if (t.kind == Grid)
        {
            velLane = t.lane; velStep = t.step;
            if (processor.getStep (velLane, velStep) == 0) processor.setStep (velLane, velStep, 100);
            dragStartY = e.y;
            dragStartValue = processor.getStep (velLane, velStep);
            velDragging = true;
        }
        repaint();
        return;
    }

    if (isValueBox (t.kind))
    {
        dragStartY = e.y;
        dragStartValue = getValue (t.kind, t.lane);
    }
    else if (t.kind == Grid)
    {
        paintValue = processor.getStep (t.lane, t.step) > 0 ? 0 : 100;
        processor.setStep (t.lane, t.step, paintValue);
        painting = true;
    }
    else if (t.kind != DragMidi && t.kind != None)
    {
        click (t, e);
    }
    repaint();
}

void DrumEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (velDragging)
    {
        processor.setStep (velLane, velStep,
                           juce::jlimit (1, 127, dragStartValue + (int) std::floor ((dragStartY - e.y) * 1.5f)));
    }
    else if (painting)
    {
        const auto t = targetAt (e.getPosition());
        if (t.kind == Grid) processor.setStep (t.lane, t.step, paintValue);
    }
    else if (isValueBox (active.kind))
    {
        setValue (active.kind, active.lane,
                  dragStartValue + (int) std::floor ((dragStartY - e.y) / dragScale (active.kind)));
    }
    else if (active.kind == DragMidi && ! midiDragging && e.getDistanceFromDragStart() > 4)
    {
        startMidiDrag();
    }
    repaint();
}

void DrumEditor::mouseUp (const juce::MouseEvent&)
{
    painting = false;
    velDragging = false;
    active = {};
    repaint();
}

void DrumEditor::mouseMove (const juce::MouseEvent& e)
{
    const auto t = targetAt (e.getPosition());
    if (t.kind != hover.kind || t.lane != hover.lane)
    {
        hover = t;
        setMouseCursor (t.kind == DragMidi ? juce::MouseCursor::DraggingHandCursor
                                           : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void DrumEditor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const auto t = targetAt (e.getPosition());
    if (isValueBox (t.kind) && w.deltaY != 0.0f)
        setValue (t.kind, t.lane, getValue (t.kind, t.lane) + (w.deltaY > 0 ? 1 : -1));
}

// ---------------------------------------------------------------------------
void DrumEditor::startMidiDrag()
{
    midiDragging = true;
    const auto file = processor.writeMidiFile();
    if (! file.existsAsFile())
    {
        midiDragging = false;
        return;
    }

    juce::Component::SafePointer<DrumEditor> safe (this);
    juce::DragAndDropContainer::performExternalDragDropOfFiles (
        juce::StringArray { file.getFullPathName() }, false, this,
        [safe]
        {
            if (safe != nullptr)
            {
                safe->midiDragging = false;
                safe->active = {};
                safe->repaint();
            }
        });
}

void DrumEditor::saveMidi()
{
    const auto temp = processor.writeMidiFile();
    chooser = std::make_unique<juce::FileChooser> (
        "Save drum pattern as MIDI",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (temp.getFileName()),
        "*.mid");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [temp] (const juce::FileChooser& fc)
                          {
                              auto dest = fc.getResult();
                              if (dest != juce::File())
                                  temp.copyFileTo (dest.withFileExtension ("mid"));
                          });
}

// ---------------------------------------------------------------------------
void DrumEditor::drawButton (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text,
                             bool on, juce::Colour onCol, bool bold)
{
    const bool isHover = r.contains (getMouseXYRelative()) && isMouseOver (true);
    g.setColour (on ? onCol : (isHover ? btnHover : btnOff));
    g.fillRect (r);
    g.setColour (on ? rgb (0.08f, 0.08f, 0.1f) : textCol);
    g.setFont (juce::FontOptions (bold ? 15.0f : 14.0f, bold ? juce::Font::bold : juce::Font::plain));
    g.drawText (text, r, juce::Justification::centred, false);
}

void DrumEditor::drawValueBox (juce::Graphics& g, Kind k, int lane, const juce::String& text, bool fill)
{
    const auto r = rectFor (k, lane);
    g.setColour (rgb (0.17f, 0.17f, 0.2f));
    g.fillRect (r);
    if (fill)
    {
        const float f = (float) (getValue (k, lane) - minValue (k)) / (float) (maxValue (k) - minValue (k));
        g.setColour (rgb (0.25f, 0.42f, 0.68f));
        g.fillRect (r.withWidth (juce::roundToInt (r.getWidth() * f)));
    }
    if (active.kind == k && active.lane == lane)
    {
        g.setColour (rgb (0.6f, 0.75f, 1.0f));
        g.drawRect (r);
    }
    g.setColour (textCol);
    g.drawText (text, r, juce::Justification::centred, false);
}

void DrumEditor::paint (juce::Graphics& g)
{
    auto& p = processor;
    g.fillAll (bg);

    // ----- top bar -----
    drawButton (g, rectFor (BtnRandomize), "RANDOMIZE", true, rgb (0.88f, 0.28f, 0.3f), true);
    drawButton (g, rectFor (BtnMutate), "Mutate", false, {});
    drawButton (g, rectFor (BtnClear), "Clear", false, {});
    drawButton (g, rectFor (BtnUndo), "Undo", false, {});
    drawButton (g, rectFor (BtnStyle), juce::String ("<   ") + DrumProcessor::styleName (p.param ("style")) + "   >", false, {});

    g.setFont (juce::FontOptions (14.0f));
    drawValueBox (g, ValComplexity, -1, "Complexity " + juce::String (p.param ("complexity")) + "%", true);
    drawValueBox (g, ValSwing, -1, "Swing " + juce::String (p.param ("swing")) + "%", true);
    drawButton (g, rectFor (BtnRate), juce::String ("Rate ") + DrumProcessor::rateName (p.param ("rate")), false, {});
    drawButton (g, rectFor (BtnMap), p.mapSel == 0 ? "Notes: GM" : p.mapSel == 1 ? "Notes: 36+" : "Notes: 60+", false, {});

    // ----- second row -----
    g.setFont (juce::FontOptions (13.0f));
    drawValueBox (g, ValVel, -1, "Vel humanize " + juce::String (p.param ("velhumanize")), true);
    drawValueBox (g, ValNoteLen, -1, "Note length " + juce::String (p.param ("notelength")) + "%", true);
    drawValueBox (g, ValChannel, -1, "MIDI ch " + juce::String (p.param ("channel") + 1), false);
    drawButton (g, rectFor (BtnPlay), p.param ("play") ? "Play with transport: On" : "Play with transport: Off",
                false, {});
    if (p.param ("play"))
    {
        g.setColour (p.isPlayingNow ? rgb (0.4f, 0.9f, 0.5f) : rgb (0.45f, 0.45f, 0.5f));
        g.fillEllipse (rectFor (BtnPlay).toFloat().removeFromLeft (16).withSizeKeepingCentre (8, 8).translated (4, 0));
    }

    const int bars = std::array<int, 4> { 1, 2, 4, 8 } [(size_t) juce::jlimit (0, 3, p.param ("exportbars"))];
    drawButton (g, rectFor (BtnExportBars), "Drag: " + juce::String (bars) + (bars > 1 ? " bars" : " bar"), false, {});
    drawButton (g, rectFor (BtnSave), "Save MIDI...", false, {});

    // drag box
    {
        auto r = rectFor (DragMidi).toFloat().reduced (1.0f);
        const bool hot = hover.kind == DragMidi || midiDragging;
        g.setColour (hot ? accent.withAlpha (0.25f) : rgb (0.16f, 0.16f, 0.19f));
        g.fillRoundedRectangle (r, 5.0f);
        juce::Path border, dashed;
        border.addRoundedRectangle (r, 5.0f);
        const float dash[] = { 5.0f, 4.0f };
        juce::PathStrokeType (1.5f).createDashedStroke (dashed, border, dash, 2);
        g.setColour (accent);
        g.fillPath (dashed);
        g.setColour (textCol);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawText (juce::String::fromUTF8 ("\xe2\x87\xa9  Drag MIDI"), rectFor (DragMidi), juce::Justification::centred);
    }

    // learn hint
    const int learn = p.learnLane.load();
    if (learn >= 0)
    {
        g.setColour (rgb (1.0f, 0.8f, 0.3f));
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (juce::String ("Learning: play a note for ") + DrumProcessor::laneName (learn),
                    juce::Rectangle<int> (420, 78, 400, 18), juce::Justification::centredLeft);
    }

    // ----- header -----
    g.setColour (dimText);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("Lane",   18,  gy - 18, 50, 16, juce::Justification::left);
    g.drawText ("Note",   172, gy - 18, 40, 16, juce::Justification::left);
    g.drawText ("Len",    216, gy - 18, 40, 16, juce::Justification::left);
    g.drawText ("Chance", 250, gy - 18, 50, 16, juce::Justification::left);
    for (int s = 0; s < 8; ++s)
        g.drawText (juce::String (s * 4 + 1), gx + s * 4 * cw + 3, gy - 18, 30, 16, juce::Justification::left);

    // ----- lanes -----
    for (int l = 0; l < DrumProcessor::NL; ++l)
    {
        const size_t li = (size_t) l;
        const int y = gy + l * rh;

        g.setColour (laneColour (l));
        g.fillRect (10, y + 8, 4, 22);
        g.setColour (textCol);
        g.setFont (juce::FontOptions (14.0f));
        g.drawText (DrumProcessor::laneName (l), 18, y + 8, 48, 22, juce::Justification::centredLeft);

        g.setFont (juce::FontOptions (13.0f));
        drawButton (g, rectFor (LaneMute, l), "M", p.laneMute[li] != 0, rgb (0.95f, 0.55f, 0.2f));
        drawButton (g, rectFor (LaneSolo, l), "S", p.laneSolo[li] != 0, rgb (0.95f, 0.85f, 0.25f));
        drawButton (g, rectFor (LaneLock, l), "L", p.laneLock[li] != 0, rgb (0.5f, 0.7f, 0.95f));
        drawButton (g, rectFor (LaneRnd, l), "Rnd", false, {});

        g.setFont (juce::FontOptions (13.0f));
        drawValueBox (g, LaneNote, l, learn == l ? juce::String ("learn") : juce::String (p.laneNote[li].load()), false);
        drawValueBox (g, LaneLen, l, juce::String (p.laneLen[li].load()), false);
        drawValueBox (g, LaneChance, l, juce::String (p.laneChance[li].load()) + "%", true);

        // step grid
        const int len = p.laneLen[li];
        const int cur = p.isPlayingNow ? p.currentStep[li].load() : -1;
        for (int s = 0; s < DrumProcessor::NS; ++s)
        {
            const int x = gx + s * cw;
            const bool inLen = s < len;
            g.setColour (inLen ? ((s / 4) % 2 == 0 ? rgb (0.21f, 0.21f, 0.25f) : rgb (0.16f, 0.16f, 0.19f))
                               : rgb (0.1f, 0.1f, 0.11f));
            g.fillRect (x, y + 4, cw - 2, rh - 8);

            const int v = p.getStep (l, s);
            if (v > 0)
            {
                const float bh = (rh - 8) * (0.25f + 0.75f * v / 127.0f);
                g.setColour (laneColour (l).withAlpha (inLen ? 1.0f : 0.3f));
                g.fillRect ((float) x, (float) (y + 4 + (rh - 8)) - bh, (float) (cw - 2), bh);
            }
            if (cur == s)
            {
                g.setColour (juce::Colours::white);
                g.drawRect (x, y + 4, cw - 2, rh - 8);
            }
        }
    }

    // ----- footer -----
    g.setColour (rgb (0.5f, 0.5f, 0.55f));
    g.setFont (juce::FontOptions (13.0f));
    const int fy = gy + DrumProcessor::NL * rh + 4;
    g.drawText ("Click/drag on grid: paint steps.  Right-drag a step: velocity.  Note/Len/Chance boxes: drag up-down or mouse wheel.",
                10, fy, 930, 16, juce::Justification::left);
    g.drawText ("Right-click a Note box, then play a key: learn note.  M/S/L = mute/solo/lock.  Drag MIDI: drop the pattern onto a REAPER track.",
                10, fy + 16, 930, 16, juce::Justification::left);
}
