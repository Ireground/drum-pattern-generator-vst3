#include "PluginProcessor.h"
#include "PluginEditor.h"

DrumProcessor::DrumProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DrumPatternGenerator", createLayout())
{
    for (auto& s : steps) s.store (0);
    for (int l = 0; l < NL; ++l)
    {
        laneLen[(size_t) l] = 16; laneChance[(size_t) l] = 100;
        laneMute[(size_t) l] = 0; laneSolo[(size_t) l] = 0; laneLock[(size_t) l] = 0;
        laneNote[(size_t) l] = 36;
        currentStep[(size_t) l] = -1;
        offRemaining[(size_t) l] = -1;
    }
    applyMap (0);
    defaultPattern();
}

juce::AudioProcessorValueTreeState::ParameterLayout DrumProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    StringArray channels;
    for (int i = 1; i <= 16; ++i) channels.add (String (i));

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "rate", 1 }, "Rate",
                    StringArray { "1/8", "1/16", "1/32", "1/8T", "1/16T" }, 1));
    layout.add (std::make_unique<AudioParameterInt>    (ParameterID { "swing", 1 },      "Swing",      0, 75, 0));
    layout.add (std::make_unique<AudioParameterInt>    (ParameterID { "complexity", 1 }, "Complexity", 0, 100, 50));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Style",
                    StringArray { "Random", "House", "Techno", "Breakbeat", "Boom Bap", "Trap", "Minimal" }, 1));
    layout.add (std::make_unique<AudioParameterInt>    (ParameterID { "velhumanize", 1 }, "Velocity Humanize", 0, 40, 8));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "channel", 1 },    "MIDI Channel", channels, 0));
    layout.add (std::make_unique<AudioParameterInt>    (ParameterID { "notelength", 1 }, "Note Length", 5, 100, 50));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "exportbars", 1 }, "Drag Length",
                    StringArray { "1 bar", "2 bars", "4 bars", "8 bars" }, 0));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "play", 1 },       "Play With Transport", true));
    return layout;
}

// ---------------------------------------------------------------------------
const char* DrumProcessor::laneName (int i)
{
    static const char* n[NL] = { "Kick", "Snare", "Clap", "C.Hat", "O.Hat", "Tom L", "Tom H", "Rim" };
    return n[juce::jlimit (0, NL - 1, i)];
}
const char* DrumProcessor::styleName (int i)
{
    static const char* n[7] = { "Random", "House", "Techno", "Breakbeat", "Boom Bap", "Trap", "Minimal" };
    return n[juce::jlimit (0, 6, i)];
}
const char* DrumProcessor::rateName (int i)
{
    static const char* n[5] = { "1/8", "1/16", "1/32", "1/8T", "1/16T" };
    return n[juce::jlimit (0, 4, i)];
}

int DrumProcessor::param (const char* id) const
{
    return juce::roundToInt (apvts.getRawParameterValue (id)->load());
}

void DrumProcessor::setParam (const char* id, float value)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        p->endChangeGesture();
    }
}

double DrumProcessor::stepBeats() const
{
    switch (param ("rate"))
    {
        case 0:  return 0.5;
        case 2:  return 0.125;
        case 3:  return 1.0 / 3.0;
        case 4:  return 1.0 / 6.0;
        default: return 0.25;
    }
}

// ---------------------------------------------------------------------------
void DrumProcessor::applyMap (int m)
{
    if (m == 0)
    {
        const int gm[NL] = { 36, 38, 39, 42, 46, 45, 50, 37 };
        for (int i = 0; i < NL; ++i) laneNote[(size_t) i] = gm[i];
    }
    else
    {
        for (int i = 0; i < NL; ++i) laneNote[(size_t) i] = (m == 1 ? 36 : 60) + i;
    }
    mapSel = m;
}

void DrumProcessor::defaultPattern()
{
    for (auto& s : steps) s.store (0);
    for (int s = 0; s < 16; ++s)
    {
        if (s % 4 == 0)          setStep (0, s, 115);
        if (s == 4 || s == 12)   setStep (2, s, 110);
        if (s % 2 == 0)          setStep (3, s, s % 4 == 2 ? 100 : 70);
    }
}

