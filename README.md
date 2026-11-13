# Drum Pattern Generator (VST3)

An 8-lane MIDI drum sequencer with style-based randomization (House, Techno, Breakbeat,
Boom Bap, Trap, Minimal, Random). Drives any drum plugin, e.g. Microtonic or Sitala,
and lets you **drag the pattern straight onto a REAPER track** as a MIDI item.

## Install (Windows)
1. Open the **Actions** tab of this repository and click the latest green run.
2. Under **Artifacts**, download **Drum-Pattern-Generator-VST3-Windows** and unzip it.
3. Copy the `Drum Pattern Generator.vst3` folder into `C:\Program Files\Common Files\VST3`.
4. In REAPER: Options > Preferences > Plug-ins > VST > **Re-scan**.

## Use
- Put it on a track **before your drum plugin**; it plays in sync with the transport.
- **Drag MIDI**: drop the pattern onto the arrange view. **Drag: N bars** sets how long the dropped item is.
- RANDOMIZE / Mutate / Clear / Undo, per-lane Mute / Solo / Lock / Rnd, Note, Length (polymeter) and Chance.
- Right-click a Note box and play a key to learn the note. Right-drag a step to set its velocity.
