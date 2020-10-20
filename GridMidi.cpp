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

// Not just a midi device any longer.  A Midi or Synth output
GridMidi::GridMidi(HMIDIOUT midiDevice, GridSynth *gridSynth)
{
    midi_device_ = midiDevice;
    grid_synth_ = gridSynth;
    play_midi_ = false;
    play_synth_ = false;
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
    if (play_synth_) {
        grid_synth_->noteOn(channel, note, midi_pressure); // FIXME
    }
    if (play_midi_) {
        MidiMessage message(MIDI::NOTE_ON + channel, note, midi_pressure);
        MMRESULT rc = midiOutShortMsg(midi_device_, message.data());
        checkAlertExit(rc);
    }
}

void GridMidi::pitchBend(int channel, int mod_pitch)
{
    if (play_synth_) {
        grid_synth_->pitchBend(channel, mod_pitch); // FIXME
    }
    if (play_midi_) {
        MidiMessage message(MIDI::PITCH_BEND + channel, mod_pitch & 0x7f, (mod_pitch >> 7) & 0x7f);
        MMRESULT rc = midiOutShortMsg(midi_device_, message.data());
        checkAlertExit(rc);
    }
}

void GridMidi::controlChange(int channel, int controller, int mod_modulation)
{
    if (play_synth_) {
        grid_synth_->controlChange(channel, controller, mod_modulation); // FIXME
    }
    if (play_midi_) {
        MidiMessage message(MIDI::CONTROL_CHANGE + channel, controller, mod_modulation);
        MMRESULT rc = midiOutShortMsg(midi_device_, message.data());
        checkAlertExit(rc);
    }
}

void GridMidi::polyKeyPressure(int channel, int key, int pressure)
{
    if (play_synth_) {
        grid_synth_->polyKeyPressure(channel, key, pressure); // FIXME
    }
    if (play_midi_) {
        MidiMessage message(MIDI::POLY_KEY_PRESSURE + channel, key, pressure);
        MMRESULT rc = midiOutShortMsg(midi_device_, message.data());
        checkAlertExit(rc);
    }
}