#pragma once
// ======================================================================
// WinGridStrument - a Windows touchscreen musical instrument
// Copyright(C) 2020 Roger Allen
// 
// This program is free software : you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// ======================================================================
#include <windows.h>
#include <mmsystem.h>

// type which is both an integer and an array of characters:
// typedef union midimessage { unsigned long word; unsigned char data[4]; } MidiMessage;
class MidiMessage
{
    union { unsigned long word; unsigned char data[4]; } msg;
public:
    MidiMessage(int d0, int d1, int d2) {
        msg.data[0] = static_cast<unsigned char>(d0);
        msg.data[1] = static_cast<unsigned char>(d1);
        msg.data[2] = static_cast<unsigned char>(d2);
        msg.data[3] = 0;
    };
    unsigned long data() { return msg.word; }
};

// https://www.midi.org/specifications-old/item/table-1-summary-of-midi-message
namespace MIDI {
    const int
        NOTE_OFF = 0x80,
        NOTE_ON = 0x90,
        POLY_KEY_PRESSURE = 0xa0,
        CONTROL_CHANGE = 0xb0,
        PROGRAM_CHANGE = 0xc0,
        CHANNEL_PRESSURE = 0xd0,
        PITCH_BEND = 0xe0,
        SYS_EX = 0xf0;
};

class GridMidi
{
    HMIDIOUT midi_device_;
public:
    GridMidi(HMIDIOUT midiDevice);

    void noteOn(int channel, int note, int midi_pressure);
    void pitchBend(int channel, int mod_pitch);
    void controlChange(int channel, int controller, int mod_modulation);
    void polyKeyPressure(int channel, int key, int pressure);
};