// probability of a hit: style, lane, position in bar (0..15), complexity 0..1
float DrumProcessor::styleProbability (int st, int l, int pos, float c) const
{
    const bool odd = pos % 2 == 1;
    const int q = pos % 4;
    float p = 0.0f;

    if (st == 0) // Random
    {
        if (l == 0)      p = pos == 0 ? 0.9f : 0.1f + 0.3f * c;
        else if (l == 3) p = 0.25f + 0.55f * c;
        else if (l == 4) p = 0.05f + 0.2f * c;
        else             p = 0.06f + 0.3f * c;
    }
    else if (st == 1) // House
    {
        if (l == 0)      p = q == 0 ? 1.0f : (odd ? 0.03f : 0.1f) * c;
        else if (l == 1) p = (pos == 4 || pos == 12) ? 0.3f : (odd ? 0.12f : 0.05f) * c;
        else if (l == 2) p = (pos == 4 || pos == 12) ? 1.0f : (pos == 7 || pos == 15) ? 0.2f * c : 0.03f * c;
        else if (l == 3) p = q == 2 ? 1.0f : odd ? 0.5f * c : 0.2f * c;
        else if (l == 4) p = q == 2 ? 0.3f + 0.4f * c : 0.03f * c;
        else if (l == 7) p = (q == 1 || q == 3) ? 0.05f + 0.25f * c : 0.02f * c;
        else             p = (0.03f + 0.18f * c) * (odd ? 1.0f : 0.6f);
    }
    else if (st == 2) // Techno
    {
        if (l == 0)      p = q == 0 ? 1.0f : (pos == 14 || pos == 7) ? 0.15f * c : 0.02f * c;
        else if (l == 1) p = 0.06f * c;
        else if (l == 2) p = (pos == 4 || pos == 12) ? 0.7f : 0.04f * c;
        else if (l == 3) p = q == 2 ? 1.0f : 0.35f + 0.55f * c;
        else if (l == 4) p = q == 2 ? 0.6f + 0.3f * c : 0.02f * c;
        else if (l == 7) p = (pos == 3 || pos == 6 || pos == 11 || pos == 14) ? 0.15f + 0.5f * c : 0.03f * c;
        else             p = (0.03f + 0.15f * c) * (odd ? 1.0f : 0.5f);
    }
    else if (st == 3) // Breakbeat
    {
        if (l == 0)      p = pos == 0 ? 1.0f : pos == 10 ? 0.85f : pos == 7 ? 0.2f + 0.4f * c : pos == 2 ? 0.3f * c : 0.05f * c;
        else if (l == 1) p = (pos == 4 || pos == 12) ? 1.0f : odd ? 0.04f + 0.2f * c : 0.03f * c;
        else if (l == 2) p = (pos == 4 || pos == 12) ? 0.25f : 0.02f * c;
        else if (l == 3) p = odd ? 0.35f * c : 0.9f;
        else if (l == 4) p = (pos == 6 || pos == 14) ? 0.2f + 0.3f * c : 0.02f * c;
        else             p = (0.03f + 0.15f * c) * (odd ? 1.0f : 0.6f);
    }
    else if (st == 4) // Boom Bap
    {
        if (l == 0)      p = pos == 0 ? 1.0f : pos == 10 ? 0.8f : pos == 7 ? 0.35f + 0.3f * c : pos == 14 ? 0.2f * c : 0.03f * c;
        else if (l == 1) p = (pos == 4 || pos == 12) ? 1.0f : odd ? 0.1f * c : 0.02f * c;
        else if (l == 2) p = (pos == 4 || pos == 12) ? 0.3f : 0.02f * c;
        else if (l == 3) p = odd ? 0.15f * c : 0.95f;
        else if (l == 4) p = pos == 14 ? 0.15f + 0.2f * c : 0.03f * c;
        else             p = (0.02f + 0.1f * c) * (odd ? 1.0f : 0.6f);
    }
    else if (st == 5) // Trap (half-time feel: snare on beat 3)
    {
        if (l == 0)      p = pos == 0 ? 1.0f : pos == 11 ? 0.5f : pos == 6 ? 0.3f + 0.3f * c : (pos == 3 || pos == 14) ? 0.3f * c : 0.04f * c;
        else if (l == 1) p = pos == 8 ? 1.0f : 0.05f * c;
        else if (l == 2) p = pos == 8 ? 0.8f : 0.03f * c;
        else if (l == 3) p = odd ? 0.25f + 0.6f * c : 1.0f;
        else if (l == 4) p = 0.07f * c;
        else             p = (0.03f + 0.12f * c) * (odd ? 1.0f : 0.6f);
    }
    else // Minimal
    {
        if (l == 0)      p = q == 0 ? 1.0f : 0.02f * c;
        else if (l == 1) p = 0.04f * c;
        else if (l == 2) p = (pos == 4 || pos == 12) ? 0.5f : 0.02f * c;
        else if (l == 3) p = q == 2 ? 0.8f : 0.15f * c;
        else if (l == 4) p = 0.05f * c;
        else if (l == 7) p = (pos == 3 || pos == 6 || pos == 10 || pos == 13) ? 0.15f + 0.35f * c : 0.02f * c;
        else             p = (0.02f + 0.1f * c) * (odd ? 1.0f : 0.5f);
    }
    return p;
}

