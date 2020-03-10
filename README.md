# WinGridStrument

A MIDI instrument inspired by the [LinnStrument](http://www.rogerlinndesign.com/linnstrument.html) and 
[Madrona Labs SoundPlane](http://madronalabs.com/soundplane).

WinGridStrument is a Windows (win32) reimplementation of my [GridStrument](https://github.com/rogerallen/GridStrument) 
and **requires a touchscreen** to work.  Developed on a Surface Pro 7.

## Status

Work-in-progress, but should build & work reasonably.  Let me know if it works for you.

## Usage

1. Start [loopMIDI](http://www.tobias-erichsen.de/software/loopmidi.html) 
2. Connect your MIDI software synthesizer to listen to the LoopMIDI port.
3. Start app & set preferences to connect to LoopMIDI port.
4. Enjoy!

### Preference Controls

- Guitar Mode: change notes vertically by fourths, except for the "B string" and above.  Like a standard guitar tuning.
- Pitch Bend Range: Match this to your MIDI Patch controls.  Typical values are 2 and 12.
- Midi Output Device: Select the MIDI device you will send to.

## License

Copyright (c) 2020, Roger Allen.

Distributed under the GPL3 License.  See the LICENSE.md file.
