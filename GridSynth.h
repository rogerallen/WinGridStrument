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

#include <fluidsynth.h>

class GridSynth
{
    fluid_settings_t     *settings_;
    fluid_synth_t        *synth_;
    fluid_audio_driver_t *adriver_;
    int                   soundfont_id_;

public:
    GridSynth();
    ~GridSynth();

    void noteOn(int channel, int note, int midi_pressure);
    void pitchBend(int channel, int mod_pitch);
    void controlChange(int channel, int controller, int mod_modulation);
    void polyKeyPressure(int channel, int key, int pressure);
};