int DrumProcessor::styleVelocity (int l, int pos)
{
    const float w = pos % 4 == 0 ? 1.0f : pos % 2 == 0 ? 0.7f : 0.4f;
    float v = (l == 3 || l == 4) ? 55.0f + 60.0f * w : 65.0f + 55.0f * w;
    v += (rng.nextFloat() * 2.0f - 1.0f) * 6.0f;
    return juce::jlimit (1, 127, (int) std::floor (v));
}

// ---------------------------------------------------------------------------
void DrumProcessor::saveUndo()
{
    for (size_t i = 0; i < undoBuf.size(); ++i) undoBuf[i] = steps[i].load();
    hasUndo = true;
}

void DrumProcessor::undo()
{
    if (! hasUndo) return;
    for (size_t i = 0; i < undoBuf.size(); ++i)
    {
        const int t = steps[i].load();
        steps[i].store (undoBuf[i]);
        undoBuf[i] = t;
    }
}

void DrumProcessor::randomizeLane (int l)
{
    const int style = param ("style");
    const float c = param ("complexity") / 100.0f;
    for (int s = 0; s < NS; ++s)
    {
        const int pos = s % 16;
        if (s < laneLen[(size_t) l])
        {
            const float p = styleProbability (style, l, pos, c);
            setStep (l, s, rng.nextFloat() < p ? styleVelocity (l, pos) : 0);
        }
        else
        {
            setStep (l, s, 0);
        }
    }
}

// open hat cuts the closed hat on the same step
void DrumProcessor::chokeHats()
{
    if (laneLock[3]) return;
    for (int s = 0; s < NS; ++s)
        if (getStep (4, s) > 0) setStep (3, s, 0);
}

void DrumProcessor::randomizeAll()
{
    saveUndo();
    for (int l = 0; l < NL; ++l)
        if (! laneLock[(size_t) l]) randomizeLane (l);
    chokeHats();
}

void DrumProcessor::mutateAll()
{
    saveUndo();
    const int style = param ("style");
    const float c = param ("complexity") / 100.0f;
    const float m = 0.06f + 0.14f * c;
    for (int l = 0; l < NL; ++l)
    {
        if (laneLock[(size_t) l]) continue;
        for (int s = 0; s < laneLen[(size_t) l]; ++s)
        {
            if (rng.nextFloat() < m)
            {
                const int pos = s % 16;
                const float p = styleProbability (style, l, pos, c);
                setStep (l, s, rng.nextFloat() < p ? styleVelocity (l, pos) : 0);
            }
        }
    }
    chokeHats();
}

void DrumProcessor::clearAll()
{
    saveUndo();
    for (int l = 0; l < NL; ++l)
        if (! laneLock[(size_t) l])
            for (int s = 0; s < NS; ++s) setStep (l, s, 0);
}

// ---------------------------------------------------------------------------
void DrumProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr > 0 ? sr : 44100.0;
    wasPlaying = false;
}

bool DrumProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void DrumProcessor::allNotesOff (juce::MidiBuffer& midi)
{
    for (int l = 0; l < NL; ++l)
    {
        if (offRemaining[(size_t) l] >= 0)
        {
            midi.addEvent (juce::MidiMessage::noteOff (offChannel[(size_t) l], offNote[(size_t) l]), 0);
            offRemaining[(size_t) l] = -1;
        }
        currentStep[(size_t) l] = -1;
    }
}

