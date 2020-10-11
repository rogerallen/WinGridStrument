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
    if (rc != FLUID_OK) {
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
    checkAlertExit(rc, L"fluid_settings_setstr");
    adriver_ = new_fluid_audio_driver(settings_, synth_);
}

// ======================================================================
GridSynth::~GridSynth() 
{
    delete_fluid_synth(synth_);
    delete_fluid_settings(settings_);
    delete_fluid_audio_driver(adriver_);
}

// ======================================================================
void GridSynth::noteOn(int channel, int note, int midi_pressure)
{
    fluid_synth_noteon(synth_, channel, note, midi_pressure);
}

// ======================================================================
void GridSynth::pitchBend(int channel, int mod_pitch)
{
    fluid_synth_pitch_bend(synth_, channel, mod_pitch);
}

// ======================================================================
void GridSynth::controlChange(int channel, int controller, int mod_modulation)
{
    // FIXME
}

// ======================================================================
void GridSynth::polyKeyPressure(int channel, int key, int pressure)
{
    // FIXME
}