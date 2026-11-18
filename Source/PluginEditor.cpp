#include "PluginEditor.h"

namespace
{
    juce::Colour hex (juce::uint32 rgbValue) { return juce::Colour (0xff000000 | rgbValue); }

    // ---- palette (dark studio-hardware look, blue accent) ----
    const juce::Colour bodyTop    = hex (0x34383d);
    const juce::Colour bodyBottom = hex (0x25282c);
    const juce::Colour panelEdge  = hex (0x3b4046);
    const juce::Colour insetFill  = hex (0x16181a);
    const juce::Colour textCol    = hex (0xdadee3);
    const juce::Colour dimText    = hex (0x8c939b);
    const juce::Colour faintText  = hex (0x5d646b);
    const juce::Colour blue       = hex (0x2ea3f2);
    const juce::Colour blueDark   = hex (0x1b6aa0);
    const juce::Colour green      = hex (0x8cc83f);
    const juce::Colour yellow     = hex (0xe0bc2e);
    const juce::Colour orange     = hex (0xef8a2e);
    const juce::Colour red        = hex (0xe35a4b);
    const juce::Colour purple     = hex (0xa774da);
    const juce::Colour teal       = hex (0x36c3be);
    const juce::Colour pink       = hex (0xde6fae);

    juce::Colour laneColour (int l)
    {
        switch (l)
        {
            case 0:  return blue;
            case 1:  return orange;
            case 2:  return yellow;
            case 3:  return green;
            case 4:  return teal;
            case 5:  return purple;
            case 6:  return pink;
            default: return red;
        }
    }

    juce::FontOptions font (float h, bool bold = false, float kerning = 0.0f)
    {
        return juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain).withKerningFactor (kerning);
    }

    void drawPowerIcon (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour colour)
    {
        juce::Path p;
        p.addCentredArc (c.x, c.y, r, r, 0.0f, 0.65f, juce::MathConstants<float>::twoPi - 0.65f, true);
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (c.x, c.y - r - 1.0f, c.x, c.y - r * 0.15f, 1.6f);
    }

    const std::array<int, 4> exportBarCounts { 1, 2, 4, 8 };
    const char* mapNames[3] = { "GM drums", "36 and up", "60 and up" };
}

// ===========================================================================
DrumEditor::DrumEditor (DrumProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    menuLaf.setColour (juce::PopupMenu::backgroundColourId, hex (0x23262a));
    menuLaf.setColour (juce::PopupMenu::textColourId, textCol);
    menuLaf.setColour (juce::PopupMenu::highlightedBackgroundColourId, blueDark);
    menuLaf.setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    menuLaf.setColour (juce::PopupMenu::headerTextColourId, dimText);

    setResizable (true, true);
    setResizeLimits ((int) (baseW * 0.6f), (int) (baseH * 0.6f), (int) (baseW * 1.6f), (int) (baseH * 1.6f));
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseW / (double) baseH);

    const float s = juce::jlimit (0.6f, 1.6f, processor.uiScale.load());
    setSize (juce::roundToInt (baseW * s), juce::roundToInt (baseH * s));
    startTimerHz (30);
}

DrumEditor::~DrumEditor() { stopTimer(); }

void DrumEditor::timerCallback() { repaint(); }

void DrumEditor::resized()
{
    processor.uiScale = scale();   // remembered (also saved with the project)
}

juce::Point<int> DrumEditor::toBase (juce::Point<float> p) const
{
    const float s = scale();
    return { (int) std::floor (p.x / s), (int) std::floor (p.y / s) };
}

// ===========================================================================
// Layout
juce::Point<float> DrumEditor::knobCentre (Kind k) const
{
    switch (k)
    {
        case KnobComplexity: return { 102.0f, 128.0f };
        case KnobSwing:      return { 102.0f, 278.0f };
        case KnobVel:        return { 1138.0f, 128.0f };
        case KnobNoteLen:    return { 1138.0f, 278.0f };
        default:             return {};
    }
}

juce::Rectangle<int> DrumEditor::rectFor (Kind k, int lane) const
{
    const int y = gy + lane * rh;
    switch (k)
    {
        case StylePrev:      return { 470, 6, 30, 28 };
        case StyleName:      return { 500, 6, 240, 28 };
        case StyleNext:      return { 740, 6, 30, 28 };

        case TabRandomize:   return { 212, 56, 200, 28 };
        case TabMutate:      return { 416, 56, 200, 28 };
        case TabClear:       return { 620, 56, 200, 28 };
        case TabUndo:        return { 824, 56, 204, 28 };

        case KnobComplexity: case KnobSwing: case KnobVel: case KnobNoteLen:
        {
            const auto c = knobCentre (k);
            return juce::Rectangle<int> (0, 0, 100, 100).withCentre ({ (int) c.x, (int) c.y });
        }

        case BoxRate:        return { 88, 388, 96, 26 };
        case BoxMap:         return { 88, 420, 96, 26 };
        case BoxChannel:     return { 88, 452, 96, 26 };
        case TogglePlay:     return { 20, 522, 164, 30 };
        case BoxExportBars:  return { 1124, 388, 96, 26 };
        case DragMidi:       return { 1056, 422, 164, 62 };
        case BtnSave:        return { 1056, 492, 164, 26 };

        case LaneMute:       return { 276, y + 17, 20, 20 };
        case LaneSolo:       return { 298, y + 17, 20, 20 };
        case LaneLock:       return { 320, y + 17, 20, 20 };
        case LaneRnd:        return { 346, y + 17, 28, 20 };
        case LaneNote:       return { 380, y + 16, 30, 22 };
        case LaneLen:        return { 414, y + 16, 22, 22 };
        case LaneChance:     return { 440, y + 16, 38, 22 };
        default:             return {};
    }
}