void DrumProcessor::fireStep (juce::int64 k, int o, juce::MidiBuffer& midi, int channel, int gateSamples)
{
    bool anySolo = false;
    for (int l = 0; l < NL; ++l) anySolo = anySolo || laneSolo[(size_t) l] != 0;

    const int hum = param ("velhumanize");

    for (int l = 0; l < NL; ++l)
    {
        const int len = std::max (1, laneLen[(size_t) l].load());
        const int s = (int) (k % len);
        currentStep[(size_t) l] = s;
        int v = getStep (l, s);

        if (v > 0 && ! laneMute[(size_t) l] && (! anySolo || laneSolo[(size_t) l]))
        {
            if (audioRng.nextFloat() * 100.0f < (float) laneChance[(size_t) l])
            {
                v = juce::jlimit (1, 127, (int) std::floor (v + (audioRng.nextFloat() * 2.0f - 1.0f) * hum));
                if (offRemaining[(size_t) l] >= 0)
                {
                    midi.addEvent (juce::MidiMessage::noteOff (offChannel[(size_t) l], offNote[(size_t) l]),
                                   std::min (offRemaining[(size_t) l], o));
                    offRemaining[(size_t) l] = -1;
                }
                const int n = laneNote[(size_t) l];
                midi.addEvent (juce::MidiMessage::noteOn (channel, n, (juce::uint8) v), o);
                offRemaining[(size_t) l] = o + gateSamples;
                offNote[(size_t) l] = n;
                offChannel[(size_t) l] = channel;
            }
        }
    }
}

void DrumProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const int numSamples = buffer.getNumSamples();

    // incoming MIDI passes through; also used for note learn
    const int learn = learnLane.load();
    if (learn >= 0)
    {
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                laneNote[(size_t) learn] = msg.getNoteNumber();
                learnLane = -1;
                break;
            }
        }
    }

    const int channel = param ("channel") + 1;
    const double sb = stepBeats();

    bool playing = false;
    double ppq = 0.0, bpm = 120.0;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) ppq = *p; else playing = false;
            if (auto b = pos->getBpm())         bpm = *b;
        }
    }

    const bool enabled = apvts.getRawParameterValue ("play")->load() > 0.5f;
    const double bps = bpm / 60.0 / sampleRate;  // beats per sample
    const int gateSamples = std::max (32, (int) std::floor (sb * param ("notelength") / 100.0 / bps));

    if (playing && enabled && numSamples > 0)
    {
        double b0;
        if (wasPlaying && std::abs (ppq - expectedPos) < 0.05)
            b0 = expectedPos;
        else
        {
            b0 = ppq;
            allNotesOff (midi);
            lastFired = std::numeric_limits<juce::int64>::min();
        }
        const double b1 = b0 + numSamples * bps;
        expectedPos = b1;

        const double swingOffset = sb * 0.5 * param ("swing") / 100.0;
        juce::int64 k = std::max<juce::int64> (0, (juce::int64) std::floor (b0 / sb) - 1);
        const juce::int64 kEnd = (juce::int64) std::floor (b1 / sb);

        for (; k <= kEnd; ++k)
        {
            const double t = (double) k * sb + ((k & 1) ? swingOffset : 0.0);
            if (t >= b0 && t < b1 && k > lastFired)
            {
                const int o = juce::jlimit (0, numSamples - 1, (int) std::floor ((t - b0) / bps));
                fireStep (k, o, midi, channel, gateSamples);
                lastFired = k;
            }
        }
        wasPlaying = true;
    }
    else if (wasPlaying)
    {
        allNotesOff (midi);
        wasPlaying = false;
    }
    isPlayingNow = playing && enabled;

    // pending note-offs
    for (int l = 0; l < NL; ++l)
    {
        auto& rem = offRemaining[(size_t) l];
        if (rem >= 0)
        {
            if (rem < numSamples)
            {
                midi.addEvent (juce::MidiMessage::noteOff (offChannel[(size_t) l], offNote[(size_t) l]), rem);
                rem = -1;
            }
            else
            {
                rem -= numSamples;
            }
        }
    }
}

