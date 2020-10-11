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
#include "GridMidi.h"
#include "GridUtils.h"
#include <iostream>

// FIXME
#include "GridSynth.h"
extern GridSynth* g_gridSynth;

GridMidi::GridMidi(HMIDIOUT midiDevice)
{
    midi_device_ = midiDevice;
}

void checkAlertExit(MMRESULT rc) {
    if (rc != MMSYSERR_NOERROR) {
        wchar_t* error = new wchar_t[MAXERRORLENGTH]();
        midiOutGetErrorText(rc, error, MAXERRORLENGTH);
        std::wostringstream text;
        text << "Unable to midiOutShortMsg: " << error;
        delete[] error;
        AlertExit(NULL, text.str().c_str());
    }
}

void GridMidi::noteOn(int channel, int note, int midi_pressure)
{
    g_gridSynth->noteOn(channel, note, midi_pressure); // FIXME
    return;

    MidiMessage message(MIDI::NOTE_ON + channel, note, midi_pressure);
    MMRESULT rc = midiOutShortMsg(midi_device_, message.data());
    checkAlertExit(rc);
}

void GridMidi::pitchBend(int channel, int mod_pitch)
{
    MidiMessage message(MIDI::PITCH_BEND + channel, mod_pitch & 0x7f, (mod_pitch >> 7) & 0x7f);
    MMRESULT rc = midiOutShortMsg(midi_device_, message.data());
    checkAlertExit(rc);
}

void GridMidi::controlChange(int channel, int controller, int mod_modulation)
{
    MidiMessage message(MIDI::CONTROL_CHANGE + channel, controller, mod_modulation);
    MMRESULT rc = midiOutShortMsg(midi_device_, message.data());
    checkAlertExit(rc);
}

void GridMidi::polyKeyPressure(int channel, int key, int pressure)
{
    MidiMessage message(MIDI::POLY_KEY_PRESSURE + channel, key, pressure);
    MMRESULT rc = midiOutShortMsg(midi_device_, message.data());
    checkAlertExit(rc);
}