DrumEditor::Target DrumEditor::targetAt (juce::Point<int> pt) const
{
    for (auto k : { StylePrev, StyleName, StyleNext, TabRandomize, TabMutate, TabClear, TabUndo,
                    BoxRate, BoxMap, BoxChannel, TogglePlay, BoxExportBars, DragMidi, BtnSave })
        if (rectFor (k).contains (pt))
            return { k, -1, -1 };

    for (auto k : { KnobComplexity, KnobSwing, KnobVel, KnobNoteLen })
        if (pt.toFloat().getDistanceFrom (knobCentre (k)) < 50.0f)
            return { k, -1, -1 };

    if (pt.y >= gy && pt.y < gy + DrumProcessor::NL * rh)
    {
        const int lane = (pt.y - gy) / rh;
        for (auto k : { LaneMute, LaneSolo, LaneLock, LaneRnd, LaneNote, LaneLen, LaneChance })
            if (rectFor (k, lane).contains (pt))
                return { k, lane, -1 };

        const int step = (pt.x - gx) / cw;
        if (pt.x >= gx && step >= 0 && step < DrumProcessor::NS)
            return { Grid, lane, step };
    }
    return {};
}

// ===========================================================================
// Values
bool DrumEditor::isKnob (Kind k) const
{
    return k == KnobComplexity || k == KnobSwing || k == KnobVel || k == KnobNoteLen;
}

bool DrumEditor::isValueBox (Kind k) const
{
    return isKnob (k) || k == LaneNote || k == LaneLen || k == LaneChance;
}

int DrumEditor::getValue (Kind k, int lane) const
{
    switch (k)
    {
        case KnobComplexity: return processor.param ("complexity");
        case KnobSwing:      return processor.param ("swing");
        case KnobVel:        return processor.param ("velhumanize");
        case KnobNoteLen:    return processor.param ("notelength");
        case LaneNote:       return processor.laneNote[(size_t) lane];
        case LaneLen:        return processor.laneLen[(size_t) lane];
        case LaneChance:     return processor.laneChance[(size_t) lane];
        default:             return 0;
    }
}

void DrumEditor::setValue (Kind k, int lane, int v)
{
    v = juce::jlimit (minValue (k), maxValue (k), v);
    if (v == getValue (k, lane)) return;
    switch (k)
    {
        case KnobComplexity: processor.setParam ("complexity", (float) v); break;
        case KnobSwing:      processor.setParam ("swing", (float) v); break;
        case KnobVel:        processor.setParam ("velhumanize", (float) v); break;
        case KnobNoteLen:    processor.setParam ("notelength", (float) v); break;
        case LaneNote:       processor.laneNote[(size_t) lane] = v; break;
        case LaneLen:        processor.laneLen[(size_t) lane] = v; break;
        case LaneChance:     processor.laneChance[(size_t) lane] = v; break;
        default: break;
    }
}

int DrumEditor::minValue (Kind k) const
{
    return k == KnobNoteLen ? 5 : k == LaneLen ? 1 : 0;
}

int DrumEditor::maxValue (Kind k) const
{
    switch (k)
    {
        case KnobSwing: return 75;
        case KnobVel:   return 40;
        case LaneNote:  return 127;
        case LaneLen:   return DrumProcessor::NS;
        default:        return 100;
    }
}

int DrumEditor::defaultValue (Kind k) const
{
    switch (k)
    {
        case KnobComplexity: return 50;
        case KnobSwing:      return 0;
        case KnobVel:        return 8;
        case KnobNoteLen:    return 50;
        case LaneLen:        return 16;
        case LaneChance:     return 100;
        default:             return 0;
    }
}

// pixels of vertical mouse movement per value step
float DrumEditor::dragScale (Kind k) const
{
    switch (k)
    {
        case KnobComplexity: return 1.5f;
        case KnobSwing:      return 2.0f;
        case KnobVel:        return 3.5f;
        case KnobNoteLen:    return 1.5f;
        case LaneNote:       return 3.0f;
        case LaneLen:        return 4.0f;
        default:             return 1.0f;
    }
}

// ===========================================================================
// Mouse
void DrumEditor::click (const Target& t, const juce::MouseEvent&)
{
    auto& p = processor;
    const size_t l = (size_t) juce::jmax (0, t.lane);

    switch (t.kind)
    {
        case TabRandomize: p.randomizeAll(); break;
        case TabMutate:    p.mutateAll(); break;
        case TabClear:     p.clearAll(); break;
        case TabUndo:      p.undo(); break;
        case StylePrev:    p.setParam ("style", (float) ((p.param ("style") + 6) % 7)); break;
        case StyleNext:    p.setParam ("style", (float) ((p.param ("style") + 1) % 7)); break;
        case StyleName: case BoxRate: case BoxMap: case BoxChannel: case BoxExportBars:
            showMenu (t.kind);
            break;
        case TogglePlay:   p.setParam ("play", p.param ("play") ? 0.0f : 1.0f); break;
        case BtnSave:      saveMidi(); break;
        case LaneMute:     p.laneMute[l] = p.laneMute[l] ? 0 : 1; break;
        case LaneSolo:     p.laneSolo[l] = p.laneSolo[l] ? 0 : 1; break;
        case LaneLock:     p.laneLock[l] = p.laneLock[l] ? 0 : 1; break;
        case LaneRnd:
            p.saveUndo();
            p.randomizeLane (t.lane);
            if (t.lane == 3 || t.lane == 4) p.chokeHats();
            break;
        default: break;
    }
}

