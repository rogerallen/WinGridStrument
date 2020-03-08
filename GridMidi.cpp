#include "GridMidi.h"
#include <iostream>

GridMidi::GridMidi(HMIDIOUT midiDevice)
{
    midi_device_ = midiDevice;
}

void GridMidi::noteOn(int channel, int note, int midi_pressure)
{
    MidiMessage message;
    message.data[0] = static_cast<unsigned char>(0x90 + channel);  // MIDI note-on message (requires to data bytes)
    message.data[1] = static_cast<unsigned char>(note);  // MIDI note-on message: Key number (60 = middle C)
    message.data[2] = static_cast<unsigned char>(midi_pressure);   // MIDI note-on message: Key velocity (100 = loud)
    message.data[3] = 0;     // Unused parameter
    MMRESULT rc = midiOutShortMsg(midi_device_, message.word);
    if (rc != MMSYSERR_NOERROR) {
        // FIXME - alert dialog box
        std::wcout << "Warning: MIDI Output is not open.\n";
    }
}

void GridMidi::pitchBend(int channel, int mod_pitch)
{
    MidiMessage message;
    message.data[0] = static_cast<unsigned char>(0xe0 + channel);  // MIDI Pitch Bend Change
    message.data[1] = static_cast<unsigned char>(mod_pitch & 0x7f);  // low bits
    message.data[2] = static_cast<unsigned char>((mod_pitch >> 7) & 0x7f); // high bits
    message.data[3] = 0;     // Unused parameter
    MMRESULT rc = midiOutShortMsg(midi_device_, message.word);
    if (rc != MMSYSERR_NOERROR) {
        // FIXME - alert dialog box
        std::wcout << "Warning: MIDI Output is not open.\n";
    }
}

void GridMidi::controlChange(int channel, int controller, int mod_modulation)
{
    MidiMessage message;
    message.data[0] = static_cast<unsigned char>(0xb0 + channel);  // MIDI Control Change
    message.data[1] = static_cast<unsigned char>(controller);               // controller
    message.data[2] = static_cast<unsigned char>(mod_modulation);  // value
    message.data[3] = 0;     // Unused parameter
    MMRESULT rc = midiOutShortMsg(midi_device_, message.word);
    if (rc != MMSYSERR_NOERROR) {
        // FIXME - alert dialog box
        std::wcout << "Warning: MIDI Output is not open.\n";
    }
}
