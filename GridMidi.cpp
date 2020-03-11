#include "GridMidi.h"
#include "GridUtils.h"
#include <iostream>

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
        AlertExit(NULL, text.str().c_str());
    }
}

void GridMidi::noteOn(int channel, int note, int midi_pressure)
{
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