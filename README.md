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
- Put it on a track **before your drum plugin**; with **Sync to host** on it plays with the transport.
- **Style** (title bar): pick with the arrows or click the name for a list.
- **Randomize / Mutate / Clear / Undo** tabs above the grid.
- Knobs: **Complexity**, **Swing**, **Vel Humanize**, **Note Length** (drag up/down, Shift = fine, double-click = reset).
- Per lane: Mute / Solo / Lock / Rnd, Note, Length (polymeter) and Chance.
- Right-click a Note box and play a key to learn the note. Right-drag a step to set its velocity.
- **Drag MIDI**: drop the pattern onto a REAPER track. **Length** sets how many bars the dropped item is.
- The window is resizable (drag the bottom-right corner); the size is remembered.