void DrumEditor::showMenu (Kind k)
{
    juce::PopupMenu m;
    m.setLookAndFeel (&menuLaf);
    auto& p = processor;
    juce::Component::SafePointer<DrumEditor> safe (this);

    std::function<void (int)> apply;
    switch (k)
    {
        case StyleName:
            m.addSectionHeader ("STYLE");
            for (int i = 0; i < 7; ++i) m.addItem (i + 1, DrumProcessor::styleName (i), true, p.param ("style") == i);
            apply = [safe] (int r) { if (safe) safe->processor.setParam ("style", (float) (r - 1)); };
            break;
        case BoxRate:
            m.addSectionHeader ("STEP RATE");
            for (int i = 0; i < 5; ++i) m.addItem (i + 1, DrumProcessor::rateName (i), true, p.param ("rate") == i);
            apply = [safe] (int r) { if (safe) safe->processor.setParam ("rate", (float) (r - 1)); };
            break;
        case BoxMap:
            m.addSectionHeader ("NOTE MAP (sets all lanes)");
            for (int i = 0; i < 3; ++i) m.addItem (i + 1, mapNames[i], true, p.mapSel.load() == i);
            apply = [safe] (int r) { if (safe) safe->processor.applyMap (r - 1); };
            break;
        case BoxChannel:
            m.addSectionHeader ("MIDI CHANNEL");
            for (int i = 0; i < 16; ++i) m.addItem (i + 1, "Channel " + juce::String (i + 1), true, p.param ("channel") == i);
            apply = [safe] (int r) { if (safe) safe->processor.setParam ("channel", (float) (r - 1)); };
            break;
        case BoxExportBars:
            m.addSectionHeader ("DRAG / SAVE LENGTH");
            for (int i = 0; i < 4; ++i)
                m.addItem (i + 1, juce::String (exportBarCounts[(size_t) i]) + (i == 0 ? " bar" : " bars"), true,
                           p.param ("exportbars") == i);
            apply = [safe] (int r) { if (safe) safe->processor.setParam ("exportbars", (float) (r - 1)); };
            break;
        default: return;
    }

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition()
                         .withStandardItemHeight (juce::roundToInt (22 * scale())),
                     [apply] (int result) { if (result > 0 && apply) apply (result); });
}

void DrumEditor::mouseDown (const juce::MouseEvent& e)
{
    const auto pt = mouseBase (e);
    const auto t = targetAt (pt);
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
            dragStartY = pt.y;
            dragStartValue = processor.getStep (velLane, velStep);
            velDragging = true;
        }
        repaint();
        return;
    }

    if (isValueBox (t.kind))
    {
        dragStartY = pt.y;
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
    const auto pt = mouseBase (e);
    if (velDragging)
    {
        processor.setStep (velLane, velStep,
                           juce::jlimit (1, 127, dragStartValue + (int) std::floor ((dragStartY - pt.y) * 1.5f)));
    }
    else if (painting)
    {
        const auto t = targetAt (pt);
        if (t.kind == Grid) processor.setStep (t.lane, t.step, paintValue);
    }
    else if (isValueBox (active.kind))
    {
        const float fine = e.mods.isShiftDown() ? 4.0f : 1.0f;
        setValue (active.kind, active.lane,
                  dragStartValue + (int) std::floor ((float) (dragStartY - pt.y) / (dragScale (active.kind) * fine)));
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
    const auto t = targetAt (mouseBase (e));
    if (t.kind != hover.kind || t.lane != hover.lane || t.step != hover.step)
    {
        hover = t;
        setMouseCursor (t.kind == DragMidi ? juce::MouseCursor::DraggingHandCursor
                      : isValueBox (t.kind) ? juce::MouseCursor::UpDownResizeCursor
                      : t.kind == None ? juce::MouseCursor::NormalCursor
                                       : juce::MouseCursor::PointingHandCursor);
        repaint();
    }
}

void DrumEditor::mouseExit (const juce::MouseEvent&)
{
    hover = {};
    repaint();
}

void DrumEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto t = targetAt (mouseBase (e));
    if (isKnob (t.kind) || t.kind == LaneLen || t.kind == LaneChance)
        setValue (t.kind, t.lane, defaultValue (t.kind));
}

void DrumEditor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const auto t = targetAt (mouseBase (e));
    if (isValueBox (t.kind) && w.deltaY != 0.0f)
        setValue (t.kind, t.lane, getValue (t.kind, t.lane) + (w.deltaY > 0 ? 1 : -1));
}

// ===========================================================================
// MIDI export
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

// ===========================================================================
// Drawing helpers
bool DrumEditor::isHot (Kind k, int lane) const
{
    return (hover.kind == k && hover.lane == lane) || (active.kind == k && active.lane == lane);
}

void DrumEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> r)
{
    const auto rf = r.toFloat();
    juce::DropShadow (juce::Colours::black.withAlpha (0.45f), 8, { 0, 2 }).drawForRectangle (g, r);
    g.setGradientFill (juce::ColourGradient (hex (0x24272b), rf.getX(), rf.getY(),
                                             hex (0x1c1e21), rf.getX(), rf.getBottom(), false));
    g.fillRoundedRectangle (rf, 4.0f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (rf.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (rf.getX() + 4, rf.getY() + 1.5f, rf.getRight() - 4, rf.getY() + 1.5f, 1.0f);
}

void DrumEditor::drawSectionHeader (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text, juce::Colour c)
{
    drawPowerIcon (g, { (float) r.getX() + 8.0f, (float) r.getCentreY() }, 5.5f, c);
    g.setColour (textCol);
    g.setFont (font (12.5f, true, 0.08f));
    const int textW = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), text) + 4;
    g.drawText (text, r.getX() + 20, r.getY(), textW + 4, r.getHeight(), juce::Justification::centredLeft);
    g.setColour (panelEdge);
    g.drawLine ((float) (r.getX() + 28 + textW), (float) r.getCentreY(), (float) r.getRight(), (float) r.getCentreY(), 1.0f);
}

void DrumEditor::drawKnob (juce::Graphics& g, Kind k, const juce::String& label, const juce::String& valueText, float R)
{
    const auto c = knobCentre (k);
    const float frac = (float) (getValue (k, -1) - minValue (k)) / (float) (maxValue (k) - minValue (k));
    const float a0 = -2.356f, a1 = 2.356f;
    const float a = a0 + frac * (a1 - a0);
    const bool hot = isHot (k);

    // label above, value below (blue), like hardware panel printing
    g.setColour (textCol);
    g.setFont (font (12.5f, true, 0.08f));
    g.drawText (label, juce::Rectangle<float> (c.x - 90, c.y - R - 32, 180, 16), juce::Justification::centred);
    g.setColour (blue);
    g.setFont (font (13.0f, false, 0.02f));
    g.drawText (valueText, juce::Rectangle<float> (c.x - 90, c.y + R + 13, 180, 16), juce::Justification::centred);

    // tick dots around the knob
    for (int i = 0; i <= 24; ++i)
    {
        const float ta = a0 + (a1 - a0) * (float) i / 24.0f;
        const float tr = R + 11.0f;
        const juce::Point<float> tp (c.x + std::sin (ta) * tr, c.y - std::cos (ta) * tr);
        g.setColour ((float) i / 24.0f <= frac + 0.001f ? blue.withAlpha (0.85f) : faintText.withAlpha (0.6f));
        g.fillEllipse (juce::Rectangle<float> (2.2f, 2.2f).withCentre (tp));
    }

    // value ring: dark track, glow and bright arc
    const float ringR = R + 4.0f;
    juce::Path track, arc;
    track.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, a0, a1, true);
    arc.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, a0, a, true);
    g.setColour (hex (0x111315));
    g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (frac > 0.001f)
    {
        g.setColour (blue.withAlpha (0.18f));
        g.strokePath (arc, juce::PathStrokeType (11.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (blue);
        g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // metallic knob body
    const float bodyR = R - 3.0f;
    const auto body = juce::Rectangle<float> (bodyR * 2, bodyR * 2).withCentre (c);
    juce::Path bodyPath; bodyPath.addEllipse (body);
    juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 10, { 0, 4 }).drawForPath (g, bodyPath);

    g.setGradientFill (juce::ColourGradient (hot ? hex (0xffffff) : hex (0xf0f2f4), c.x - bodyR * 0.35f, c.y - bodyR * 0.55f,
                                             hex (0x6f757b), c.x + bodyR * 0.5f, c.y + bodyR * 0.9f, true));
    g.fillEllipse (body);
    g.setColour (hex (0x3e4348));
    g.drawEllipse (body, 1.2f);

    // inner cap
    const auto cap = body.reduced (bodyR * 0.3f);
    g.setGradientFill (juce::ColourGradient (hex (0xd8dbde), cap.getX(), cap.getY(),
                                             hex (0x9aa0a5), cap.getRight(), cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawEllipse (cap, 0.8f);

    // pointer
    const juce::Point<float> p1 (c.x + std::sin (a) * bodyR * 0.3f, c.y - std::cos (a) * bodyR * 0.3f);
    const juce::Point<float> p2 (c.x + std::sin (a) * bodyR * 0.88f, c.y - std::cos (a) * bodyR * 0.88f);
    g.setColour (hex (0x22252a));
    g.drawLine ({ p1, p2 }, 3.0f);
}

void DrumEditor::drawTab (juce::Graphics& g, Kind k, const juce::String& text, juce::Colour c, bool highlighted)
{
    const auto r = rectFor (k).toFloat();
    const bool hot = isHot (k);
    const bool down = active.kind == k;

    juce::Colour top = hex (0x34383d), bottom = hex (0x26292d);
    if (highlighted) { top = top.interpolatedWith (c, 0.28f); bottom = bottom.interpolatedWith (c, 0.18f); }
    if (hot)         { top = top.brighter (0.15f); bottom = bottom.brighter (0.1f); }
    if (down)        { std::swap (top, bottom); }

    g.setGradientFill (juce::ColourGradient (top, r.getX(), r.getY(), bottom, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (highlighted ? c.withAlpha (0.8f) : panelEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    if (highlighted)
    {
        g.setColour (c);
        g.fillRect (r.getX() + 3, r.getBottom() - 2.5f, r.getWidth() - 6, 2.0f);
    }

    drawPowerIcon (g, { r.getX() + 16.0f, r.getCentreY() }, 5.5f, c);
    g.setColour (highlighted ? juce::Colours::white : c);
    g.setFont (font (13.5f, true, 0.1f));
    g.drawText (text, r.withTrimmedLeft (28), juce::Justification::centred);
}

void DrumEditor::drawMenuBox (juce::Graphics& g, Kind k, const juce::String& label, const juce::String& value)
{
    const auto r = rectFor (k);
    const bool hot = isHot (k);

    g.setColour (dimText);
    g.setFont (font (11.5f, true, 0.08f));
    g.drawText (label, juce::Rectangle<int> (r.getX() - 72, r.getY(), 68, r.getHeight()), juce::Justification::centredLeft);

    const auto rf = r.toFloat();
    g.setColour (hot ? hex (0x1d2023) : insetFill);
    g.fillRoundedRectangle (rf, 3.0f);
    g.setColour (hot ? blue.withAlpha (0.7f) : panelEdge);
    g.drawRoundedRectangle (rf.reduced (0.5f), 3.0f, 1.0f);

    g.setColour (textCol);
    g.setFont (font (13.0f));
    g.drawText (value, r.withTrimmedLeft (8).withTrimmedRight (18), juce::Justification::centredLeft);

    juce::Path tri;
    const float tx = rf.getRight() - 12.0f, ty = rf.getCentreY();
    tri.addTriangle (tx - 4, ty - 2, tx + 4, ty - 2, tx, ty + 3);
    g.setColour (blue);
    g.fillPath (tri);
}

void DrumEditor::drawSmallButton (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text,
                                  bool on, juce::Colour onCol, bool hot)
{
    const auto rf = r.toFloat();
    if (on)
    {
        g.setGradientFill (juce::ColourGradient (onCol.brighter (0.15f), rf.getX(), rf.getY(),
                                                 onCol.darker (0.2f), rf.getX(), rf.getBottom(), false));
        g.fillRoundedRectangle (rf, 3.0f);
        g.setColour (hex (0x111214));
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (hot ? hex (0x41464c) : hex (0x363a3f), rf.getX(), rf.getY(),
                                                 hot ? hex (0x30343a) : hex (0x282b2f), rf.getX(), rf.getBottom(), false));
        g.fillRoundedRectangle (rf, 3.0f);
        g.setColour (panelEdge);
        g.drawRoundedRectangle (rf.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (hot ? textCol : dimText);
    }
    g.setFont (font (11.5f, true, 0.05f));
    g.drawText (text, r, juce::Justification::centred);
}

void DrumEditor::drawLaneValue (juce::Graphics& g, Kind k, int lane, const juce::String& text, float fill)
{
    const auto r = rectFor (k, lane);
    const auto rf = r.toFloat();
    const bool hot = isHot (k, lane);
    g.setColour (insetFill);
    g.fillRoundedRectangle (rf, 3.0f);
    if (fill >= 0.0f)
    {
        g.setColour (blue.withAlpha (0.9f));
        g.fillRect (rf.getX() + 2, rf.getBottom() - 3.0f, (rf.getWidth() - 4) * fill, 2.0f);
    }
    g.setColour (hot ? blue.withAlpha (0.8f) : panelEdge);
    g.drawRoundedRectangle (rf.reduced (0.5f), 3.0f, 1.0f);
    g.setColour (hot ? juce::Colours::white : textCol);
    g.setFont (font (12.0f));
    g.drawText (text, r.withTrimmedBottom (1), juce::Justification::centred);
}

// ===========================================================================
void DrumEditor::drawTitleBar (juce::Graphics& g)
{
    auto& p = processor;
    g.setGradientFill (juce::ColourGradient (hex (0x1d1f22), 0, 0, hex (0x131517), 0, 40, false));
    g.fillRect (0, 0, baseW, 40);
    g.setColour (juce::Colours::black);
    g.drawLine (0, 40, (float) baseW, 40, 1.5f);

    // logo
    const juce::Point<float> lc (24.0f, 20.0f);
    g.setColour (blue);
    g.drawEllipse (juce::Rectangle<float> (18, 18).withCentre (lc), 2.0f);
    g.fillEllipse (juce::Rectangle<float> (6, 6).withCentre (lc));
    for (int i = 0; i < 4; ++i)
    {
        const float a = juce::MathConstants<float>::halfPi * (float) i + 0.785f;
        g.fillEllipse (juce::Rectangle<float> (3, 3).withCentre ({ lc.x + std::sin (a) * 13.0f, lc.y - std::cos (a) * 13.0f }));
    }

    g.setFont (font (17.0f, true, 0.06f));
    g.setColour (hex (0xe8ebee));
    g.drawText ("DRUM PATTERN", 44, 0, 150, 40, juce::Justification::centredLeft);
    g.setColour (blue);
    g.setFont (font (17.0f, false, 0.06f));
    g.drawText ("GENERATOR", 186, 0, 140, 40, juce::Justification::centredLeft);

    // style "preset" display
    g.setColour (dimText);
    g.setFont (font (11.5f, true, 0.1f));
    g.drawText ("STYLE", 404, 6, 60, 28, juce::Justification::centredRight);

    const auto field = rectFor (StylePrev).getUnion (rectFor (StyleNext)).toFloat();
    g.setColour (insetFill);
    g.fillRoundedRectangle (field, 3.0f);
    g.setColour (isHot (StyleName) ? blue.withAlpha (0.7f) : panelEdge);
    g.drawRoundedRectangle (field.reduced (0.5f), 3.0f, 1.0f);

    g.setColour (textCol);
    g.setFont (font (15.5f, false, 0.03f));
    g.drawText (DrumProcessor::styleName (p.param ("style")), rectFor (StyleName), juce::Justification::centred);

    for (auto k : { StylePrev, StyleNext })
    {
        const auto r = rectFor (k).toFloat();
        juce::Path tri;
        const float cx = r.getCentreX(), cy = r.getCentreY();
        if (k == StylePrev) tri.addTriangle (cx + 4, cy - 5, cx + 4, cy + 5, cx - 4, cy);
        else                tri.addTriangle (cx - 4, cy - 5, cx - 4, cy + 5, cx + 4, cy);
        g.setColour (isHot (k) ? juce::Colours::white : blue);
        g.fillPath (tri);
    }

    // right side: status
    const bool sync = p.param ("play") != 0;
    const bool playing = sync && p.isPlayingNow.load();
    const auto pill = juce::Rectangle<float> (1110, 10, 116, 20);
    g.setColour (insetFill);
    g.fillRoundedRectangle (pill, 10.0f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (pill.reduced (0.5f), 10.0f, 1.0f);
    g.setColour (playing ? green : sync ? faintText : orange);
    g.fillEllipse (juce::Rectangle<float> (8, 8).withCentre ({ pill.getX() + 13, pill.getCentreY() }));
    g.setColour (textCol);
    g.setFont (font (11.0f, true, 0.08f));
    g.drawText (playing ? "PLAYING" : sync ? "STOPPED" : "SYNC OFF", pill.withTrimmedLeft (22), juce::Justification::centredLeft);
}

void DrumEditor::drawGrid (juce::Graphics& g)
{
    auto& p = processor;
    const int learn = p.learnLane.load();
    const int gridW = DrumProcessor::NS * cw;

    // column headers
    g.setColour (dimText);
    g.setFont (font (10.5f, true, 0.08f));
    g.drawText ("LANE",    226, 92, 60, 14, juce::Justification::centredLeft);
    g.drawText ("M  S  L", 276, 92, 64, 14, juce::Justification::centred);
    g.setFont (font (9.5f, true, 0.02f));
    g.drawText ("RAND",    344, 92, 32, 14, juce::Justification::centred);
    g.drawText ("NOTE",    378, 92, 34, 14, juce::Justification::centred);
    g.drawText ("LEN",     412, 92, 26, 14, juce::Justification::centred);
    g.drawText ("CHANCE",  439, 92, 40, 14, juce::Justification::centred);

    // step number tabs
    for (int s = 0; s < DrumProcessor::NS; ++s)
    {
        const auto r = juce::Rectangle<float> ((float) (gx + s * cw), 91.0f, (float) (cw - 2), 15.0f);
        const bool beat = s % 4 == 0;
        g.setColour (beat ? blueDark : hex (0x2b2f34));
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (beat ? juce::Colours::white : dimText);
        g.setFont (font (9.0f, beat));
        g.drawText (juce::String (s + 1), r, juce::Justification::centred);
    }

    // grid background
    const auto area = juce::Rectangle<float> ((float) gx - 4, (float) gy - 2, (float) gridW + 6,
                                              (float) (DrumProcessor::NL * rh) + 4);
    g.setColour (hex (0x17191c));
    g.fillRoundedRectangle (area, 3.0f);

    for (int l = 0; l < DrumProcessor::NL; ++l)
    {
        const size_t li = (size_t) l;
        const int y = gy + l * rh;
        const auto lc = laneColour (l);
        const bool muted = p.laneMute[li] != 0;

        // row separator
        if (l > 0)
        {
            g.setColour (hex (0x2c3035));
            g.drawLine (216.0f, (float) y, (float) (gx + gridW), (float) y, 1.0f);
        }

        // lane label with colour strip
        g.setColour (lc.withAlpha (muted ? 0.35f : 1.0f));
        g.fillRoundedRectangle (216.0f, (float) y + 12.0f, 4.0f, (float) rh - 24.0f, 2.0f);
        g.setColour (muted ? faintText : textCol);
        g.setFont (font (13.0f, true, 0.03f));
        g.drawText (juce::String (DrumProcessor::laneName (l)).toUpperCase(), 226, y + 8, 50, rh - 16,
                    juce::Justification::centredLeft);

        drawSmallButton (g, rectFor (LaneMute, l), "M", muted, orange, isHot (LaneMute, l));
        drawSmallButton (g, rectFor (LaneSolo, l), "S", p.laneSolo[li] != 0, yellow, isHot (LaneSolo, l));
        drawSmallButton (g, rectFor (LaneLock, l), "L", p.laneLock[li] != 0, blue, isHot (LaneLock, l));
        drawSmallButton (g, rectFor (LaneRnd, l), "RND", false, {}, isHot (LaneRnd, l));

        drawLaneValue (g, LaneNote, l, learn == l ? juce::String ("?") : juce::String (p.laneNote[li].load()), -1.0f);
        drawLaneValue (g, LaneLen, l, juce::String (p.laneLen[li].load()), -1.0f);
        drawLaneValue (g, LaneChance, l, juce::String (p.laneChance[li].load()) + "%", (float) p.laneChance[li].load() / 100.0f);
        if (learn == l)
        {
            g.setColour (yellow);
            g.drawRoundedRectangle (rectFor (LaneNote, l).toFloat().expanded (1.5f), 3.0f, 1.5f);
        }

        // steps
        const int len = p.laneLen[li];
        const int cur = p.isPlayingNow ? p.currentStep[li].load() : -1;
        for (int s = 0; s < DrumProcessor::NS; ++s)
        {
            const auto cell = juce::Rectangle<float> ((float) (gx + s * cw), (float) y + 5.0f, (float) (cw - 2), (float) rh - 10.0f);
            const bool inLen = s < len;
            g.setColour (inLen ? ((s / 4) % 2 == 0 ? hex (0x272a2e) : hex (0x222528)) : hex (0x1a1c1e));
            g.fillRect (cell);

            const int v = p.getStep (l, s);
            if (v > 0)
            {
                const float bh = cell.getHeight() * (0.2f + 0.8f * (float) v / 127.0f);
                const auto bar = cell.withTop (cell.getBottom() - bh);
                const float alpha = (inLen ? 1.0f : 0.25f) * (muted ? 0.4f : 1.0f);
                g.setGradientFill (juce::ColourGradient (lc.brighter (0.25f).withAlpha (alpha), bar.getX(), bar.getY(),
                                                         lc.darker (0.35f).withAlpha (alpha), bar.getX(), bar.getBottom(), false));
                g.fillRect (bar);
                g.setColour (juce::Colours::white.withAlpha (0.45f * alpha));
                g.fillRect (bar.withHeight (1.5f));
            }

            if (hover.kind == Grid && hover.lane == l && hover.step == s)
            {
                g.setColour (juce::Colours::white.withAlpha (0.08f));
                g.fillRect (cell);
            }
            if (cur == s)
            {
                g.setColour (juce::Colours::white.withAlpha (0.1f));
                g.fillRect (cell);
                g.setColour (juce::Colours::white.withAlpha (0.85f));
                g.drawRect (cell, 1.0f);
            }
        }
    }

    // footer hints
    g.setColour (panelEdge);
    g.drawLine (212.0f, 556.0f, 1028.0f, 556.0f, 1.0f);
    if (learn >= 0)
    {
        g.setColour (yellow);
        g.setFont (font (13.0f, true, 0.03f));
        g.drawText (juce::String ("NOTE LEARN: play a key to set the note for ") + DrumProcessor::laneName (learn)
                        + "  (right-click the note box again to cancel)",
                    216, 564, 810, 18, juce::Justification::centredLeft);
    }
    else
    {
        g.setColour (dimText);
        g.setFont (font (11.5f));
        g.drawText ("Click / drag the grid to paint steps.  Right-drag a step to set its velocity.  "
                    "Drag knobs and number boxes up or down (Shift = fine, double-click = reset).",
                    216, 564, 810, 16, juce::Justification::centredLeft);
        g.drawText ("Right-click a NOTE box, then play a key to learn it.  M / S / L = mute, solo, lock "
                    "(locked lanes are never randomized).",
                    216, 582, 810, 16, juce::Justification::centredLeft);
    }
}

void DrumEditor::drawExport (juce::Graphics& g)
{
    auto& p = processor;

    // drag target
    {
        const auto r = rectFor (DragMidi).toFloat();
        const bool hot = isHot (DragMidi) || midiDragging;
        g.setColour (hot ? blue.withAlpha (0.2f) : insetFill);
        g.fillRoundedRectangle (r, 5.0f);
        juce::Path border, dashed;
        border.addRoundedRectangle (r.reduced (0.75f), 5.0f);
        const float dash[] = { 5.0f, 4.0f };
        juce::PathStrokeType (1.5f).createDashedStroke (dashed, border, dash, 2);
        g.setColour (blue);
        g.fillPath (dashed);

        // arrow icon
        const float ax = r.getX() + 26, ay = r.getCentreY();
        juce::Path arrow;
        arrow.startNewSubPath (ax, ay - 11); arrow.lineTo (ax, ay + 6);
        arrow.startNewSubPath (ax - 6, ay); arrow.lineTo (ax, ay + 7); arrow.lineTo (ax + 6, ay);
        g.strokePath (arrow, juce::PathStrokeType (2.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        g.drawLine (ax - 9, ay + 11, ax + 9, ay + 11, 2.0f);

        g.setColour (hot ? juce::Colours::white : textCol);
        g.setFont (font (14.0f, true, 0.1f));
        g.drawText ("DRAG MIDI", r.withTrimmedLeft (44).withTrimmedBottom (32), juce::Justification::bottomLeft);
        g.setColour (dimText);
        g.setFont (font (11.0f));
        g.drawText ("onto a track", r.withTrimmedLeft (44).withTrimmedTop (34), juce::Justification::topLeft);
    }

    drawSmallButton (g, rectFor (BtnSave), "SAVE MIDI FILE...", false, {}, isHot (BtnSave));

    // per-lane activity LEDs
    drawSectionHeader (g, { 1052, 532, 172, 18 }, "ACTIVITY", green);
    static const char* shortNames[DrumProcessor::NL] = { "KK", "SN", "CL", "CH", "OH", "LT", "HT", "RM" };
    for (int l = 0; l < DrumProcessor::NL; ++l)
    {
        const size_t li = (size_t) l;
        const float x = 1066.0f + (float) l * 20.5f;
        const int cur = p.currentStep[li].load();
        const bool lit = p.isPlayingNow.load() && cur >= 0 && cur < DrumProcessor::NS
                         && p.getStep (l, cur) > 0 && p.laneMute[li] == 0;
        const auto led = juce::Rectangle<float> (10, 10).withCentre ({ x, 570.0f });
        const auto lc = laneColour (l);
        if (lit)
        {
            g.setColour (lc.withAlpha (0.35f));
            g.fillEllipse (led.expanded (4));
        }
        g.setColour (lit ? lc : lc.withAlpha (0.18f));
        g.fillEllipse (led);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (led, 1.0f);

        g.setColour (dimText);
        g.setFont (font (9.0f, true));
        g.drawText (shortNames[l], juce::Rectangle<float> (x - 10, 582, 20, 12), juce::Justification::centred);
    }
}

// ===========================================================================
void DrumEditor::paint (juce::Graphics& g)
{
    auto& p = processor;
    g.addTransform (juce::AffineTransform::scale (scale()));

    // body
    g.setGradientFill (juce::ColourGradient (bodyTop, 0, 40, bodyBottom, 0, (float) baseH, false));
    g.fillRect (0, 0, baseW, baseH);

    drawTitleBar (g);

    drawPanel (g, { 8, 48, 188, 564 });     // left
    drawPanel (g, { 204, 48, 832, 564 });   // centre
    drawPanel (g, { 1044, 48, 188, 564 });  // right

    // ----- left: groove knobs + pattern settings -----
    drawKnob (g, KnobComplexity, "COMPLEXITY", juce::String (p.param ("complexity")) + " %", 38.0f);
    drawKnob (g, KnobSwing, "SWING", juce::String (p.param ("swing")) + " %", 38.0f);

    drawSectionHeader (g, { 12, 360, 180, 18 }, "PATTERN", blue);
    drawMenuBox (g, BoxRate, "RATE", DrumProcessor::rateName (p.param ("rate")));
    drawMenuBox (g, BoxMap, "NOTES", mapNames[juce::jlimit (0, 2, p.mapSel.load())]);
    drawMenuBox (g, BoxChannel, "MIDI CH", juce::String (p.param ("channel") + 1));

    drawSectionHeader (g, { 12, 494, 180, 18 }, "TRANSPORT", blue);
    {
        const auto r = rectFor (TogglePlay).toFloat();
        const bool on = p.param ("play") != 0;
        const bool hot = isHot (TogglePlay);
        g.setGradientFill (juce::ColourGradient (on ? hex (0x23415a) : hex (0x363a3f), r.getX(), r.getY(),
                                                 on ? hex (0x1a2f42) : hex (0x282b2f), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 15.0f);
        g.setColour (on ? blue.withAlpha (hot ? 1.0f : 0.7f) : (hot ? dimText : panelEdge));
        g.drawRoundedRectangle (r.reduced (0.5f), 15.0f, 1.0f);
        drawPowerIcon (g, { r.getX() + 17, r.getCentreY() }, 6.0f, on ? blue : faintText);
        g.setColour (on ? juce::Colours::white : dimText);
        g.setFont (font (12.0f, true, 0.08f));
        g.drawText ("SYNC TO HOST", r.withTrimmedLeft (32), juce::Justification::centredLeft);
    }
    g.setColour (dimText);
    g.setFont (font (11.0f));
    g.drawText (p.param ("play") ? "Runs with the transport" : "Silent (drag / save still work)",
                20, 558, 170, 16, juce::Justification::centredLeft);

    // ----- centre: tabs + grid -----
    drawTab (g, TabRandomize, "RANDOMIZE", blue, true);
    drawTab (g, TabMutate, "MUTATE", green, false);
    drawTab (g, TabClear, "CLEAR", yellow, false);
    drawTab (g, TabUndo, "UNDO", orange, false);
    drawGrid (g);

    // ----- right: humanize knobs + export -----
    const int vh = p.param ("velhumanize");
    drawKnob (g, KnobVel, "VEL HUMANIZE",
              vh == 0 ? juce::String ("off") : juce::String::fromUTF8 ("\xc2\xb1") + juce::String (vh), 38.0f);
    drawKnob (g, KnobNoteLen, "NOTE LENGTH", juce::String (p.param ("notelength")) + " %", 38.0f);

    drawSectionHeader (g, { 1052, 360, 172, 18 }, "EXPORT", blue);
    const int bars = exportBarCounts[(size_t) juce::jlimit (0, 3, p.param ("exportbars"))];
    drawMenuBox (g, BoxExportBars, "LENGTH", juce::String (bars) + (bars > 1 ? " bars" : " bar"));
    drawExport (g);
}