// ---------------------------------------------------------------------------
juce::File DrumProcessor::writeMidiFile()
{
    constexpr int tpq = 960;
    const double sb = stepBeats();
    const int bars = std::array<int, 4> { 1, 2, 4, 8 } [(size_t) juce::jlimit (0, 3, param ("exportbars"))];
    const int totalSteps = juce::roundToInt (bars * 4.0 / sb);
    const double swingOffset = sb * 0.5 * param ("swing") / 100.0;
    const double gateBeats = sb * param ("notelength") / 100.0;
    const int channel = param ("channel") + 1;
    const int hum = param ("velhumanize");

    bool anySolo = false;
    for (int l = 0; l < NL; ++l) anySolo = anySolo || laneSolo[(size_t) l] != 0;

    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, "Drums"), 0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);

    for (int k = 0; k < totalSteps; ++k)
    {
        const double t = k * sb + ((k & 1) ? swingOffset : 0.0);
        for (int l = 0; l < NL; ++l)
        {
            const int len = std::max (1, laneLen[(size_t) l].load());
            int v = getStep (l, k % len);
            if (v <= 0 || laneMute[(size_t) l] || (anySolo && ! laneSolo[(size_t) l])) continue;
            if (rng.nextFloat() * 100.0f >= (float) laneChance[(size_t) l]) continue;

            v = juce::jlimit (1, 127, (int) std::floor (v + (rng.nextFloat() * 2.0f - 1.0f) * hum));
            const double on  = std::round (t * tpq);
            const double off = on + std::max (30.0, std::round (gateBeats * tpq));
            const int n = laneNote[(size_t) l];
            seq.addEvent (juce::MidiMessage::noteOn (channel, n, (juce::uint8) v), on);
            seq.addEvent (juce::MidiMessage::noteOff (channel, n), off);
        }
    }

    seq.addEvent (juce::MidiMessage::endOfTrack(), (double) bars * 4 * tpq);
    seq.updateMatchedPairs();

    juce::MidiFile file;
    file.setTicksPerQuarterNote (tpq);
    file.addTrack (seq);

    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("DrumPatternGenerator");
    dir.createDirectory();
    const juce::String name = juce::String ("Drums ") + juce::String (styleName (param ("style"))).removeCharacters (" ")
                            + " " + juce::String (bars) + "bar "
                            + juce::Time::getCurrentTime().formatted ("%H%M%S") + ".mid";
    auto out = dir.getChildFile (juce::File::createLegalFileName (name));

    juce::FileOutputStream stream (out);
    if (stream.openedOk())
    {
        stream.setPosition (0);
        stream.truncate();
        file.writeTo (stream, 1);
        stream.flush();
    }
    return out;
}

// ---------------------------------------------------------------------------
void DrumProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr) return;

    auto* pat = xml->createNewChildElement ("Pattern");
    pat->setAttribute ("map", mapSel.load());
    for (int l = 0; l < NL; ++l)
    {
        auto* lane = pat->createNewChildElement ("Lane");
        lane->setAttribute ("note",   laneNote[(size_t) l].load());
        lane->setAttribute ("len",    laneLen[(size_t) l].load());
        lane->setAttribute ("mute",   laneMute[(size_t) l].load());
        lane->setAttribute ("solo",   laneSolo[(size_t) l].load());
        lane->setAttribute ("chance", laneChance[(size_t) l].load());
        lane->setAttribute ("lock",   laneLock[(size_t) l].load());
        juce::StringArray vals;
        for (int s = 0; s < NS; ++s) vals.add (juce::String (getStep (l, s)));
        lane->setAttribute ("steps", vals.joinIntoString (","));
    }
    copyXmlToBinary (*xml, destData);
}

void DrumProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;

    if (auto* pat = xml->getChildByName ("Pattern"))
    {
        mapSel = pat->getIntAttribute ("map", 0);
        int l = 0;
        for (auto* lane : pat->getChildWithTagNameIterator ("Lane"))
        {
            if (l >= NL) break;
            laneNote[(size_t) l]   = juce::jlimit (0, 127, lane->getIntAttribute ("note", 36));
            laneLen[(size_t) l]    = juce::jlimit (1, NS, lane->getIntAttribute ("len", 16));
            laneMute[(size_t) l]   = lane->getIntAttribute ("mute", 0);
            laneSolo[(size_t) l]   = lane->getIntAttribute ("solo", 0);
            laneChance[(size_t) l] = juce::jlimit (0, 100, lane->getIntAttribute ("chance", 100));
            laneLock[(size_t) l]   = lane->getIntAttribute ("lock", 0);
            auto vals = juce::StringArray::fromTokens (lane->getStringAttribute ("steps"), ",", "");
            for (int s = 0; s < NS; ++s)
                setStep (l, s, s < vals.size() ? juce::jlimit (0, 127, vals[s].getIntValue()) : 0);
            ++l;
        }
        xml->removeChildElement (pat, true);
    }
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* DrumProcessor::createEditor()
{
    return new DrumEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumProcessor();
}
