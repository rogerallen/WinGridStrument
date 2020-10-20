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
#include "GridSynth.h"
#include "GridUtils.h"

// ======================================================================
void checkAlertExit(int rc, std::wstring errStr) {
    if (rc == FLUID_FAILED) {
        std::wostringstream text;
        text << "fluid fail (" << rc << "): " << errStr;
        AlertExit(NULL, text.str().c_str());
    }
}

// ======================================================================
GridSynth::GridSynth() 
{
    settings_ = new_fluid_settings();
    synth_ = new_fluid_synth(settings_);

    int rc = fluid_settings_setstr(settings_, "audio.driver", "dsound");
    checkAlertExit(rc, L"fluid_settings_setstr-dsound");
    adriver_ = new_fluid_audio_driver(settings_, synth_);

    soundfont_id_ = -1;
}

// ======================================================================
GridSynth::~GridSynth() 
{
    delete_fluid_synth(synth_);
    delete_fluid_settings(settings_);
    delete_fluid_audio_driver(adriver_);
}

// ======================================================================
// HOWTO FIXME? On windows the path is wchar.  We convert it
// to a string before calling this routine, but this might not be right
// for international users.
void GridSynth::loadSoundfont(std::string soundfont_path_)
{
    // "/Users/rallen/Documents/Devel/Cpp/WinGridStrument/SoundFonts/VintageDreamsWaves-v2.sf2"
    // "/Users/rallen/Documents/Devel/Cpp/WinGridStrument/SoundFonts/fenderjazz.sf2"
    // "/Users/rallen/Documents/Devel/Cpp/WinGridStrument/SoundFonts/60s_Rock_Guitar.sf2"
    // "/Users/rallen/Documents/Devel/Cpp/WinGridStrument/SoundFonts/Electric_guitar.sf2"
    soundfont_id_ = fluid_synth_sfload(synth_, soundfont_path_.c_str(), TRUE);
    // FLUID_FAILED is -1, so we will not play if the id is < 0
}


// ======================================================================
void GridSynth::noteOn(int channel, int note, int midi_pressure)
{
    if (soundfont_id_ < 0) return;
    fluid_synth_noteon(synth_, channel, note, midi_pressure);
}

// ======================================================================
void GridSynth::pitchBend(int channel, int mod_pitch)
{
    if (soundfont_id_ < 0) return;
    fluid_synth_pitch_bend(synth_, channel, mod_pitch);
}

// ======================================================================
void GridSynth::controlChange(int channel, int controller, int mod_modulation)
{
    if (soundfont_id_ < 0) return;
    fluid_synth_cc(synth_, channel, controller, mod_modulation);
}

// ======================================================================
void GridSynth::polyKeyPressure(int channel, int key, int pressure)
{
    if (soundfont_id_ < 0) return;
    fluid_synth_key_pressure(synth_, channel, key, pressure);
}