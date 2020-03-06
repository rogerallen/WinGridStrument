#pragma once
// ======================================================================
// WinGridStrument - a Windows touchscreen musical instrument
// Copyright(C) 2020 Roger Allen
// 
// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.If not, see < https://www.gnu.org/licenses/>.
// ======================================================================
#include <windows.h>
#include <mmsystem.h>

// type which is both an integer and an array of characters:
typedef union midimessage { unsigned long word; unsigned char data[4]; } MidiMessage;

class GridMidi
{
    HMIDIOUT midi_device_;
public:
    GridMidi(HMIDIOUT midiDevice);

    void noteOn(int channel, int note, int midi_pressure);
    void pitchBend(int channel, int mod_pitch);
    void controlChange(int channel, int controller, int mod_modulation);
};

