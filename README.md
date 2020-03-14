# WinGridStrument

A MIDI instrument inspired by the [LinnStrument](http://www.rogerlinndesign.com/linnstrument.html) and 
[Madrona Labs SoundPlane](http://madronalabs.com/soundplane).

WinGridStrument is a Windows (win32) reimplementation of my [GridStrument](https://github.com/rogerallen/GridStrument) 
and **requires a touchscreen** to work.  Developed on a Surface Pro 7.

## Status

Let me know if it works for you.  Send feedback on twitter or file an Issue.

[![Build status](https://ci.appveyor.com/api/projects/status/7n24031r405mv2bv?svg=true)](https://ci.appveyor.com/project/rogerallen/wingridstrument)
You can get pre-built binaries at this link.  Click the build status button and look for the WinGridStrument.exe under the
Artifacts menu.

## Usage

Press anywhere on the grid to strike a note.  Press down using multiple fingers to create chords.  Notes are arranged 
in horizontal rows where each grid is separated by a semitone (A, A#, B, C, C#, ... ) 
and vertical columns are separated by musical fourths or 5 semitones (A, D, G, C, F, ... ).

The grid shows all C's as blue.  All other chromatic notes (D, E, F, G, A, B) are green.
After striking a note, the same note is drawn as yellow.

When a note is struck, notice a purple square showing the area of your finger's touch.  Velocity and after-pressure
are sent depending on this area.

Once you strike a note, slide your finger left or right to control the pitch bend of the note.    The pitch bend range
setting controls how far you must move to reach a full effect.

Slide your finger vertically to control the modulation of the note.  Sliding up or down one note gets the full effect.

1. Start [loopMIDI](http://www.tobias-erichsen.de/software/loopmidi.html) 
2. Connect your MIDI software synthesizer to listen to the LoopMIDI port.
3. Start app & set preferences to connect to LoopMIDI port.
4. Enjoy!

If you do not have a DAW, or just want to quickly try GridStrument out, you can use the internal Microsoft GS 
synthesizer, but it isn't that responsive.

### Preference Controls

- Guitar Mode: change notes vertically by fourths, except for the "B string" and above.  Like a standard guitar tuning.
- Pitch Bend Range: Match this to your MIDI Patch controls.  Typical values are 2 and 12.  Pitch bend adjusts when you move your finger left and right.
- Pitch Bend Mask: AND with this value before sending in order to quantize.
- Midi Output Device: Select the MIDI device you will send to.
- Modulation MIDI Controller: Select the [controller](https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2) to use when your finger moves up and down.
- MIDI Channel Min/Max: Each finger press gets a new MIDI channel so controls can try be per-note.  Use this to constrain the channels used.
- Grid Size: the width & height of one grid on the screen.  (Default is 90)
- Per Row MIDI Channel Mode

## License

Copyright (c) 2020, Roger Allen.

Distributed under the GPL3 License.  See the LICENSE.md file.
